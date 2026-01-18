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

#include "../Resource.hpp"
#include "ImplSycl.hpp"

#include <sycl/sycl.hpp>

namespace sgl {
extern bool openMessageBoxOnComputeApiError;
extern sycl::queue* g_syclQueue;
}

namespace sgl { namespace d3d12 {

struct SyclExternalSemaphoreWrapper {
    sycl::ext::oneapi::experimental::external_semaphore syclExternalSemaphore;
};
struct SyclExternalMemWrapper {
    sycl::ext::oneapi::experimental::external_mem syclExternalMem;
};
struct SyclImageMemHandleWrapper {
    sycl::ext::oneapi::experimental::image_descriptor syclImageDescriptor;
    sycl::ext::oneapi::experimental::image_mem_handle syclImageMemHandle;
};

void FenceD3D12SyclInterop::importExternalFenceWin32Handle() {
    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    sycl::ext::oneapi::experimental::external_semaphore_handle_type semaphoreHandleType =
            sycl::ext::oneapi::experimental::external_semaphore_handle_type::win32_nt_dx12_fence;
    sycl::ext::oneapi::experimental::external_semaphore_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalSemaphoreDescriptor{handle, semaphoreHandleType};
    auto* wrapper = new SyclExternalSemaphoreWrapper;
    wrapper->syclExternalSemaphore = sycl::ext::oneapi::experimental::import_external_semaphore(
            syclExternalSemaphoreDescriptor, *g_syclQueue);
    externalSemaphore = reinterpret_cast<void*>(wrapper);
}

void FenceD3D12SyclInterop::free() {
    freeHandle();
    if (externalSemaphore) {
        auto* wrapper = reinterpret_cast<SyclExternalSemaphoreWrapper*>(externalSemaphore);
        sycl::ext::oneapi::experimental::release_external_semaphore(wrapper->syclExternalSemaphore, *g_syclQueue);
        delete wrapper;
        externalSemaphore = {};
    }
}

FenceD3D12SyclInterop::~FenceD3D12SyclInterop() {
    FenceD3D12SyclInterop::free();
}

void FenceD3D12SyclInterop::signalFenceComputeApi(
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
        *static_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void FenceD3D12SyclInterop::waitFenceComputeApi(
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


void BufferD3D12SyclInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();

    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::win32_nt_dx12_resource;
    sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, sizeInBytes};
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemory = reinterpret_cast<void*>(wrapper);

    devicePtr = sycl::ext::oneapi::experimental::map_external_linear_memory(
            wrapper->syclExternalMem, 0, sizeInBytes, *g_syclQueue);
}

void BufferD3D12SyclInterop::free() {
    freeHandle();
    if (externalMemory) {
        auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemory);
        sycl::ext::oneapi::experimental::unmap_external_linear_memory(devicePtr, *g_syclQueue);
        sycl::ext::oneapi::experimental::release_external_memory(wrapper->syclExternalMem, *g_syclQueue);
        delete wrapper;
        externalMemory = {};
    }
}

BufferD3D12SyclInterop::~BufferD3D12SyclInterop() {
    BufferD3D12SyclInterop::free();
}

void BufferD3D12SyclInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtr, devicePtrSrc, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void BufferD3D12SyclInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtrDst, devicePtr, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void BufferD3D12SyclInterop::copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(devicePtr, hostPtrSrc, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void BufferD3D12SyclInterop::copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut) {
    auto syclEvent = stream.syclQueuePtr->memcpy(hostPtrDst, devicePtr, resource->getCopiableSizeInBytes());
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}


void ImageD3D12SyclInterop::importExternalMemoryWin32Handle() {
    size_t sizeInBytes = resource->getCopiableSizeInBytes();

    // https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
    auto memoryHandleType = sycl::ext::oneapi::experimental::external_mem_handle_type::win32_nt_dx12_resource;
    sycl::ext::oneapi::experimental::external_mem_descriptor<sycl::ext::oneapi::experimental::resource_win32_handle>
            syclExternalMemDescriptor{(void*)handle, memoryHandleType, sizeInBytes};
    auto* wrapper = new SyclExternalMemWrapper;
    wrapper->syclExternalMem = sycl::ext::oneapi::experimental::import_external_memory(
            syclExternalMemDescriptor, *g_syclQueue);
    externalMemory = reinterpret_cast<void*>(wrapper);

    const auto& resourceDesc = resource->getD3D12ResourceDesc();
    auto* wrapperImg = new SyclImageMemHandleWrapper;
    sycl::ext::oneapi::experimental::image_descriptor& syclImageDescriptor = wrapperImg->syclImageDescriptor;
    syclImageDescriptor.width = resourceDesc.Width;
    if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE1D
            && resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D
            && resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        sgl::Logfile::get()->throwError(
                "Error in ImageD3D12SyclInterop::importExternalMemoryWin32Handle: Invalid D3D12 resource dimension.");
    }
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            || resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        syclImageDescriptor.height = resourceDesc.Height;
    }
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        // TODO: When syclImageDescriptor.array_size?
        syclImageDescriptor.depth = resourceDesc.DepthOrArraySize;
    }
    // CUDA_ERROR_ALREADY_MAPPED generated if numLevels == 0.
    syclImageDescriptor.num_levels = std::max(static_cast<unsigned>(resourceDesc.MipLevels), 1u);

    syclImageDescriptor.num_channels = unsigned(getDXGIFormatNumChannels(resourceDesc.Format));
    if (syclImageDescriptor.num_levels > 1) {
        syclImageDescriptor.type = sycl::ext::oneapi::experimental::image_type::mipmap;
    } else {
        syclImageDescriptor.type = sycl::ext::oneapi::experimental::image_type::standard;
    }

    switch (resourceDesc.Format) {
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8B8A8_UINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int8;
        break;
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16B16A16_UINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int16;
        break;
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unsigned_int32;
        break;
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int8;
        break;
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int16;
        break;
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::signed_int32;
        break;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unorm_int8;
        break;
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        syclImageDescriptor.channel_type = sycl::image_channel_type::unorm_int16;
        break;
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        syclImageDescriptor.channel_type = sycl::image_channel_type::snorm_int8;
        break;
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        syclImageDescriptor.channel_type = sycl::image_channel_type::snorm_int16;
        break;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::fp16;
        break;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
        syclImageDescriptor.channel_type = sycl::image_channel_type::fp32;
        break;
    default:
        sgl::Logfile::get()->throwError(
                "Error in ImageD3D12SyclInterop::_initialize: "
                "Unsupported channel type for SYCL.");
        return;
    }

    syclImageDescriptor.verify();

    auto* wrapperMem = reinterpret_cast<SyclExternalMemWrapper*>(externalMemory);
    bool supportsHandleType = false;
    std::vector<sycl::ext::oneapi::experimental::image_memory_handle_type> supportedHandleTypes =
            sycl::ext::oneapi::experimental::get_image_memory_support(syclImageDescriptor, *g_syclQueue);
    for (auto supportedHandleType : supportedHandleTypes) {
        if (supportedHandleType == sycl::ext::oneapi::experimental::image_memory_handle_type::opaque_handle) {
            supportsHandleType = true;
            break;
        }
    }
    if (!supportsHandleType) {
        sycl::ext::oneapi::experimental::release_external_memory(wrapperMem->syclExternalMem, *g_syclQueue);
        delete wrapperMem;
        externalMemory = nullptr;
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in ImageD3D12SyclInterop::_initialize: "
                    "Unsupported SYCL image memory type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in ImageD3D12SyclInterop::_initialize: "
                    "Unsupported SYCL image memory type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported SYCL image memory type");
    }

    wrapperImg->syclImageMemHandle = sycl::ext::oneapi::experimental::map_external_image_memory(
            wrapperMem->syclExternalMem, syclImageDescriptor, *g_syclQueue);
    mipmappedArray = reinterpret_cast<void*>(wrapperImg);
}

