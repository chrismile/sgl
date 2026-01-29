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

#include "ImplSycl.hpp"

#include <sycl/sycl.hpp>

namespace sgl {
extern bool openMessageBoxOnComputeApiError;
extern sycl::queue* g_syclQueue;
}

namespace syclexp = sycl::ext::oneapi::experimental;

namespace sgl { namespace vk {

struct SyclExternalSemaphoreWrapper {
    syclexp::external_semaphore syclExternalSemaphore;
};
struct SyclExternalMemWrapper {
    syclexp::external_mem syclExternalMem;
};
struct SyclImageMemHandleWrapper {
    syclexp::image_descriptor syclImageDescriptor;
    syclexp::image_mem_handle syclImageMemHandle;
};


#ifdef _WIN32
void SemaphoreVkSyclInterop::setExternalSemaphoreWin32Handle(HANDLE handle) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    syclexp::external_semaphore_handle_type semaphoreHandleType;
    if (isTimelineSemaphore()) {
#ifndef SYCL_NO_EXTERNAL_TIMELINE_SEMAPHORE_SUPPORT
        semaphoreHandleType = syclexp::external_semaphore_handle_type::timeline_win32_nt_handle;
#else
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkSyclInterop::SemaphoreVkSyclInterop: "
                "The installed version of SYCL does not support external timeline semaphores.");
#endif
    } else {
        semaphoreHandleType = syclexp::external_semaphore_handle_type::win32_nt_handle;
    }
    syclexp::external_semaphore_descriptor<syclexp::resource_win32_handle>
            syclExternalSemaphoreDescriptor{handle, semaphoreHandleType};
    auto* wrapper = new SyclExternalSemaphoreWrapper;
    wrapper->syclExternalSemaphore = syclexp::import_external_semaphore(
            syclExternalSemaphoreDescriptor, *g_syclQueue);
    externalSemaphore = reinterpret_cast<void*>(wrapper);
}
#endif

#ifdef __linux__
void SemaphoreVkSyclInterop::setExternalSemaphoreFd(int fileDescriptor) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    syclexp::external_semaphore_handle_type semaphoreHandleType;
    if (isTimelineSemaphore()) {
#ifndef SYCL_NO_EXTERNAL_TIMELINE_SEMAPHORE_SUPPORT
        semaphoreHandleType = syclexp::external_semaphore_handle_type::timeline_fd;
#else
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkSyclInterop::SemaphoreVkSyclInterop: "
                "The installed version of SYCL does not support external timeline semaphores.");
#endif
    } else {
        semaphoreHandleType = syclexp::external_semaphore_handle_type::opaque_fd;
    }
    syclexp::external_semaphore_descriptor<syclexp::resource_fd>
            syclExternalSemaphoreDescriptor{fileDescriptor, semaphoreHandleType};
    auto* wrapper = new SyclExternalSemaphoreWrapper;
    wrapper->syclExternalSemaphore = syclexp::import_external_semaphore(
            syclExternalSemaphoreDescriptor, *g_syclQueue);
    externalSemaphore = reinterpret_cast<void*>(wrapper);
}
#endif

SemaphoreVkSyclInterop::~SemaphoreVkSyclInterop() {
    if (externalSemaphore) {
        auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
        syclexp::release_external_semaphore(wrapper->syclExternalSemaphore, *g_syclQueue);
        delete wrapper;
    }
}

void SemaphoreVkSyclInterop::signalSemaphoreComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
    sycl::event syclEvent;
    if (eventIn) {
        syclEvent = stream.syclQueuePtr->ext_oneapi_signal_external_semaphore(
                wrapper->syclExternalSemaphore, uint64_t(timelineValue), *static_cast<sycl::event*>(eventIn));
    } else {
        syclEvent = stream.syclQueuePtr->ext_oneapi_signal_external_semaphore(
                wrapper->syclExternalSemaphore, uint64_t(timelineValue));
    }
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void SemaphoreVkSyclInterop::waitSemaphoreComputeApi(
        StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) {
    auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
    sycl::event syclEvent;
    if (eventIn) {
        syclEvent = stream.syclQueuePtr->ext_oneapi_wait_external_semaphore(
                wrapper->syclExternalSemaphore, uint64_t(timelineValue), *static_cast<sycl::event*>(eventIn));
    } else {
        syclEvent = stream.syclQueuePtr->ext_oneapi_wait_external_semaphore(
                wrapper->syclExternalSemaphore, uint64_t(timelineValue));
    }
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}


