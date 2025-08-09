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

#ifndef SGL_VULKAN_INTEROPCUDA_HPP
#define SGL_VULKAN_INTEROPCUDA_HPP

#include "../Buffers/Buffer.hpp"
#include "../Image/Image.hpp"
#include "SyncObjects.hpp"
#include "Device.hpp"

#include <Graphics/Utils/InteropCuda.hpp>

#ifdef _WIN32
typedef void* HANDLE;
#endif

namespace sgl { namespace vk {

/**
 * Returns the closest matching CUDA device.
 * @param device The Vulkan device.
 * @param cuDevice The CUDA device (if true is returned).
 * @return Whether a matching CUDA device was found.
 */
DLL_OBJECT bool getMatchingCudaDevice(sgl::vk::Device* device, CUdevice* cuDevice);

/**
 * A CUDA driver API CUexternalSemaphore object created from a Vulkan semaphore.
 * Both binary and timeline semaphores are supported, but timeline semaphores require at least CUDA 11.2.
 */
class DLL_OBJECT SemaphoreVkCudaDriverApiInterop : public vk::Semaphore {
public:
    explicit SemaphoreVkCudaDriverApiInterop(
            Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0,
            VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY, uint64_t timelineSemaphoreInitialValue = 0);
    ~SemaphoreVkCudaDriverApiInterop() override;

    /// Signal semaphore.
    void signalSemaphoreCuda(CUstream stream, unsigned long long timelineValue = 0);

    /// Wait on semaphore.
    void waitSemaphoreCuda(CUstream stream, unsigned long long timelineValue = 0);

private:
    CUexternalSemaphore cuExternalSemaphore = {};
};

typedef std::shared_ptr<SemaphoreVkCudaDriverApiInterop> SemaphoreVkCudaDriverApiInteropPtr;


/**
 * A CUDA driver API CUdeviceptr object created from a Vulkan buffer.
 */
class DLL_OBJECT BufferCudaDriverApiExternalMemoryVk
{
public:
    explicit BufferCudaDriverApiExternalMemoryVk(vk::BufferPtr& vulkanBuffer);
    virtual ~BufferCudaDriverApiExternalMemoryVk();

    inline const sgl::vk::BufferPtr& getVulkanBuffer() { return vulkanBuffer; }
    [[nodiscard]] inline CUdeviceptr getCudaDevicePtr() const { return cudaDevicePtr; }

protected:
    sgl::vk::BufferPtr vulkanBuffer;
    CUexternalMemory cudaExternalMemoryBuffer{};
    CUdeviceptr cudaDevicePtr{};

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferCudaDriverApiExternalMemoryVk> BufferCudaDriverApiExternalMemoryVkPtr;
typedef BufferCudaDriverApiExternalMemoryVk BufferCudaExternalMemoryVk;
typedef std::shared_ptr<BufferCudaExternalMemoryVk> BufferCudaExternalMemoryVkPtr;


/**
 * A CUDA driver API CUmipmappedArray object created from a Vulkan image.
 */
class DLL_OBJECT ImageCudaExternalMemoryVk
{
public:
    explicit ImageCudaExternalMemoryVk(vk::ImagePtr& vulkanImage);
    ImageCudaExternalMemoryVk(
            vk::ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore);
    virtual ~ImageCudaExternalMemoryVk();

    inline const sgl::vk::ImagePtr& getVulkanImage() { return vulkanImage; }
    [[nodiscard]] inline CUmipmappedArray getCudaMipmappedArray() const { return cudaMipmappedArray; }
    CUarray getCudaMipmappedArrayLevel(uint32_t level = 0);

    /*
     * Asynchronous copy from a device pointer to level 0 mipmap level.
     */
    void memcpyCudaDtoA2DAsync(CUdeviceptr devicePtr, CUstream stream);
    void memcpyCudaDtoA3DAsync(CUdeviceptr devicePtr, CUstream stream);

protected:
    void _initialize(vk::ImagePtr& _vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore);