void ImageD3D12SyclInterop::free() {
    freeHandle();
    if (mipmappedArray) {
        auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
        sycl::ext::oneapi::experimental::free_image_mem(
                wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor.type, *g_syclQueue);
        delete wrapperImg;
        mipmappedArray = {};
    }
    if (externalMemory) {
        auto* wrapper = reinterpret_cast<SyclExternalMemWrapper*>(externalMemory);
        sycl::ext::oneapi::experimental::release_external_memory(wrapper->syclExternalMem, *g_syclQueue);
        delete wrapper;
        externalMemory = {};
    }
}

ImageD3D12SyclInterop::~ImageD3D12SyclInterop() {
    ImageD3D12SyclInterop::free();
}

void ImageD3D12SyclInterop::copyFromDevicePtrAsync(
        void* devicePtrSrc, StreamWrapper stream, void* eventOut) {
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
    auto syclEvent = stream.syclQueuePtr->ext_oneapi_copy(
            devicePtrSrc, wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor);
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}

void ImageD3D12SyclInterop::copyToDevicePtrAsync(
        void* devicePtrDst, StreamWrapper stream, void* eventOut) {
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(mipmappedArray);
    auto syclEvent = stream.syclQueuePtr->ext_oneapi_copy(
            wrapperImg->syclImageMemHandle, devicePtrDst, wrapperImg->syclImageDescriptor);
    if (eventOut) {
        *reinterpret_cast<sycl::event*>(eventOut) = std::move(syclEvent);
    }
}


void UnsampledImageD3D12SyclInterop::initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) {
    static_assert(sizeof(sycl::ext::oneapi::experimental::unsampled_image_handle) == sizeof(rawImageHandle));
    this->image = _image;
    auto imageVkSycl = std::static_pointer_cast<ImageD3D12SyclInterop>(image);
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(imageVkSycl->mipmappedArray);

    if (!sycl::ext::oneapi::experimental::is_image_handle_supported<sycl::ext::oneapi::experimental::unsampled_image_handle>(
            wrapperImg->syclImageDescriptor, sycl::ext::oneapi::experimental::image_memory_handle_type::opaque_handle,
            *g_syclQueue)) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in UnsampledImageD3D12SyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in UnsampledImageD3D12SyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported SYCL image handle type");
    }

    auto handle = sycl::ext::oneapi::experimental::create_image(
            wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor, *g_syclQueue);
    rawImageHandle = handle.raw_handle;
}