#ifdef _WIN32
void BufferVkSyclInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = syclexp::external_mem_handle_type::win32_nt_handle;
    syclexp::external_mem_descriptor<syclexp::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, vulkanBuffer->getDeviceMemorySize()}; // memoryRequirements.size;
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = syclexp::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
}
#endif

#ifdef __linux__
void BufferVkSyclInterop::setExternalMemoryFd(int fileDescriptor) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = syclexp::external_mem_handle_type::opaque_fd;
    syclexp::external_mem_descriptor<syclexp::resource_fd>
        syclExternalMemDescriptor{fileDescriptor, memoryHandleType, vulkanBuffer->getDeviceMemorySize()}; // memoryRequirements.size;
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = syclexp::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
}
#endif

void BufferVkSyclInterop::importExternalMemory() {
    auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
    devicePtr = syclexp::map_external_linear_memory(
            wrapper->syclExternalMem, 0, vulkanBuffer->getDeviceMemorySize(), *g_syclQueue);
}

void BufferVkSyclInterop::free() {
    freeHandlesAndFds();
    if (externalMemoryBuffer) {
        auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
        syclexp::unmap_external_linear_memory(devicePtr, *g_syclQueue);
        syclexp::release_external_memory(wrapper->syclExternalMem, *g_syclQueue);
        delete wrapper;
        externalMemoryBuffer = {};
    }
}

BufferVkSyclInterop::~BufferVkSyclInterop() {
    BufferVkSyclInterop::free();
}

void BufferVkSyclInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtr, devicePtrSrc, vulkanBuffer->getSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void BufferVkSyclInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtrDst, devicePtr, vulkanBuffer->getSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void BufferVkSyclInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtr, hostPtrSrc, vulkanBuffer->getSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void BufferVkSyclInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(hostPtrDst, devicePtr, vulkanBuffer->getSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}


#ifdef _WIN32
void ImageVkSyclInterop::setExternalMemoryWin32Handle(HANDLE handle) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = syclexp::external_mem_handle_type::win32_nt_handle;
    syclexp::external_mem_descriptor<syclexp::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, vulkanImage->getDeviceMemorySize()}; // memoryRequirements.size;
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = syclexp::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
}
#endif

#ifdef __linux__
void ImageVkSyclInterop::setExternalMemoryFd(int fileDescriptor) {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = syclexp::external_mem_handle_type::opaque_fd;
    syclexp::external_mem_descriptor<syclexp::resource_fd>
            syclExternalMemDescriptor{fileDescriptor, memoryHandleType, vulkanImage->getDeviceMemorySize()}; // memoryRequirements.size;
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = syclexp::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemoryBuffer = reinterpret_cast<void*>(wrapper);
}
#endif

