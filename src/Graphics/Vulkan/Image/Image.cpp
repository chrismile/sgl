/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstring>

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "../Utils/Interop.hpp"
#include "../Buffers/Buffer.hpp"
#include "Image.hpp"

namespace sgl { namespace vk {

Image::Image(Device* device, const ImageSettings& imageSettings) : device(device), imageSettings(imageSettings) {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.extent.width = imageSettings.width;
    imageCreateInfo.extent.height = imageSettings.height;
    imageCreateInfo.extent.depth = imageSettings.depth;
    imageCreateInfo.imageType = imageSettings.imageType;
    imageCreateInfo.mipLevels = imageSettings.mipLevels;
    imageCreateInfo.arrayLayers = imageSettings.arrayLayers;
    imageCreateInfo.format = imageSettings.format;
    imageCreateInfo.tiling = imageSettings.tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = imageSettings.usage;
    imageCreateInfo.sharingMode = imageSettings.sharingMode;
    imageCreateInfo.samples = imageSettings.numSamples;
    imageCreateInfo.flags = 0;

    if (!imageSettings.exportMemory) {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = imageSettings.memoryUsage;

        VmaAllocationInfo textureImageAllocationInfo;
        if (vmaCreateImage(
                device->getAllocator(), &imageCreateInfo, &allocCreateInfo, &image,
                &imageAllocation, &textureImageAllocationInfo) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError("Image::Image: vmaCreateImage failed!");
        }
    } else {
#if defined(_WIN32)
        VkExternalMemoryHandleTypeFlags handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
        VkExternalMemoryHandleTypeFlags handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
        Logfile::get()->throwError(
                "Error in Image::Image: External memory is only supported on Linux, Android and Windows systems!");
#endif

        VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = {};
        externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        externalMemoryImageCreateInfo.handleTypes = handleTypes;
        imageCreateInfo.pNext = &externalMemoryImageCreateInfo;

        // If the memory should be exported for external use, we need to allocate the memory manually.
        if (vkCreateImage(device->getVkDevice(), &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Image::Image: Failed to create an image!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device->getVkDevice(), image, &memoryRequirements);
        deviceMemorySizeInBytes = memoryRequirements.size;

        VkExportMemoryAllocateInfo exportMemoryAllocateInfo = {};
        exportMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportMemoryAllocateInfo.handleTypes = handleTypes;

        VkMemoryPropertyFlags memoryPropertyFlags = convertVmaMemoryUsageToVkMemoryPropertyFlags(
                imageSettings.memoryUsage);

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = device->findMemoryTypeIndex(
                memoryRequirements.memoryTypeBits, memoryPropertyFlags);
        memoryAllocateInfo.pNext = &exportMemoryAllocateInfo;

        if (memoryAllocateInfo.memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
            Logfile::get()->throwError("Error in Image::Image: No suitable memory type index found!");
        }

        if (vkAllocateMemory(device->getVkDevice(), &memoryAllocateInfo, 0, &deviceMemory) != VK_SUCCESS) {
            Logfile::get()->throwError("Error in Image::Image: Could not allocate memory!");
        }

        vkBindImageMemory(device->getVkDevice(), image, deviceMemory, 0);
    }
}

Image::Image(Device* device, const ImageSettings& imageSettings, VkImage image, bool takeImageOwnership)
    : device(device), hasImageOwnership(takeImageOwnership), imageSettings(imageSettings), image(image) {
}

Image::Image(
        Device* device, const ImageSettings& imageSettings, VkImage image,
        VmaAllocation imageAllocation, VmaAllocationInfo imageAllocationInfo)
        : device(device), imageSettings(imageSettings), image(image), imageAllocation(imageAllocation),
        imageAllocationInfo(imageAllocationInfo) {
    this->imageSettings = imageSettings;
    this->image = image;
    this->imageAllocation = imageAllocation;
    this->imageAllocationInfo = imageAllocationInfo;
    this->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

Image::~Image() {
    if (hasImageOwnership) {
        if (imageAllocation) {
            vmaDestroyImage(device->getAllocator(), image, imageAllocation);
        }
        if (deviceMemory) {
            vkDestroyImage(device->getVkDevice(), image, nullptr);
            vkFreeMemory(device->getVkDevice(), deviceMemory, nullptr);
        }
    }
}

ImagePtr Image::copy(bool copyContent, VkImageAspectFlags aspectFlags) {
    ImagePtr newImage(new Image(device, imageSettings));
    if (copyContent) {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        VkImageSubresourceLayers subresource{};
        subresource.aspectMask = aspectFlags;
        subresource.mipLevel = 0;
        subresource.baseArrayLayer = 0;
        subresource.layerCount = imageSettings.arrayLayers;

        VkImageCopy imageCopy{};
        imageCopy.srcSubresource = subresource;
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource = subresource;
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.extent = { imageSettings.width, imageSettings.height, imageSettings.depth };
        vkCmdCopyImage(
                commandBuffer, this->image, this->imageLayout, newImage->image, newImage->imageLayout,
                1, &imageCopy);
        device->endSingleTimeCommands(commandBuffer);
    }
    return newImage;
}

void Image::uploadData(VkDeviceSize sizeInBytes, void* data, bool generateMipmaps) {
    if (imageSettings.mipLevels <= 1) {
        generateMipmaps = false;
    }

    BufferPtr stagingBuffer(new Buffer(
            device, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY));

    void* stagingBufferData = stagingBuffer->mapMemory();
    memcpy(stagingBufferData, data, sizeInBytes);
    stagingBuffer->unmapMemory();

    if (generateMipmaps && (imageSettings.memoryUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) {
        Logfile::get()->throwError(
                "Error in Image::uploadData: Generating mipmaps is requested, but VK_IMAGE_USAGE_TRANSFER_SRC_BIT "
                "is not set.");
    }

    copyBufferToImage(stagingBuffer);

    if (generateMipmaps) {
        _generateMipmaps();
    } else {
        transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void Image::copyBufferToImage(BufferPtr& buffer, VkCommandBuffer commandBuffer) {
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = { imageSettings.width, imageSettings.height, imageSettings.depth };

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer->getVkBuffer(),
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
    );

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::copyToBuffer(BufferPtr& buffer, VkCommandBuffer commandBuffer) {
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = { imageSettings.width, imageSettings.height, imageSettings.depth };

    vkCmdCopyImageToBuffer(
            commandBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            buffer->getVkBuffer(),
            1,
            &region
    );

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::blit(ImagePtr& destImage, VkCommandBuffer commandBuffer) {
    // Does the device support linear filtering for blit operations?
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(
            device->getVkPhysicalDevice(), imageSettings.format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        Logfile::get()->throwError(
                "Error in Image::blit: Texture image format does not support linear blitting!");
    }

    bool transientCommandBuffer = commandBuffer == VK_NULL_HANDLE;
    if (transientCommandBuffer) {
        commandBuffer = device->beginSingleTimeCommands();
    }

    destImage->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageBlit blit{};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = {
            int32_t(imageSettings.width), int32_t(imageSettings.height), int32_t(imageSettings.depth)
    };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = imageSettings.arrayLayers;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = {
            int32_t(destImage->getImageSettings().width),
            int32_t(destImage->getImageSettings().height),
            int32_t(destImage->getImageSettings().depth)
    };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = destImage->getImageSettings().arrayLayers;

    vkCmdBlitImage(commandBuffer,
                   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   destImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   VK_FILTER_LINEAR);

    destImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    this->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (transientCommandBuffer) {
        device->endSingleTimeCommands(commandBuffer);
    }
}

void Image::transitionImageLayout(VkImageLayout newLayout) {
    transitionImageLayout(imageLayout, newLayout);
}

void Image::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = imageSettings.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = imageSettings.arrayLayers;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(imageSettings.format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        barrier.srcAccessMask = 0;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        Logfile::get()->throwError("Error in Image::transitionImageLayout: Unsupported old layout!");
    }

    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        Logfile::get()->throwError("Error in Image::transitionImageLayout: Unsupported new layout!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0, // 0 or VK_DEPENDENCY_BY_REGION_BIT
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    device->endSingleTimeCommands(commandBuffer);
}

void Image::_generateMipmaps() {
    // Does the device support linear filtering for blit operations?
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(
            device->getVkPhysicalDevice(), imageSettings.format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        Logfile::get()->throwError(
                "Error in Image::_generateMipmaps: Texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = int32_t(imageSettings.width);
    int32_t mipHeight = int32_t(imageSettings.height);

    for (uint32_t i = 1; i < imageSettings.mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = imageSettings.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    device->endSingleTimeCommands(commandBuffer);
}

void* Image::mapMemory() {
    //if (imageSettings.tiling != VK_IMAGE_TILING_LINEAR) {
    //    Logfile::get()->throwError("Error in Image::mapMemory: Invalid tiling format for mapping.");
    //}

    void* dataPointer = nullptr;
    vmaMapMemory(device->getAllocator(), imageAllocation, &dataPointer);
    return dataPointer;
}

void Image::unmapMemory() {
    vmaUnmapMemory(device->getAllocator(), imageAllocation);
}

#ifdef SUPPORT_OPENGL
bool Image::createGlMemoryObject(GLuint& memoryObjectGl) {
    if (!imageSettings.exportMemory) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: An external memory object can only be created if the "
                "export memory flag was set on creation!");
        return false;
    }
    return createGlMemoryObjectFromVkDeviceMemory(
            memoryObjectGl, device->getVkDevice(), deviceMemory, deviceMemorySizeInBytes);
}
#endif



ImageView::ImageView(ImagePtr& image, VkImageViewType imageViewType, VkImageAspectFlags aspectFlags)
        : device(image->getDevice()), image(image), imageViewType(imageViewType), aspectFlags(aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image->getVkImage();
    viewInfo.viewType = imageViewType;
    viewInfo.format = image->getImageSettings().format;
    //viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    //viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    //viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    //viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = image->getImageSettings().mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = image->getImageSettings().arrayLayers;

    if (vkCreateImageView(device->getVkDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in ImageView::ImageView: vkCreateImageView failed!");
    }
}

ImageView::ImageView(ImagePtr& image, VkImageAspectFlags aspectFlags)
        : ImageView(image, VkImageViewType(image->getImageSettings().imageType), aspectFlags) {
}

ImageView::ImageView(
        ImagePtr& image, VkImageView imageView, VkImageViewType imageViewType, VkImageAspectFlags aspectFlags)
        : device(image->getDevice()), image(image), imageViewType(imageViewType), aspectFlags(aspectFlags) {
    this->imageView = imageView;
}

ImageView::~ImageView() {
    vkDestroyImageView(device->getVkDevice(), imageView, nullptr);
}

ImageViewPtr ImageView::copy(bool copyImage, bool copyContent) {
    ImagePtr newImage;
    if (copyImage) {
        newImage = image->copy(copyContent, aspectFlags);
    } else {
        newImage = image;
    }
    ImageViewPtr newImageView(new ImageView(newImage, imageViewType, aspectFlags));
    return newImageView;
}

ImageSampler::ImageSampler(Device* device, const ImageSamplerSettings& samplerSettings, float maxLodOverwrite)
        : device(device), imageSamplerSettings(samplerSettings) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = samplerSettings.magFilter;
    samplerInfo.minFilter = samplerSettings.minFilter;

    samplerInfo.addressModeU = samplerSettings.addressModeU;
    samplerInfo.addressModeV = samplerSettings.addressModeV;
    samplerInfo.addressModeW = samplerSettings.addressModeW;

    if (samplerSettings.anisotropyEnable && device->getPhysicalDeviceFeatures().samplerAnisotropy) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        if (samplerInfo.maxAnisotropy < 0.0f) {
            samplerInfo.maxAnisotropy = device->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
        } else {
            samplerInfo.maxAnisotropy = samplerSettings.maxAnisotropy;
        }
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }
    samplerInfo.borderColor = samplerSettings.borderColor;
    samplerInfo.unnormalizedCoordinates = samplerSettings.unnormalizedCoordinates;

    samplerInfo.compareEnable = samplerSettings.compareEnable;
    samplerInfo.compareOp = samplerSettings.compareOp;

    samplerInfo.mipmapMode = samplerSettings.mipmapMode;
    samplerInfo.mipLodBias = samplerSettings.mipLodBias;
    samplerInfo.minLod = samplerSettings.minLod;
    if (maxLodOverwrite < 0.0f) {
        samplerInfo.maxLod = maxLodOverwrite;
    } else {
        samplerInfo.maxLod = samplerSettings.maxLod;
    }

    if (vkCreateSampler(device->getVkDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in ImageSampler::ImageSampler: vkCreateSampler!");
    }
}

ImageSampler::ImageSampler(Device* device, const ImageSamplerSettings& samplerSettings, ImagePtr image)
        : ImageSampler(
                device, samplerSettings, float(
                image->getImageSettings().mipLevels <= 1 ? 0 : image->getImageSettings().mipLevels)) {
}

ImageSampler::~ImageSampler() {
    vkDestroySampler(device->getVkDevice(), sampler, nullptr);
}

Texture::Texture(ImageViewPtr& imageView, ImageSamplerPtr& imageSampler)
        : imageView(imageView), imageSampler(imageSampler) {
}

Texture::Texture(
        Device* device, const ImageSettings& imageSettings, const ImageSamplerSettings& samplerSettings,
        VkImageAspectFlags aspectFlags) {
    vk::ImagePtr image(new Image(device, imageSettings));
    imageView = vk::ImageViewPtr(new ImageView(image, aspectFlags));
    imageSampler = vk::ImageSamplerPtr(new ImageSampler(device, samplerSettings, image));
}

}}
