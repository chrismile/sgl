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

#include <string>
#include <memory>
#include <algorithm>
#include <cmath>

#ifdef USE_GLM
#include <glm/vec4.hpp>
#else
#include <Math/Geometry/fallback/vec4.hpp>
#endif

#include <Defs.hpp>
#include "../libs/volk/volk.h"
#include "../libs/VMA/vk_mem_alloc.h"

#ifdef _WIN32
typedef void* HANDLE;
#endif

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
typedef unsigned int GLuint;
namespace sgl {
union InteropMemoryHandle;
}
#endif

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
    ImageSettings() = default;
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
    uint32_t queueFamilyIndexCount = 0; // Only for sharingMode == VK_SHARING_MODE_CONCURRENT.
    const uint32_t* pQueueFamilyIndices = nullptr;
    VkImageCreateFlags flags = 0;
    bool exportMemory = false; // Whether to export the memory for external use, e.g., in OpenGL.
    /**
     * Whether to use a dedicated allocation instead of using VMA. At the moment, this is only supported for exported
     * memory. When not using dedicated allocations, multiple images may share one block of VkDeviceMemory.
     * At the moment, some APIs (like OpenCL) may not support creating buffers with memory offsets when not using
     * sub-buffers.
     */
    bool useDedicatedAllocationForExportedMemory = true;
    // Can be used for specifying compatible image view formats (VK_KHR_image_format_list).
    uint32_t viewFormatCount = 0;
    const VkFormat* pViewFormats = nullptr;
};

inline bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
inline bool isDepthStencilFormat(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_X8_D24_UNORM_PACK32
           || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT
           || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}
DLL_OBJECT size_t getImageFormatEntryByteSize(VkFormat format);

DLL_OBJECT size_t getImageFormatNumChannels(VkFormat format);

// Returns the GLSL format specifier of the format, e.g., "rgba32f" or "r16".
DLL_OBJECT std::string getImageFormatGlslString(VkFormat format);

// Converts the VkFormat entry to a string representation.
DLL_OBJECT std::string convertVkFormatToString(VkFormat format);

// Converts the VkFormatFeatureFlags flags to a string representation.
DLL_OBJECT std::string convertVkFormatFeatureFlagsToString(VkFormatFeatureFlags flags);