void ImageVkSyclInterop::importExternalMemory() {
    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    auto* wrapperImg = new SyclImageMemHandleWrapper;
    syclexp::image_descriptor& syclImageDescriptor = wrapperImg->syclImageDescriptor;
    syclImageDescriptor.width = imageSettings.width;
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
        syclImageDescriptor.height = imageSettings.height;
    }
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        syclImageDescriptor.depth = imageSettings.depth;
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
        syclImageDescriptor.array_size = imageSettings.arrayLayers;
    }
    syclImageDescriptor.num_levels = imageSettings.mipLevels;

    syclImageDescriptor.num_channels = unsigned(getImageFormatNumChannels(imageSettings.format));
    if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
        syclImageDescriptor.type = syclexp::image_type::array;
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_CUBE) {
        syclImageDescriptor.type = syclexp::image_type::cubemap;
    } else if (imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_1D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_2D
            || imageComputeApiInfo.imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        if (syclImageDescriptor.num_levels > 1) {
            syclImageDescriptor.type = syclexp::image_type::mipmap;
        } else {
            syclImageDescriptor.type = syclexp::image_type::standard;
        }
    } else {
        Logfile::get()->throwError(
                "Error in ImageVkSyclInterop::_initialize: "
                "Unsupported image view type for SYCL.");
    }
    switch (imageSettings.format) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_S8_UINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int8;
        break;
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int16;
        break;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int32;
        break;
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int8;
        break;
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int16;
        break;
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int32;
        break;
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unorm_int8;
        break;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unorm_int16;
        break;
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        syclImageDescriptor.channel_type = sycl::image_channel_type::snorm_int8;
        break;
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
        syclImageDescriptor.channel_type = sycl::image_channel_type::snorm_int16;
        break;
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::fp16;
        break;
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::fp32;
        break;
    default:
        sgl::Logfile::get()->throwError(
                "Error in ImageVkSyclInterop::_initialize: "
                "Unsupported channel type for SYCL.");
        return;
    }

    syclImageDescriptor.verify();

    auto* wrapperMem = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
    bool supportsHandleType = false;
    std::vector<syclexp::image_memory_handle_type> supportedHandleTypes =
            syclexp::get_image_memory_support(syclImageDescriptor, *g_syclQueue);
    for (auto supportedHandleType : supportedHandleTypes) {
        if (supportedHandleType == syclexp::image_memory_handle_type::opaque_handle) {
            supportsHandleType = true;
            break;
        }
    }
    if (!supportsHandleType) {
        syclexp::release_external_memory(wrapperMem->syclExternalMem, *g_syclQueue);
        delete wrapperMem;
        externalMemoryBuffer = nullptr;
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in ImageVkSyclInterop::_initialize: "
                    "Unsupported SYCL image memory type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in ImageVkSyclInterop::_initialize: "
                    "Unsupported SYCL image memory type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported SYCL image memory type");
    }

    wrapperImg->syclImageMemHandle = syclexp::map_external_image_memory(
            wrapperMem->syclExternalMem, syclImageDescriptor, *g_syclQueue);
    mipmappedArray = reinterpret_cast<void*>(wrapperImg);
}

void ImageVkSyclInterop::free() {
    freeHandlesAndFds();
    if (mipmappedArray) {
        auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
        syclexp::free_image_mem(
                wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor.type, *g_syclQueue);
        delete wrapperImg;
        mipmappedArray = {};
    }
    if (externalMemoryBuffer) {
        auto* wrapperMem = reinterpret_cast<SyclExternalMemWrapper*>(externalMemoryBuffer);
        syclexp::release_external_memory(wrapperMem->syclExternalMem, *g_syclQueue);
        delete wrapperMem;
        externalMemoryBuffer = {};
    }
}

ImageVkSyclInterop::~ImageVkSyclInterop() {
    ImageVkSyclInterop::free();
}

void ImageVkSyclInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
    auto syclEvent = stream.syclQueuePtr->ext_oneapi_copy(
            devicePtrSrc, wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor);
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void ImageVkSyclInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
    auto syclEvent = stream.syclQueuePtr->ext_oneapi_copy(
            wrapperImg->syclImageMemHandle, devicePtrDst, wrapperImg->syclImageDescriptor);
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}


void UnsampledImageVkSyclInterop::initialize(const ImageVkComputeApiExternalMemoryPtr& _image) {
    static_assert(sizeof(syclexp::unsampled_image_handle) == sizeof(rawImageHandle));
    this->image = _image;
    auto imageVkSycl = std::static_pointer_cast<ImageVkSyclInterop>(image);
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(imageVkSycl->mipmappedArray);

    if (!syclexp::is_image_handle_supported<syclexp::unsampled_image_handle>(
            wrapperImg->syclImageDescriptor, syclexp::image_memory_handle_type::opaque_handle,
            *g_syclQueue)) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in UnsampledImageVkSyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in UnsampledImageVkSyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported SYCL image handle type");
    }

    auto handle = syclexp::create_image(
            wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor, *g_syclQueue);
    rawImageHandle = handle.raw_handle;
}

