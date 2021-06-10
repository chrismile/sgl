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

#ifndef SGL_IMAGE_HPP
#define SGL_IMAGE_HPP

#include <memory>
#include <cmath>

#include <vulkan/vulkan.h>
#include "../libs/VMA/vk_mem_alloc.h"

namespace sgl { namespace vk {

class Device;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

class Image;
typedef std::shared_ptr<Image> ImagePtr;
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class ImageSampler;
typedef std::shared_ptr<ImageSampler> ImageSamplerPtr;
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

struct DLL_OBJECT ImageSettings {
    ImageSettings() {}
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    VkImageType imageType = VK_IMAGE_TYPE_2D;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_SRGB
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_OPTIMAL; VK_IMAGE_TILING_LINEAR -> row-major
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
};

inline bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

class DLL_OBJECT Image {
public:
    Image(Device* device, const ImageSettings& imageSettings);
    Image(Device* device, const ImageSettings& imageSettings, VkImage image, bool takeImageOwnership = true);
    Image(
            Device* device, const ImageSettings& imageSettings, VkImage image,
            VmaAllocation imageAllocation, VmaAllocationInfo imageAllocationInfo);
    ~Image();

    /**
     * Creates a copy of the buffer.
     * @param copyContent Whether to copy the content, too, or only create a buffer with the same settings.
     * @param aspectFlags The image aspect flags.
     * @return The new buffer.
     */
    ImagePtr copy(bool copyContent, VkImageAspectFlags aspectFlags);

    // Computes the recommended number of mip levels for a 2D texture of the specified size.
    static uint32_t computeMipLevels(uint32_t width, uint32_t height) {
        return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    }

    inline VkImage getVkImage() { return image; }
    inline const ImageSettings& getImageSettings() { return imageSettings; }

    /**
     * Uploads the data to the GPU memory of the texture using a stating buffer.
     * @param sizeInBytes The size of the data to upload in bytes.
     * @param data A pointer to the data on the CPU.
     * @param generateMipmaps Whether to generate mipmaps (if imageSettings.mipLevels > 1).
     * NOTE: If generateMipmaps is true, VK_IMAGE_USAGE_TRANSFER_SRC_BIT must be specified in ImageSettings::usage.
     * Please also note that using this command only really makes sense if the texture was created with the mode
     * VMA_MEMORY_USAGE_GPU_ONLY.
     */
    void uploadData(VkDeviceSize sizeInBytes, void* data, bool generateMipmaps = true);

    /**
     * Copies the content of a buffer to this image.
     * @param buffer The copy source.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyBufferToImage(BufferPtr& buffer, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of the image to the specified buffer.
     * @param buffer The copy destination.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyToBuffer(BufferPtr& buffer, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Blits the content of this image to a destination image and performs format conversion if necessary.
     * @param destImage The destination image.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void blit(ImagePtr& destImage, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /// Transitions the image layout from the current layout to the new layout.
    void transitionImageLayout(VkImageLayout newLayout);

    /// Transitions the image layout from the old layout to the new layout.
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

    /// Transitions the image layout from the old layout to the new layout.
    inline VkImageLayout getVkImageLayout() const { return imageLayout; }

    inline Device* getDevice() { return device; }

    void* mapMemory();
    void unmapMemory();

private:
    void _generateMipmaps();

    Device* device = nullptr;
    bool hasImageOwnership = true;
    ImageSettings imageSettings;
    VkImage image = nullptr;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocation imageAllocation = nullptr;
    VmaAllocationInfo imageAllocationInfo = {};
};

class DLL_OBJECT ImageView {
public:
    ImageView(
            ImagePtr& image, VkImageViewType imageViewType, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    explicit ImageView(
            ImagePtr& image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ImageView(
            ImagePtr& image, VkImageView imageView, VkImageViewType imageViewType,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ~ImageView();

    /**
     * Creates a copy of the image view.
     * @param copyImage Whether to create a deep copy (with the underlying image also being copied) or to create a
     * shallow copy that shares the image object with the original image view.
     * @param copyContent If copyImage is true: Whether to also copy the content of the underlying image.
     * @return The new image view.
     */
    ImageViewPtr copy(bool copyImage, bool copyContent);

    inline Device* getDevice() { return device; }
    inline ImagePtr& getImage() { return image; }
    inline VkImageView getVkImageView() { return imageView; }
    inline VkImageAspectFlags getVkImageAspectFlags() { return aspectFlags; }

private:
    Device* device = nullptr;
    ImagePtr image;
    VkImageView imageView = nullptr;
    VkImageViewType imageViewType;
    VkImageAspectFlags aspectFlags;
};

struct DLL_OBJECT ImageSamplerSettings {
    ImageSamplerSettings() {}
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    /*
     * VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
     * VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
     * VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
     */
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    float mipLodBias = 0.0f;
    VkBool32 anisotropyEnable = VK_FALSE;
    float maxAnisotropy = -1.0f; // Negative value means max. anisotropy.
    VkBool32 compareEnable = VK_FALSE; // VK_TRUE, e.g., for percentage-closer filtering
    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
    float minLod = 0.0f;
    float maxLod = VK_LOD_CLAMP_NONE;
    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    // [0, 1) range vs. [0, size) range.
    VkBool32 unnormalizedCoordinates = VK_FALSE;
};

/**
 * A texture sampler wrapper that lets images be used for sampling in a shader.
 */
class DLL_OBJECT ImageSampler {
public:
    ImageSampler(Device* device, const ImageSamplerSettings& samplerSettings, float maxLodOverwrite = -1.0f);
    ImageSampler(Device* device, const ImageSamplerSettings& samplerSettings, ImagePtr image);
    ~ImageSampler();

    inline VkSampler getVkSampler() { return sampler; }

private:
    Device* device = nullptr;
    VkSampler sampler = nullptr;
};

class DLL_OBJECT Texture {
public:
    Texture(ImageViewPtr& imageView, ImageSamplerPtr& imageSampler);
    Texture(
            Device* device, const ImageSettings& imageSettings,
            const ImageSamplerSettings& samplerSettings = ImageSamplerSettings(),
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

    inline ImagePtr& getImage() { return imageView->getImage(); }
    inline ImageViewPtr& getImageView() { return imageView; }
    inline ImageSamplerPtr& getImageSampler() { return imageSampler; }

private:
    ImageViewPtr imageView;
    ImageSamplerPtr imageSampler;
};

}}

#endif //SGL_IMAGE_HPP
