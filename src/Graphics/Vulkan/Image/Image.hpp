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
    Image(Device* device, ImageSettings imageSettings);
    Image(Device* device, ImageSettings imageSettings, VkImage image, bool takeImageOwnership = true);
    Image(
            Device* device, ImageSettings imageSettings, VkImage image,
            VmaAllocation imageAllocation, VmaAllocationInfo imageAllocationInfo);
    ~Image();

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

    /// Transitions the image layout from the old
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

private:
    void _generateMipmaps();
    void _copyBufferToImage(VkBuffer buffer);

    Device* device = nullptr;
    bool hasImageOwnership = true;
    ImageSettings imageSettings;
    VkImage image = nullptr;
    VmaAllocation imageAllocation = nullptr;
    VmaAllocationInfo imageAllocationInfo = {};
};

typedef std::shared_ptr<Image> ImagePtr;

class DLL_OBJECT ImageView {
public:
    ImageView(Device* device, ImagePtr image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ImageView(
            Device* device, ImagePtr image, VkImageView imageView,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ~ImageView();

    inline ImagePtr& getImage() { return image; }
    inline VkImageView getVkImageView() { return imageView; }

private:
    Device* device = nullptr;
    ImagePtr image;
    VkImageView imageView = nullptr;
};

typedef std::shared_ptr<ImageView> ImageViewPtr;

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
    float maxLod = -1.0f; // Negative value means max. LoD.
    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    // [0, 1) range vs. [0, size) range.
    VkBool32 unnormalizedCoordinates = VK_FALSE;
};

/**
 * A texture sampler wrapper that lets images be used for sampling in a shader.
 */
class DLL_OBJECT ImageSampler {
public:
    ImageSampler(Device* device, ImageSamplerSettings samplerSettings, uint32_t maxLod = 0);
    ImageSampler(Device* device, ImageSamplerSettings samplerSettings, ImagePtr image);
    ~ImageSampler();

private:
    Device* device = nullptr;
    VkSampler sampler = nullptr;
};

typedef std::shared_ptr<ImageSampler> ImageSamplerPtr;

class DLL_OBJECT Texture {
public:
    Texture(ImagePtr image, ImageViewPtr imageView, ImageSampler imageSampler);

private:
    ImagePtr image;
    ImageViewPtr imageView;
    ImageSampler imageSampler;
};

typedef std::shared_ptr<Texture> TexturePtr;

}}

#endif //SGL_IMAGE_HPP