UnsampledImageVkSyclInterop::~UnsampledImageVkSyclInterop() {
    if (rawImageHandle) {
        syclexp::unsampled_image_handle handle{rawImageHandle};
        syclexp::destroy_image_handle(handle, *g_syclQueue);
        rawImageHandle = {};
    }
}

uint64_t UnsampledImageVkSyclInterop::getRawHandle() {
    return rawImageHandle;
}


static sycl::addressing_mode getSyclSamplerAddressModeVk(VkSamplerAddressMode samplerAddressModeVk) {
    switch (samplerAddressModeVk) {
    case VK_SAMPLER_ADDRESS_MODE_REPEAT:
        return sycl::addressing_mode::repeat;
    case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        return sycl::addressing_mode::mirrored_repeat;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        return sycl::addressing_mode::clamp_to_edge;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        return sycl::addressing_mode::clamp;
    default:
        return sycl::addressing_mode::none;
    }
}

void SampledImageVkSyclInterop::initialize(
            const ImageVkComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) {
    static_assert(sizeof(syclexp::sampled_image_handle) == sizeof(rawImageHandle));
    this->image = _image;
    const auto& imageComputeApiInfo = image->getImageComputeApiInfo();
    const auto& samplerSettings = imageComputeApiInfo.imageSamplerSettings;

    auto imageVkSycl = std::static_pointer_cast<ImageVkSyclInterop>(image);
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(imageVkSycl->mipmappedArray);

    if (!syclexp::is_image_handle_supported<syclexp::sampled_image_handle>(
            wrapperImg->syclImageDescriptor, syclexp::image_memory_handle_type::opaque_handle,
            *g_syclQueue)) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in SampledImageVkSyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in SampledImageVkSyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported SYCL image handle type");
    }

    syclexp::bindless_image_sampler syclSampler{};
    syclSampler.addressing[0] = getSyclSamplerAddressModeVk(samplerSettings.addressModeU);
    syclSampler.addressing[1] = getSyclSamplerAddressModeVk(samplerSettings.addressModeV);
    syclSampler.addressing[2] = getSyclSamplerAddressModeVk(samplerSettings.addressModeW);
    syclSampler.coordinate =
            textureExternalMemorySettings.useNormalizedCoordinates
            ? sycl::coordinate_normalization_mode::normalized
            : sycl::coordinate_normalization_mode::unnormalized;
    syclSampler.filtering =
            samplerSettings.minFilter == VK_FILTER_NEAREST
            ? sycl::filtering_mode::nearest : sycl::filtering_mode::linear;
    syclSampler.mipmap_filtering =
            samplerSettings.mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST
            ? sycl::filtering_mode::nearest : sycl::filtering_mode::linear;
    syclSampler.cubemap_filtering = syclexp::cubemap_filtering_mode::disjointed;
    syclSampler.min_mipmap_level_clamp = samplerSettings.minLod;
    syclSampler.max_mipmap_level_clamp = samplerSettings.maxLod;
    syclSampler.max_anisotropy = samplerSettings.maxAnisotropy;

    auto handle = syclexp::create_image(
            wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor, *g_syclQueue);
    rawImageHandle = handle.raw_handle;
}

SampledImageVkSyclInterop::~SampledImageVkSyclInterop() {
    if (rawImageHandle) {
        syclexp::sampled_image_handle handle{rawImageHandle};
        syclexp::destroy_image_handle(handle, *g_syclQueue);
        rawImageHandle = {};
    }
}

uint64_t SampledImageVkSyclInterop::getRawHandle() {
    return rawImageHandle;
}

}}
