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

#ifndef SGL_IMPLLEVELZERO_HPP
#define SGL_IMPLLEVELZERO_HPP

#include "../InteropCompute.hpp"
#include "../InteropLevelZero.hpp"

namespace sgl { namespace vk {

class SemaphoreVkLevelZeroInterop : public SemaphoreVkComputeApiInterop {
public:
    SemaphoreVkLevelZeroInterop();
    ~SemaphoreVkLevelZeroInterop() override;

    /// Signal semaphore.
    void signalSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) override;

    /// Wait on semaphore.
    void waitSemaphoreComputeApi(StreamWrapper stream, unsigned long long timelineValue = 0, void* eventOut = nullptr) override;

protected:
#ifdef _WIN32
    void setExternalSemaphoreWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalSemaphoreFd(int fileDescriptor) override;
#endif
    void importExternalSemaphore() override;

private:
    ze_external_semaphore_ext_desc_t externalSemaphoreExtDesc{};
#ifdef _WIN32
    ze_external_semaphore_win32_ext_desc_t externalSemaphoreWin32ExtDesc{};
#endif
#ifdef __linux__
    ze_external_semaphore_fd_ext_desc_t externalSemaphoreFdExtDesc{};
#endif
    void* externalSemaphore{};
};


class BufferVkLevelZeroInterop : public BufferVkComputeApiExternalMemory {
public:
    ~BufferVkLevelZeroInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyFromHostPtrAsync(void* hostPtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToHostPtrAsync(void* hostPtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

protected:
    void preCheckExternalMemoryImport() override;
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fileDescriptor) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    ze_device_mem_alloc_desc_t deviceMemAllocDesc{};
#ifdef _WIN32
    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};
#endif
#ifdef __linux__
    ze_external_memory_import_fd_t externalMemoryImportFd{};
#endif
    void* externalMemoryBuffer{}; // hipExternalMemory_t
};


class ImageVkLevelZeroInterop : public ImageVkComputeApiExternalMemory {
public:
    ~ImageVkLevelZeroInterop() override;

    void copyFromDevicePtrAsync(void* devicePtrSrc, StreamWrapper stream, void* eventOut = nullptr) override;
    void copyToDevicePtrAsync(void* devicePtrDst, StreamWrapper stream, void* eventOut = nullptr) override;

    ze_image_handle_t getImageHandle() { return static_cast<ze_image_handle_t>(mipmappedArray); }

protected:
    void preCheckExternalMemoryImport() override;
#ifdef _WIN32
    void setExternalMemoryWin32Handle(HANDLE handle) override;
#endif
#ifdef __linux__
    void setExternalMemoryFd(int fileDescriptor) override;
#endif
    void importExternalMemory() override;
    void free() override;

private:
    ze_image_desc_t zeImageDesc{};

    // Bindless images.
    ze_device_mem_alloc_desc_t deviceMemAllocDesc{};
    ze_image_pitched_exp_desc_t imagePitchedExpDesc{};
    ze_image_bindless_exp_desc_t imageBindlessExpDesc{};
    ze_sampler_desc_t samplerDesc{};
    void* devicePtr{}; // void* device pointer; only used by bindless images.

#ifdef _WIN32
    ze_external_memory_import_win32_handle_t externalMemoryImportWin32Handle{};
#endif
#ifdef __linux__
    ze_external_memory_import_fd_t externalMemoryImportFd{};
#endif

    void* mipmappedArray{}; // ze_image_handle_t
};


class DLL_OBJECT UnsampledImageVkLevelZeroInterop : public UnsampledImageVkComputeApiExternalMemory {
public:
    UnsampledImageVkLevelZeroInterop() = default;
    void initialize(const ImageVkComputeApiExternalMemoryPtr& _image) override { image = _image; }
    ~UnsampledImageVkLevelZeroInterop() {}

    [[nodiscard]] ze_image_handle_t getImageHandle() const { return std::static_pointer_cast<ImageVkLevelZeroInterop>(image)->getImageHandle(); }
};


class DLL_OBJECT SampledImageVkLevelZeroInterop : public SampledImageVkComputeApiExternalMemory {
public:
    SampledImageVkLevelZeroInterop() = default;
    void initialize(
            const ImageVkComputeApiExternalMemoryPtr& _image,
            const TextureExternalMemorySettings& textureExternalMemorySettings) override { image = _image; }
    ~SampledImageVkLevelZeroInterop() {}

    [[nodiscard]] ze_image_handle_t getImageHandle() const { return std::static_pointer_cast<ImageVkLevelZeroInterop>(image)->getImageHandle(); }
};

}}

#endif //SGL_IMPLLEVELZERO_HPP