UnsampledImageD3D12SyclInterop::~UnsampledImageD3D12SyclInterop() {
    if (rawImageHandle) {
        sycl::ext::oneapi::experimental::unsampled_image_handle handle{rawImageHandle};
        sycl::ext::oneapi::experimental::destroy_image_handle(handle, *g_syclQueue);
        rawImageHandle = {};
    }
}

uint64_t UnsampledImageD3D12SyclInterop::getRawHandle() {
    return rawImageHandle;
}


static sycl::addressing_mode getSyclSamplerAddressModeD3D12(D3D12_TEXTURE_ADDRESS_MODE samplerAddressModeD3D12) {
    switch (samplerAddressModeD3D12) {
    case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
        return sycl::addressing_mode::repeat;
    case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:
    case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:
        return sycl::addressing_mode::mirrored_repeat;
    case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
        return sycl::addressing_mode::clamp_to_edge;
    case D3D12_TEXTURE_ADDRESS_MODE_BORDER:
        return sycl::addressing_mode::clamp;
    default:
        return sycl::addressing_mode::none;
    }
}

void SampledImageD3D12SyclInterop::initialize(
            const ImageD3D12ComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) {
    static_assert(sizeof(sycl::ext::oneapi::experimental::sampled_image_handle) == sizeof(rawImageHandle));
    this->image = _image;
    const auto& imageComputeApiInfo = image->getImageComputeApiInfo();
    const auto& samplerDesc = imageComputeApiInfo.samplerDesc;

    auto imageVkSycl = std::static_pointer_cast<ImageD3D12SyclInterop>(image);
    auto* wrapperImg = reinterpret_cast<SyclImageMemHandleWrapper*>(imageVkSycl->mipmappedArray);

    if (!sycl::ext::oneapi::experimental::is_image_handle_supported<sycl::ext::oneapi::experimental::sampled_image_handle>(
            wrapperImg->syclImageDescriptor, sycl::ext::oneapi::experimental::image_memory_handle_type::opaque_handle,
            *g_syclQueue)) {
        if (openMessageBoxOnComputeApiError) {
            sgl::Logfile::get()->writeError(
                    "Error in SampledImageD3D12SyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.");
        } else {
            sgl::Logfile::get()->write(
                    "Error in SampledImageD3D12SyclInterop::_initialize: "
                    "Unsupported SYCL image handle type.", sgl::RED);
        }
        throw UnsupportedComputeApiFeatureException("Unsupported SYCL image handle type");
    }

    sycl::ext::oneapi::experimental::bindless_image_sampler syclSampler{};
    syclSampler.addressing[0] = getSyclSamplerAddressModeD3D12(samplerDesc.AddressU);
    syclSampler.addressing[1] = getSyclSamplerAddressModeD3D12(samplerDesc.AddressV);
    syclSampler.addressing[2] = getSyclSamplerAddressModeD3D12(samplerDesc.AddressW);
    syclSampler.coordinate =
            textureExternalMemorySettings.useNormalizedCoordinates
            ? sycl::coordinate_normalization_mode::normalized
            : sycl::coordinate_normalization_mode::unnormalized;
    syclSampler.filtering =
            samplerDesc.Filter == D3D12_FILTER_MIN_MAG_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR
            ? sycl::filtering_mode::nearest : sycl::filtering_mode::linear;
    syclSampler.mipmap_filtering =
            samplerDesc.Filter == D3D12_FILTER_MIN_MAG_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT
            || samplerDesc.Filter == D3D12_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT
            ? sycl::filtering_mode::nearest : sycl::filtering_mode::linear;
    syclSampler.cubemap_filtering = sycl::ext::oneapi::experimental::cubemap_filtering_mode::disjointed;
    syclSampler.min_mipmap_level_clamp = samplerDesc.MinLOD;
    syclSampler.max_mipmap_level_clamp = samplerDesc.MaxLOD;
    syclSampler.max_anisotropy = samplerDesc.MaxAnisotropy;

    auto handle = sycl::ext::oneapi::experimental::create_image(
            wrapperImg->syclImageMemHandle, wrapperImg->syclImageDescriptor, *g_syclQueue);
    rawImageHandle = handle.raw_handle;
}

SampledImageD3D12SyclInterop::~SampledImageD3D12SyclInterop() {
    if (rawImageHandle) {
        sycl::ext::oneapi::experimental::sampled_image_handle handle{rawImageHandle};
        sycl::ext::oneapi::experimental::destroy_image_handle(handle, *g_syclQueue);
        rawImageHandle = {};
    }
}

uint64_t SampledImageD3D12SyclInterop::getRawHandle() {
    return rawImageHandle;
}

}}
