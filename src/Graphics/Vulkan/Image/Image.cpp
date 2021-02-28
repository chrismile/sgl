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

#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "Image.hpp"

namespace sgl { namespace vk {

Image::Image(Device* device, ImageSettings imageSettings) : device(device), imageSettings(imageSettings) {
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

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = imageSettings.memoryUsage;

    VmaAllocationInfo textureImageAllocationInfo;
    if (vmaCreateImage(
            device->getAllocator(), &imageCreateInfo, &allocCreateInfo, &image,
            &imageAllocation, &textureImageAllocationInfo) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Image::Image: vmaCreateImage failed!");
    }
}

Image::Image(Device* device, ImageSettings imageSettings, VkImage image, bool takeImageOwnership)
    : device(device), hasImageOwnership(takeImageOwnership), imageSettings(imageSettings), image(image) {
}

Image::Image(
        Device* device, ImageSettings imageSettings, VkImage image,
        VmaAllocation imageAllocation, VmaAllocationInfo imageAllocationInfo)
        : device(device), imageSettings(imageSettings), image(image), imageAllocation(imageAllocation),
        imageAllocationInfo(imageAllocationInfo) {
    this->imageSettings = imageSettings;
    this->image = image;
    this->imageAllocation = imageAllocation;
    this->imageAllocationInfo = imageAllocationInfo;
}

Image::~Image() {
    if (hasImageOwnership) {
        vmaDestroyImage(device->getAllocator(), image, imageAllocation);
    }
}

ImageView::ImageView(Device* device, ImagePtr image, VkImageAspectFlags aspectFlags) : device(device), image(image) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image->getVkImage();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
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

ImageView::ImageView(Device* device, ImagePtr image, VkImageView imageView, VkImageAspectFlags aspectFlags)
        : device(device), image(image) {
    this->imageView = imageView;
}

ImageView::~ImageView() {
    vkDestroyImageView(device->getVkDevice(), imageView, nullptr);
}

ImageSampler::ImageSampler(Device* device, ImageSamplerSettings samplerSettings, uint32_t maxLod)
        : device(device) {
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
    if (samplerInfo.maxLod < 0.0f) {
        samplerInfo.maxLod = static_cast<float>(maxLod);
    } else {
        samplerInfo.maxLod = samplerSettings.maxLod;
    }

    if (vkCreateSampler(device->getVkDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in ImageSampler::ImageSampler: vkCreateSampler!");
    }
}

ImageSampler::ImageSampler(Device* device, ImageSamplerSettings samplerSettings, ImagePtr image)
        : ImageSampler(device, samplerSettings, image->getImageSettings().mipLevels) {}

ImageSampler::~ImageSampler() {
    vkDestroySampler(device->getVkDevice(), sampler, nullptr);
}

Texture::Texture(ImagePtr image, ImageViewPtr imageView, ImageSampler imageSampler)
        : image(image), imageView(imageView), imageSampler(imageSampler) {
}

}}
