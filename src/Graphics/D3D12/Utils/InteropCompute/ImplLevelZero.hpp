/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#ifndef SGL_D3D12_IMPLLEVELZERO_HPP
#define SGL_D3D12_IMPLLEVELZERO_HPP

#include "../InteropCompute.hpp"
#include "../InteropLevelZero.hpp"

namespace sgl { namespace d3d12 {

class FenceD3D12LevelZeroInterop : public FenceD3D12ComputeApiInterop {
public:
    ~FenceD3D12LevelZeroInterop() override;

    /// Signal fence.
    void signalFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) override;

    /// Wait on fence.
    void waitFenceComputeApi(StreamWrapper stream, unsigned long long timelineValue, void* eventIn, void* eventOut) override;

protected:
    void importExternalFenceWin32Handle() override;
    void free() override;

private:
    ze_external_semaphore_ext_desc_t externalSemaphoreExtDesc{};
    ze_external_semaphore_win32_ext_desc_t externalSemaphoreWin32ExtDesc{};
    void* externalSemaphore{};
};


class BufferD3D12LevelZeroInterop : public BufferD3D12ComputeApiExternalMemory {
public:
    ~BufferD3D12LevelZeroInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
    void importExternalMemoryWin32Handle() override;
    void free() override;

private:
    ze_device_mem_alloc_desc_t deviceMemAllocDesc{};
    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};
    void* externalMemoryBuffer{}; // hipExternalMemory_t
};


class ImageD3D12LevelZeroInterop : public ImageD3D12ComputeApiExternalMemory {
    friend class UnsampledImageD3D12LevelZeroInterop;
    friend class SampledImageD3D12LevelZeroInterop;
public:
    ~ImageD3D12LevelZeroInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

    ze_image_handle_t getImageHandle() { return static_cast<ze_image_handle_t>(mipmappedArray); }

protected:
    void importExternalMemoryWin32Handle() override;
    void free() override;

private:
    ze_image_desc_t zeImageDesc{};

    // Bindless images.
    ze_device_mem_alloc_desc_t deviceMemAllocDesc{};
    ze_image_pitched_exp_desc_t imagePitchedExpDesc{};
    ze_image_bindless_exp_desc_t imageBindlessExpDesc{};
    ze_sampler_desc_t samplerDesc{};
    void* devicePtr{}; // void* device pointer; only used by bindless images.

    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};

    void* mipmappedArray{}; // ze_image_handle_t
};


class DLL_OBJECT UnsampledImageD3D12LevelZeroInterop : public UnsampledImageD3D12ComputeApiExternalMemory {
public:
    UnsampledImageD3D12LevelZeroInterop() = default;
    void initialize(const ImageD3D12ComputeApiExternalMemoryPtr& _image) override { image = _image; }
    ~UnsampledImageD3D12LevelZeroInterop() override {}

    [[nodiscard]] ze_image_handle_t getImageHandle() const { return std::static_pointer_cast<ImageD3D12LevelZeroInterop>(image)->getImageHandle(); }
};


class DLL_OBJECT SampledImageD3D12LevelZeroInterop : public SampledImageD3D12ComputeApiExternalMemory {
public:
    SampledImageD3D12LevelZeroInterop() = default;
    void initialize(
            const ImageD3D12ComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) override { image = _image; }
    ~SampledImageD3D12LevelZeroInterop() override {}

    [[nodiscard]] ze_image_handle_t getImageHandle() const { return std::static_pointer_cast<ImageD3D12LevelZeroInterop>(image)->getImageHandle(); }
};

}}

#endif //SGL_D3D12_IMPLLEVELZERO_HPP
