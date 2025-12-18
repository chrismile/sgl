/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#ifndef SGL_VULKAN_INTEROP_COMPUTE_HPP
#define SGL_VULKAN_INTEROP_COMPUTE_HPP

#include <utility>

#include <Graphics/Utils/InteropCompute.hpp>
#include "../Buffers/Buffer.hpp"
#include "../Image/Image.hpp"
#include "SyncObjects.hpp"
#include "Device.hpp"

/*
 * This file provides wrappers over InteropCuda, InteropHIP and InteropLevelZero.
 * Depending on what interop API has been initialized, CUDA, HIP, Level Zero or SYCL objects, semaphores, memory, etc.
 * are used internally.
 *
 * Documentation of SYCL side code:
 * https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
 */

namespace sgl { namespace vk {

/// Decides the compute API usable for the passed device. SYCL has precedence over other APIs if available.
DLL_OBJECT InteropComputeApi decideInteropComputeApi(Device* device);

/**
 * A CUDA driver API CUexternalSemaphore/HIP driver API hipExternalSemaphore_t object created from a Vulkan semaphore.
 * Both binary and timeline semaphores are supported, but timeline semaphores require at least CUDA 11.2.
 */
class DLL_OBJECT SemaphoreVkComputeApiInterop : public vk::Semaphore {
public:
    SemaphoreVkComputeApiInterop() = default;
    void initialize(
            Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
            VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue);
    ~SemaphoreVkComputeApiInterop() override = default;

    /// Signal semaphore.
    void signalSemaphoreComputeApi(StreamWrapper stream) {
        signalSemaphoreComputeApi(stream, 0, nullptr, nullptr);
    }
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
        signalSemaphoreComputeApi(stream, timelineValue, nullptr, nullptr);
    }
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
        signalSemaphoreComputeApi(stream, timelineValue, nullptr, eventOut);
    }
    virtual void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) = 0;

    /// Wait on semaphore.
    void waitSemaphoreComputeApi(StreamWrapper stream) {
        waitSemaphoreComputeApi(stream, 0, nullptr, nullptr);
    }
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue) {
        waitSemaphoreComputeApi(stream, timelineValue, nullptr, nullptr);
    }
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventOut) {
        waitSemaphoreComputeApi(stream, timelineValue, nullptr, eventOut);
    }
    virtual void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) = 0;

protected:
    virtual void preCheckExternalSemaphoreImport() {}
#ifdef _WIN32
    virtual void setExternalSemaphoreWin32Handle(HANDLE handle) = 0;
#endif
#ifdef __linux__
    virtual void setExternalSemaphoreFd(int fd) = 0;
#endif
    virtual void importExternalSemaphore() {}
};

typedef std::shared_ptr<SemaphoreVkComputeApiInterop> SemaphoreVkComputeApiInteropPtr;

DLL_OBJECT SemaphoreVkComputeApiInteropPtr createSemaphoreVkComputeApiInterop(
        Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0,
        VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY, uint64_t timelineSemaphoreInitialValue = 0);


/**
 * A CUDA driver API CUexternalMemory/hipExternalMemory_t object created from a Vulkan buffer.
 */
class DLL_OBJECT BufferVkComputeApiExternalMemory {
public:
    BufferVkComputeApiExternalMemory() = default;
    void initialize(vk::BufferPtr& vulkanBuffer);
    virtual ~BufferVkComputeApiExternalMemory() = default;

    inline const sgl::vk::BufferPtr& getVulkanBuffer() { return vulkanBuffer; }

    template<class T>
    [[nodiscard]] inline T* getDevicePtr() const { return reinterpret_cast<T*>(devicePtr); }
    template<class T>
    [[nodiscard]] inline T getDevicePtrReinterpreted() const { return reinterpret_cast<T>(devicePtr); }

    virtual void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;

protected:
    virtual void preCheckExternalMemoryImport() {}
#ifdef _WIN32
    virtual void setExternalMemoryWin32Handle(HANDLE handle) = 0;
#endif
#ifdef __linux__
    virtual void setExternalMemoryFd(int fileDescriptor) = 0;
#endif
    virtual void importExternalMemory() {}
    virtual void free() = 0;
    void freeHandlesAndFds();

    sgl::vk::BufferPtr vulkanBuffer;
    VkMemoryRequirements memoryRequirements{};
    void* devicePtr{}; // CUdeviceptr or hipDeviceptr_t or void* device pointer

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferVkComputeApiExternalMemory> BufferVkComputeApiExternalMemoryPtr;

DLL_OBJECT BufferVkComputeApiExternalMemoryPtr createBufferVkComputeApiExternalMemory(vk::BufferPtr& vulkanBuffer);


struct DLL_OBJECT ImageVkComputeApiInfo {
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    VkImageSubresourceRange imageSubresourceRange{};

    // Only needed for CUDA.
    bool surfaceLoadStore = false;