class DLL_OBJECT Image {
    friend class Renderer;
public:
    Image(Device* device, const ImageSettings& imageSettings);
    Image(Device* device, const ImageSettings& imageSettings, VkImage image, bool takeImageOwnership = true);
    Image(
            Device* device, const ImageSettings& imageSettings, VkImage image,
            VmaAllocation imageAllocation, VmaAllocationInfo imageAllocationInfo);
    /**
     * Does not allocate any memory and image. This constructor is mainly needed when later calling
     * @see createFromD3D12SharedResourceHandle.
     */
    explicit Image(Device* device) : device(device) {}
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
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     *
     * NOTE: If generateMipmaps is true, VK_IMAGE_USAGE_TRANSFER_SRC_BIT must be specified in ImageSettings::usage.
     * Please also note that using this command only really makes sense if the texture was created with the mode
     * VMA_MEMORY_USAGE_GPU_ONLY.
     */
    void uploadData(
            VkDeviceSize sizeInBytes, const void* data, bool generateMipmaps = true,
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Generates mipmaps for the image (if imageSettings.mipLevels > 1).
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     * NOTE: VK_IMAGE_USAGE_TRANSFER_SRC_BIT must be specified in ImageSettings::usage.
     */
    void generateMipmaps(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of a buffer to this image.
     * @param buffer The copy source.
     * @param aspectMask The aspect mask flags for the copy operation.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyFromBuffer(
            BufferPtr& buffer, VkImageAspectFlags aspectMask, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of a buffer to this image.
     * @param buffer The copy source.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyFromBuffer(BufferPtr& buffer, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of a buffer to an array layer of this image.
     * @param buffer The copy source.
     * @param baseArrayLayer The array layer of the image to copy the content of the buffer to.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyFromBufferLayered(
            BufferPtr& buffer, uint32_t baseArrayLayer, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of the image to the specified buffer.
     * @param buffer The copy destination.
     * @param aspectMask The aspect mask flags for the copy operation.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyToBuffer(BufferPtr& buffer, VkImageAspectFlags aspectMask, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of the image to the specified buffer.
     * @param buffer The copy destination.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyToBuffer(BufferPtr& buffer, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of the image to the specified buffer.
     * @param buffer The copy destination.
     * @param baseArrayLayer The array layer of the image to copy the content of the buffer to.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void copyToBufferLayered(
            BufferPtr& buffer, uint32_t baseArrayLayer, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of the image to the specified image.
     * @param destImage The destination image.
     * @param aspectFlags The image aspect flags.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     * NOTE: This operation
     */
    void copyToImage(
            ImagePtr& destImage, VkImageAspectFlags aspectFlags, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Copies the content of the image to the specified image.
     * @param destImage The destination image.
     * @param srcAspectFlags The source image aspect flags.
     * @param destAspectFlags The destination image aspect flags.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     * NOTE: This operation
     */
    void copyToImage(
            ImagePtr& destImage, VkImageAspectFlags srcAspectFlags, VkImageAspectFlags destAspectFlags,
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /**
     * Blits the content of this image to a destination image and performs format conversion if necessary.
     * @param destImage The destination image.
     * @param commandBuffer The command buffer. If VK_NULL_HANDLE is specified, a transient command buffer is used and
     * the function will wait with vkQueueWaitIdle for the command to finish on the GPU.
     */
    void blit(ImagePtr& destImage, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    void clearColor(
            const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
    void clearColor(
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
            const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
    void clearDepthStencil(
            VkImageAspectFlags aspectFlags, float clearDepth = 1.0f, uint32_t clearStencil = 0,
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
    void clearDepthStencil(
            VkImageAspectFlags aspectFlags,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
            float clearDepth = 1.0f, uint32_t clearStencil = 0, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /// Transitions the image layout from the current layout to the new layout.
    void transitionImageLayout(VkImageLayout newLayout);
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void transitionImageLayout(VkImageLayout newLayout, VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);
    void transitionImageLayoutEx(
            VkImageLayout newLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccessMask,
            VkCommandBuffer commandBuffer);
    void insertMemoryBarrier(
            VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
    void transitionImageLayoutSubresource(
            VkImageLayout newLayout,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void transitionImageLayoutSubresource(
            VkImageLayout oldLayout, VkImageLayout newLayout,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void transitionImageLayoutSubresource(
            VkImageLayout newLayout, VkCommandBuffer commandBuffer,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void transitionImageLayoutSubresource(
            VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
    void insertMemoryBarrierSubresource(
            VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

    /// The subresource layout contains information necessary when accessing the image data on the CPU (e.g., stride).
    [[nodiscard]] VkSubresourceLayout getSubresourceLayout(
            VkImageAspectFlags aspectFlags, uint32_t mipLevel = 0, uint32_t arrayLayer = 0) const;

    /// Transitions the image layout from the old layout to the new layout.
    [[nodiscard]] inline VkImageLayout getVkImageLayout() const { return imageLayout; }
    /// Overwrites the image layout if it was changed outside of sgl.
    void overwriteImageLayout(VkImageLayout newLayout);
    /// To avoid automatic vertex/fragment shader layout transitions
    void setUseInComputeQueueTemp(bool _usedInCompute) { usedInComputeQueueTemp = _usedInCompute; }

    /// For access from the framebuffer after a subpass has finished.
    inline void _updateLayout(VkImageLayout newLayout) { imageLayout = newLayout; }

    [[nodiscard]] inline Device* getDevice() { return device; }
    [[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() { return deviceMemory; }
    [[nodiscard]] inline VkDeviceSize getDeviceMemoryOffset() const { return deviceMemoryOffset; }
    [[nodiscard]] inline VkDeviceSize getDeviceMemorySize() const { return deviceMemorySize; }
    [[nodiscard]] inline VkDeviceSize getDeviceMemoryAllocationSize() const { return deviceMemoryAllocationSize; }

    void* mapMemory();
    void unmapMemory();

#if defined(SUPPORT_OPENGL) && defined(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
    /**
     * Creates an OpenGL memory object from the external Vulkan memory.
     * NOTE: The image must have been created with exportMemory set to true.
     * @param memoryObjectGl The OpenGL memory object.
     * @param interopMemoryHandle The handle (Windows) or file descriptor (Unix) to the Vulkan memory object.
     * @return Whether the OpenGL memory object could be created successfully.
     */
    bool createGlMemoryObject(GLuint& memoryObjectGl, InteropMemoryHandle& interopMemoryHandle);
#endif

#ifdef _WIN32
    /**
     * Imports a Direct3D 12 shared resource handle. The object must have been created using the standard constructor.
     * The handle is expected to point to device-local memory. The object will take ownership of the handle and close
     * it on destruction. Below, an example of how to create the shared handle can be found.
     *
     * HANDLE resourceHandle;
     * std::wstring sharedHandleNameString = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
     * ID3D12Device::CreateSharedHandle(dxObject, nullptr, GENERIC_ALL, sharedHandleNameString.data(), &resourceHandle);
     *
     * If the passed handle name already exists, the function will fail with DXGI_ERROR_NAME_ALREADY_EXISTS.
     *
     * In order to query the resource settings of a Direct3D 12 image, ID3D12Resource::GetDesc can be used. Example:
     * ComPtr<ID3D12Resource> dxImage;
     * D3D12_RESOURCE_DESC imageResourceDesc = dxImage->GetDesc();
     * imageSettings.width = imageResourceDesc.Width;
     * imageSettings.height = imageResourceDesc.Height;
     * ...
     */
    void createFromD3D12SharedResourceHandle(HANDLE resourceHandle, const ImageSettings& imageSettings);
#endif

private:
    void _generateMipmaps(VkCommandBuffer commandBuffer);

    Device* device = nullptr;
    bool hasImageOwnership = true;
    ImageSettings imageSettings;
    VkImage image = nullptr;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool usedInComputeQueueTemp = false; ///< To avoid automatic vertex/fragment shader layout transitions.

    // For looking up and caching the properties of the currently used format.
    VkFormat cachedFormat{};
    VkFormatProperties formatProperties{};

    // Memory not exported, used only in Vulkan.
    VmaAllocation imageAllocation = nullptr;
    VmaAllocationInfo imageAllocationInfo = {};

    // Exported memory for external use.
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    VkDeviceSize deviceMemorySize = 0;
    VkDeviceSize deviceMemoryOffset = 0;
    VkDeviceSize deviceMemoryAllocationSize = 0;
    bool isDedicatedAllocation = false;

#ifdef _WIN32
    HANDLE handle = nullptr;
#endif
};

struct ImageViewSettings {
    ImageViewSettings() {
        imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresourceRange.baseMipLevel = 0;
        imageSubresourceRange.levelCount = std::numeric_limits<uint32_t>::max(); // by default set to maximum image level count.
        imageSubresourceRange.baseArrayLayer = 0;
        imageSubresourceRange.layerCount = std::numeric_limits<uint32_t>::max(); // by default set to maximum image layer count.
        componentMapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        componentMapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        componentMapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        componentMapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    }
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM; // by default inferred from image type.
    VkFormat format = VK_FORMAT_UNDEFINED; // by default mapped to image format.
    VkImageSubresourceRange imageSubresourceRange;
    VkComponentMapping componentMapping;
    VkImageUsageFlags usage = 0; // by default inferred from image.
};

class DLL_OBJECT ImageView {
public:
    ImageView(const ImagePtr& image, const ImageViewSettings& imageViewSettings);
    ImageView(
            const ImagePtr& image, VkImageViewType imageViewType,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ImageView(
            const ImagePtr& image, VkImageViewType imageViewType,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    explicit ImageView(
            const ImagePtr& image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ImageView(
            const ImagePtr& image, VkImageView imageView, VkImageViewType imageViewType,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    ImageView(
            const ImagePtr& image, VkImageView imageView, VkImageViewType imageViewType,
            VkImageSubresourceRange range);
    ~ImageView();

    /**
     * Creates a copy of the image view.
     * @param copyImage Whether to create a deep copy (with the underlying image also being copied) or to create a
     * shallow copy that shares the image object with the original image view.
     * @param copyContent If copyImage is true: Whether to also copy the content of the underlying image.
     * @return The new image view.
     */
    ImageViewPtr copy(bool copyImage, bool copyContent);

    void clearColor(
            const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
    void clearDepthStencil(
            float clearDepth = 1.0f, uint32_t clearStencil = 0, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

    /// Transitions the image layout from the current layout to the new layout for the subresource.
    void transitionImageLayout(VkImageLayout newLayout);
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void transitionImageLayout(VkImageLayout newLayout, VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);
    void transitionImageLayoutEx(
            VkImageLayout newLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccessMask,
            VkCommandBuffer commandBuffer);
    void insertMemoryBarrier(
            VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

    inline Device* getDevice() { return device; }
    inline ImagePtr& getImage() { return image; }
    inline VkImageView getVkImageView() { return imageView; }
    [[nodiscard]] inline VkImageViewType getVkImageViewType() const { return imageViewType; }
    [[nodiscard]] inline VkImageAspectFlags getVkImageAspectFlags() const { return subresourceRange.aspectMask; }
    [[nodiscard]] inline VkImageSubresourceRange getVkImageSubresourceRange() const { return subresourceRange; }

private:
    Device* device = nullptr;
    ImagePtr image;
    VkImageView imageView = nullptr;
    VkImageViewType imageViewType;
    VkImageSubresourceRange subresourceRange{};
};

struct DLL_OBJECT ImageSamplerSettings {
    ImageSamplerSettings() = default;

    /// Sets filter modes to NEAREST if an integer format is used.
    ImageSamplerSettings(const ImageSettings& imageSettings);

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

    [[nodiscard]] inline const ImageSamplerSettings& getImageSamplerSettings() const { return imageSamplerSettings; }
    inline VkSampler getVkSampler() { return sampler; }

private:
    Device* device = nullptr;
    ImageSamplerSettings imageSamplerSettings;
    VkSampler sampler = nullptr;
};

class DLL_OBJECT Texture {
public:
    Texture(const ImageViewPtr& imageView, const ImageSamplerPtr& imageSampler);
    Texture(
            Device* device, const ImageSettings& imageSettings,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    Texture(
            Device* device, const ImageSettings& imageSettings, VkImageViewType imageViewType,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    Texture(const ImageViewPtr& imageView);
    Texture(
            Device* device, const ImageSettings& imageSettings,
            const ImageSamplerSettings& samplerSettings,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    Texture(
            Device* device, const ImageSettings& imageSettings, VkImageViewType imageViewType,
            const ImageSamplerSettings& samplerSettings,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    Texture(const ImageViewPtr& imageView, const ImageSamplerSettings& samplerSettings);

    inline ImagePtr& getImage() { return imageView->getImage(); }
    inline ImageViewPtr& getImageView() { return imageView; }
    inline ImageSamplerPtr& getImageSampler() { return imageSampler; }

private:
    ImageViewPtr imageView;
    ImageSamplerPtr imageSampler;
};

}}

#endif //SGL_IMAGE_HPP