    sgl::vk::ImagePtr vulkanImage;
    CUexternalMemory cudaExternalMemoryBuffer{};
    CUmipmappedArray cudaMipmappedArray{};

    // Cache for storing the array for mipmap level 0.
    CUarray cudaArrayLevel0{};

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

DLL_OBJECT CUarray_format getCudaArrayFormatFromVkFormat(VkFormat format);
typedef std::shared_ptr<ImageCudaExternalMemoryVk> ImageCudaExternalMemoryVkPtr;
typedef ImageCudaExternalMemoryVk ImageCudaDriverApiExternalMemoryVk;
typedef std::shared_ptr<ImageCudaDriverApiExternalMemoryVk> ImageCudaDriverApiExternalMemoryVkPtr;

struct DLL_OBJECT TextureCudaExternalMemorySettings {
    // Use CUDA mipmapped array or CUDA array at level 0.
    bool useMipmappedArray = false;
    // Whether to use normalized coordinates in the range [0, 1) or integer coordinates in the range [0, dim).
    // NOTE: For CUDA mipmapped arrays, this value has to be set to true!
    bool useNormalizedCoordinates = false;
    // Whether to allow bilinear filtering in case it can closely approximate trilinear filtering.
    bool useTrilinearOptimization = true;
    // Whether to transform integer values to the range [0, 1] or not.
    bool readAsInteger = false;
};

class DLL_OBJECT TextureCudaExternalMemoryVk {
public:
    TextureCudaExternalMemoryVk(
            vk::TexturePtr& vulkanTexture, const TextureCudaExternalMemorySettings& texCudaSettings = {});
    TextureCudaExternalMemoryVk(
            vk::ImagePtr& vulkanImage, const ImageSamplerSettings& samplerSettings,
            const TextureCudaExternalMemorySettings& texCudaSettings = {});
    TextureCudaExternalMemoryVk(
            vk::ImagePtr& vulkanImage, const ImageSamplerSettings& samplerSettings,
            VkImageViewType imageViewType, const TextureCudaExternalMemorySettings& texCudaSettings = {});
    TextureCudaExternalMemoryVk(
            vk::ImagePtr& vulkanImage, const ImageSamplerSettings& samplerSettings,
            VkImageViewType imageViewType, VkImageSubresourceRange imageSubresourceRange,
            const TextureCudaExternalMemorySettings& texCudaSettings = {});
    ~TextureCudaExternalMemoryVk();

    [[nodiscard]] inline CUtexObject getCudaTextureObject() const { return cudaTextureObject; }
    inline const sgl::vk::ImagePtr& getVulkanImage() { return imageCudaExternalMemory->getVulkanImage(); }
    inline const ImageCudaExternalMemoryVkPtr& getImageCudaExternalMemory() { return imageCudaExternalMemory; }

protected:
    CUtexObject cudaTextureObject{};
    ImageCudaExternalMemoryVkPtr imageCudaExternalMemory;
};

typedef std::shared_ptr<TextureCudaExternalMemoryVk> TextureCudaExternalMemoryVkPtr;

class DLL_OBJECT SurfaceCudaExternalMemoryVk {
public:
    SurfaceCudaExternalMemoryVk(vk::ImagePtr& vulkanImageView, VkImageViewType imageViewType);
    explicit SurfaceCudaExternalMemoryVk(vk::ImageViewPtr& vulkanImageView);
    ~SurfaceCudaExternalMemoryVk();

    [[nodiscard]] inline CUtexObject getCudaSurfaceObject() const { return cudaSurfaceObject; }
    inline const sgl::vk::ImagePtr& getVulkanImage() { return imageCudaExternalMemory->getVulkanImage(); }

protected:
    CUsurfObject cudaSurfaceObject{};
    ImageCudaExternalMemoryVkPtr imageCudaExternalMemory;
};

typedef std::shared_ptr<SurfaceCudaExternalMemoryVk> SurfaceCudaExternalMemoryVkPtr;

}}

#endif //SGL_VULKAN_INTEROPCUDA_HPP