    // Only needed for Level Zero.
    bool useSampledImage = false;
    sgl::vk::ImageSamplerSettings imageSamplerSettings{};
    TextureExternalMemorySettings textureExternalMemorySettings{};
};

/**
 * A CUDA driver API CUexternalMemory + CUarray/hipExternalMemory_t + hipArray_t object created from a Vulkan image.
 */
class DLL_OBJECT ImageVkComputeApiExternalMemory {
public:
    ImageVkComputeApiExternalMemory() = default;
    void initialize(vk::ImagePtr& vulkanImage);
    void initialize(vk::ImagePtr& vulkanImage, const ImageVkComputeApiInfo& _imageComputeApiInfo);
    virtual ~ImageVkComputeApiExternalMemory() = default;

    inline const sgl::vk::ImagePtr& getVulkanImage() { return vulkanImage; }
    inline const ImageVkComputeApiInfo& getImageComputeApiInfo() { return imageComputeApiInfo; }

    /*
     * Asynchronous copy from/to a device pointer to level 0 mipmap level.
     */
    virtual void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) = 0;
    virtual void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) = 0;

protected:
    virtual void preCheckExternalMemoryImport() {}
#ifdef _WIN32
    virtual void setExternalMemoryWin32Handle(HANDLE handle) = 0;
#endif
#ifdef __linux__
    virtual void setExternalMemoryFd(int fd) = 0;
#endif
    virtual void importExternalMemory() {}
    virtual void free() = 0;
    void freeHandlesAndFds();

    sgl::vk::ImagePtr vulkanImage;
    ImageVkComputeApiInfo imageComputeApiInfo{};
    VkMemoryRequirements memoryRequirements{};
    void* mipmappedArray{}; // CUmipmappedArray or hipMipmappedArray_t or ze_image_handle_t or SyclImageMemHandleWrapper (image_mem_handle)

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<ImageVkComputeApiExternalMemory> ImageVkComputeApiExternalMemoryPtr;

DLL_OBJECT ImageVkComputeApiExternalMemoryPtr createImageVkComputeApiExternalMemory(vk::ImagePtr& vulkanImage);
DLL_OBJECT ImageVkComputeApiExternalMemoryPtr createImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage, const ImageVkComputeApiInfo& imageComputeApiInfo);


/**
 * An unsampled image.
 */
class DLL_OBJECT UnsampledImageVkComputeApiExternalMemory {
public:
    UnsampledImageVkComputeApiExternalMemory() = default;
    virtual void initialize(const ImageVkComputeApiExternalMemoryPtr& _image) = 0;
    virtual ~UnsampledImageVkComputeApiExternalMemory() = default;

    inline const sgl::vk::ImagePtr& getVulkanImage() { return image->getVulkanImage(); }

    /*
     * Asynchronous copy from/to a device pointer to level 0 mipmap level.
     */
    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyFromDevicePtrAsync(devicePtrSrc, stream, eventOut);
    }
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyToDevicePtrAsync(devicePtrDst, stream, eventOut);
    }

protected:
    ImageVkComputeApiExternalMemoryPtr image;
};

typedef std::shared_ptr<UnsampledImageVkComputeApiExternalMemory> UnsampledImageVkComputeApiExternalMemoryPtr;

DLL_OBJECT UnsampledImageVkComputeApiExternalMemoryPtr createUnsampledImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage);
DLL_OBJECT UnsampledImageVkComputeApiExternalMemoryPtr createUnsampledImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage, const ImageVkComputeApiInfo& imageComputeApiInfo);
DLL_OBJECT UnsampledImageVkComputeApiExternalMemoryPtr createUnsampledImageVkComputeApiExternalMemory(
        const ImageVkComputeApiExternalMemoryPtr& image);


/**
 * An sampled image.
 */
class DLL_OBJECT SampledImageVkComputeApiExternalMemory {
public:
    SampledImageVkComputeApiExternalMemory() {}
    virtual void initialize(
            const ImageVkComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) = 0;
    virtual ~SampledImageVkComputeApiExternalMemory() = default;

    inline const sgl::vk::ImagePtr& getVulkanImage() { return image->getVulkanImage(); }

    /*
     * Asynchronous copy from/to a device pointer to level 0 mipmap level.
     */
    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyFromDevicePtrAsync(devicePtrSrc, stream, eventOut);
    }
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) {
        image->copyToDevicePtrAsync(devicePtrDst, stream, eventOut);
    }

protected:
    ImageVkComputeApiExternalMemoryPtr image;
};

typedef std::shared_ptr<SampledImageVkComputeApiExternalMemory> SampledImageVkComputeApiExternalMemoryPtr;

DLL_OBJECT SampledImageVkComputeApiExternalMemoryPtr createSampledImageVkComputeApiExternalMemory(
        vk::TexturePtr& vulkanTexture, const TextureExternalMemorySettings& textureExternalMemorySettings);
DLL_OBJECT SampledImageVkComputeApiExternalMemoryPtr createSampledImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage, const ImageVkComputeApiInfo& imageComputeApiInfo);
DLL_OBJECT SampledImageVkComputeApiExternalMemoryPtr createSampledImageVkComputeApiExternalMemory(
        vk::ImagePtr& vulkanImage, const vk::ImageViewSettings& imageViewSettings,
        const vk::ImageSamplerSettings& imageSamplerSettings,
        const TextureExternalMemorySettings& textureExternalMemorySettings);

}}

#endif //SGL_VULKAN_INTEROP_COMPUTE_HPP
