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

#include "InteropCuda.hpp"

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace sgl { namespace vk {

CudaDeviceApiFunctionTable g_cudaDeviceApiFunctionTable{};

#ifdef _WIN32
HMODULE g_cudaLibraryHandle = nullptr;
#define dlsym GetProcAddress
#else
void* g_cudaLibraryHandle = nullptr;
#endif

bool initializeCudaDeviceApiFunctionTable() {
    typedef CUresult ( *PFN_cuInit )( unsigned int Flags );
    typedef CUresult ( *PFN_cuGetErrorString )( CUresult error, const char **pStr );
    typedef CUresult ( *PFN_cuCtxCreate )( CUcontext *pctx, unsigned int flags, CUdevice dev );
    typedef CUresult ( *PFN_cuCtxDestroy )( CUcontext ctx );
    typedef CUresult ( *PFN_cuStreamCreate )( CUstream *phStream, unsigned int Flags );
    typedef CUresult ( *PFN_cuStreamDestroy )( CUstream hStream );
    typedef CUresult ( *PFN_cuStreamSynchronize )( CUstream hStream );
    typedef CUresult ( *PFN_cuMemAlloc )( CUdeviceptr *dptr, size_t bytesize );
    typedef CUresult ( *PFN_cuMemFree )( CUdeviceptr dptr );
    typedef CUresult ( *PFN_cuMemsetD8Async )( CUdeviceptr dstDevice, unsigned char uc, size_t N, CUstream hStream );
    typedef CUresult ( *PFN_cuMemsetD16Async )( CUdeviceptr dstDevice, unsigned short us, size_t N, CUstream hStream );
    typedef CUresult ( *PFN_cuMemsetD32Async )( CUdeviceptr dstDevice, unsigned int ui, size_t N, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpyAsync )( CUdeviceptr dst, CUdeviceptr src, size_t ByteCount, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpyDtoHAsync )( void *dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpyHtoDAsync )( CUdeviceptr dstDevice, const void *srcHost, size_t ByteCount, CUstream hStream );
    typedef CUresult ( *PFN_cuImportExternalMemory )( CUexternalMemory *extMem_out, const CUDA_EXTERNAL_MEMORY_HANDLE_DESC *memHandleDesc );
    typedef CUresult ( *PFN_cuExternalMemoryGetMappedBuffer )( CUdeviceptr *devPtr, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_BUFFER_DESC *bufferDesc );
    typedef CUresult ( *PFN_cuDestroyExternalMemory )( CUexternalMemory extMem );
    typedef CUresult ( *PFN_cuImportExternalSemaphore )( CUexternalSemaphore *extSem_out, const CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC *semHandleDesc );
    typedef CUresult ( *PFN_cuSignalExternalSemaphoresAsync )( const CUexternalSemaphore *extSemArray, const CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS *paramsArray, unsigned int numExtSems, CUstream stream );
    typedef CUresult ( *PFN_cuWaitExternalSemaphoresAsync )( const CUexternalSemaphore *extSemArray, const CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS *paramsArray, unsigned int numExtSems, CUstream stream );
    typedef CUresult ( *PFN_cuDestroyExternalSemaphore )( CUexternalSemaphore extSem );

#if defined(__linux__)
    g_cudaLibraryHandle = dlopen("libcuda.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_cudaLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeCudaDeviceApiFunctionTable: Could not load libcuda.so.");
        return false;
    }
#elif defined(_WIN32)
    g_cudaLibraryHandle = LoadLibraryA("nvcuda.dll");
    if (!g_cudaLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeCudaDeviceApiFunctionTable: Could not load libcuda.so.");
        return false;
    }
#endif
    g_cudaDeviceApiFunctionTable.cuInit = PFN_cuInit(dlsym(g_cudaLibraryHandle, TOSTRING(cuInit)));
    g_cudaDeviceApiFunctionTable.cuGetErrorString = PFN_cuGetErrorString(dlsym(g_cudaLibraryHandle, TOSTRING(cuGetErrorString)));
    g_cudaDeviceApiFunctionTable.cuCtxCreate = PFN_cuCtxCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxCreate)));
    g_cudaDeviceApiFunctionTable.cuCtxDestroy = PFN_cuCtxDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxDestroy)));
    g_cudaDeviceApiFunctionTable.cuStreamCreate = PFN_cuStreamCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuStreamCreate)));
    g_cudaDeviceApiFunctionTable.cuStreamDestroy = PFN_cuStreamDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuStreamDestroy)));
    g_cudaDeviceApiFunctionTable.cuStreamSynchronize = PFN_cuStreamSynchronize(dlsym(g_cudaLibraryHandle, TOSTRING(cuStreamSynchronize)));
    g_cudaDeviceApiFunctionTable.cuMemAlloc = PFN_cuMemAlloc(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemAlloc)));
    g_cudaDeviceApiFunctionTable.cuMemFree = PFN_cuMemFree(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemFree)));
    g_cudaDeviceApiFunctionTable.cuMemsetD8Async = PFN_cuMemsetD8Async(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemsetD8Async)));
    g_cudaDeviceApiFunctionTable.cuMemsetD16Async = PFN_cuMemsetD16Async(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemsetD16Async)));
    g_cudaDeviceApiFunctionTable.cuMemsetD32Async = PFN_cuMemsetD32Async(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemsetD32Async)));
    g_cudaDeviceApiFunctionTable.cuMemcpyAsync = PFN_cuMemcpyAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyAsync)));
    g_cudaDeviceApiFunctionTable.cuMemcpyDtoHAsync = PFN_cuMemcpyDtoHAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyDtoHAsync)));
    g_cudaDeviceApiFunctionTable.cuMemcpyHtoDAsync = PFN_cuMemcpyHtoDAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyHtoDAsync)));
    g_cudaDeviceApiFunctionTable.cuImportExternalMemory = PFN_cuImportExternalMemory(dlsym(g_cudaLibraryHandle, TOSTRING(cuImportExternalMemory)));
    g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer = PFN_cuExternalMemoryGetMappedBuffer(dlsym(g_cudaLibraryHandle, TOSTRING(cuExternalMemoryGetMappedBuffer)));
    g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory = PFN_cuDestroyExternalMemory(dlsym(g_cudaLibraryHandle, TOSTRING(cuDestroyExternalMemory)));
    g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore = PFN_cuImportExternalSemaphore(dlsym(g_cudaLibraryHandle, TOSTRING(cuImportExternalSemaphore)));
    g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync = PFN_cuSignalExternalSemaphoresAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuSignalExternalSemaphoresAsync)));
    g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync = PFN_cuWaitExternalSemaphoresAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuWaitExternalSemaphoresAsync)));
    g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore = PFN_cuDestroyExternalSemaphore(dlsym(g_cudaLibraryHandle, TOSTRING(cuDestroyExternalSemaphore)));

    if (!g_cudaDeviceApiFunctionTable.cuInit
            || !g_cudaDeviceApiFunctionTable.cuGetErrorString
            || !g_cudaDeviceApiFunctionTable.cuCtxCreate
            || !g_cudaDeviceApiFunctionTable.cuCtxDestroy
            || !g_cudaDeviceApiFunctionTable.cuStreamCreate
            || !g_cudaDeviceApiFunctionTable.cuStreamDestroy
            || !g_cudaDeviceApiFunctionTable.cuStreamSynchronize
            || !g_cudaDeviceApiFunctionTable.cuMemAlloc
            || !g_cudaDeviceApiFunctionTable.cuMemFree
            || !g_cudaDeviceApiFunctionTable.cuMemsetD8Async
            || !g_cudaDeviceApiFunctionTable.cuMemsetD16Async
            || !g_cudaDeviceApiFunctionTable.cuMemsetD32Async
            || !g_cudaDeviceApiFunctionTable.cuImportExternalMemory
            || !g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer
            || !g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory
            || !g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore
            || !g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync
            || !g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync
            || !g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore) {
        sgl::Logfile::get()->throwError(
                "Error in initializeCudaDeviceApiFunctionTable: "
                "At least one function pointer could not be loaded.");
    }

    return true;
}

#ifdef _WIN32
#undef dlsym
#endif

bool getIsCudaDeviceApiFunctionTableInitialized() {
    return g_cudaLibraryHandle != nullptr;
}

void freeCudaDeviceApiFunctionTable() {
    if (g_cudaLibraryHandle) {
#if defined(__linux__)
        dlclose(g_cudaLibraryHandle);
#elif defined(_WIN32)
        FreeLibrary(g_cudaLibraryHandle);
#endif
        g_cudaLibraryHandle = {};
    }
}

void _checkCUresult(CUresult cuResult, const char* text, const char* locationText) {
    if (cuResult != CUDA_SUCCESS) {
        const char* errorString = nullptr;
        cuResult = g_cudaDeviceApiFunctionTable.cuGetErrorString(cuResult, &errorString);
        if (cuResult == CUDA_SUCCESS) {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + errorString);
        } else {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + "Error in cuGetErrorString.");
        }
    }
}

SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop(
        sgl::vk::Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
        VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue) {
    VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo = {};
    exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#if defined(_WIN32)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
    Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: "
                "External semaphores are only supported on Linux, Android and Windows systems!");
#endif
    _initialize(
            device, semaphoreCreateFlags, semaphoreType, timelineSemaphoreInitialValue,
            &exportSemaphoreCreateInfo);


    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC externalSemaphoreHandleDesc{};

#if defined(_WIN32)
    auto _vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
                device->getVkDevice(), "vkGetSemaphoreWin32HandleKHR");
    if (!_vkGetSemaphoreWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: "
                "vkGetSemaphoreWin32HandleKHR was not found!");
    }

    VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo = {};
    semaphoreGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    semaphoreGetWin32HandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    semaphoreGetWin32HandleInfo.semaphore = semaphoreVk;
    handle = nullptr;
    if (_vkGetSemaphoreWin32HandleKHR(
            device->getVkDevice(), &semaphoreGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: "
                "vkGetSemaphoreFdKHR failed!");
    }

    if (isTimelineSemaphore()) {
#if CUDA_VERSION >= 11020
        externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_WIN32;
#else
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: Timeline semaphores "
                "are only supported starting in CUDA version 11.2.");
#endif
    } else {
        externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32;
    }
    externalSemaphoreHandleDesc.handle.win32.handle = handle;
#elif defined(__linux__)
    auto _vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
            device->getVkDevice(), "vkGetSemaphoreFdKHR");
    if (!_vkGetSemaphoreFdKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: "
                "vkGetSemaphoreFdKHR was not found!");
    }

    VkSemaphoreGetFdInfoKHR semaphoreGetFdInfo = {};
    semaphoreGetFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    semaphoreGetFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    semaphoreGetFdInfo.semaphore = semaphoreVk;
    fileDescriptor = 0;
    if (_vkGetSemaphoreFdKHR(device->getVkDevice(), &semaphoreGetFdInfo, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: "
                "vkGetSemaphoreFdKHR failed!");
    }

    if (isTimelineSemaphore()) {
#if CUDA_VERSION >= 11020
        externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_FD;
#else
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkCudaDriverApiInterop::SemaphoreVkCudaDriverApiInterop: Timeline semaphores "
                "are only supported starting in CUDA version 11.2.");
#endif
    } else {
        externalSemaphoreHandleDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD;
    }
    externalSemaphoreHandleDesc.handle.fd = fileDescriptor;
#endif


    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore(
            &cuExternalSemaphore, &externalSemaphoreHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalSemaphore: ");

    /*
     * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXTRES__INTEROP.html
     * - CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD and CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_FD:
     * "Ownership of the file descriptor is transferred to the CUDA driver when the handle is imported successfully."
     * - CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32 and CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_WIN32:
     * "Ownership of this handle is not transferred to CUDA after the import operation, so the application must release
     * the handle using the appropriate system call."
     */
#if defined(__linux__)
    fileDescriptor = -1;
#endif
}

SemaphoreVkCudaDriverApiInterop::~SemaphoreVkCudaDriverApiInterop() {
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore(cuExternalSemaphore);
    checkCUresult(cuResult, "Error in cuDestroyExternalSemaphore: ");
}

void SemaphoreVkCudaDriverApiInterop::signalSemaphoreCuda(CUstream stream, unsigned long long timelineValue) {
    CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS signalParams{};
    if (isTimelineSemaphore()) {
        signalParams.params.fence.value = timelineValue;
    }
    g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync(&cuExternalSemaphore, &signalParams, 1, stream);
}

void SemaphoreVkCudaDriverApiInterop::waitSemaphoreCuda(CUstream stream, unsigned long long timelineValue) {
    CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS waitParams{};
    if (isTimelineSemaphore()) {
        waitParams.params.fence.value = timelineValue;
    }
    g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync(&cuExternalSemaphore, &waitParams, 1, stream);
}


BufferCudaDriverApiExternalMemoryVk::BufferCudaDriverApiExternalMemoryVk(vk::BufferPtr& vulkanBuffer)
        : vulkanBuffer(vulkanBuffer) {
    VkDevice device = vulkanBuffer->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanBuffer->getVkDeviceMemory();

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device, vulkanBuffer->getVkBuffer(), &memoryRequirements);

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    externalMemoryHandleDesc.size = memoryRequirements.size;

#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: vkGetMemoryWin32HandleKHR was not found!");
        return;
    }
    VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = {};
    memoryGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    memoryGetWin32HandleInfo.memory = deviceMemory;
    memoryGetWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE handle = nullptr;
    if (_vkGetMemoryWin32HandleKHR(device, &memoryGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in BufferCudaDriverApiExternalMemoryVk::BufferCudaDriverApiExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
    externalMemoryHandleDesc.handle.win32.handle = (void*)handle;
    this->handle = handle;
#elif defined(__linux__)
    auto _vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!_vkGetMemoryFdKHR) {
        Logfile::get()->throwError(
                "Error in BufferCudaDriverApiExternalMemoryVk::BufferCudaDriverApiExternalMemoryVk: "
                "vkGetMemoryFdKHR was not found!");
        return;
    }

    VkMemoryGetFdInfoKHR memoryGetFdInfoKhr = {};
    memoryGetFdInfoKhr.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    memoryGetFdInfoKhr.memory = deviceMemory;
    memoryGetFdInfoKhr.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fileDescriptor = 0;
    if (_vkGetMemoryFdKHR(device, &memoryGetFdInfoKhr, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in BufferCudaDriverApiExternalMemoryVk::BufferCudaDriverApiExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    externalMemoryHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
    externalMemoryHandleDesc.handle.fd = fileDescriptor;
    this->fileDescriptor = fileDescriptor;
#else
    Logfile::get()->throwError(
            "Error in BufferCudaDriverApiExternalMemoryVk::BufferCudaDriverApiExternalMemoryVk: "
            "External memory is only supported on Linux, Android and Windows systems!");
    return false;
#endif

    CUexternalMemory cudaExtMemVertexBuffer{};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
            &cudaExtMemVertexBuffer, &externalMemoryHandleDesc);
    checkCUresult(cuResult, "Error in cuImportExternalMemory: ");

    /*
     * https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXTRES__INTEROP.html
     * - CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD: "Ownership of the file descriptor is transferred to the CUDA driver
     * when the handle is imported successfully."
     * - CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32: "Ownership of this handle is not transferred to CUDA after the
     * import operation, so the application must release the handle using the appropriate system call."
     */
#if defined(__linux__)
    this->fileDescriptor = -1;
#endif

    CUDA_EXTERNAL_MEMORY_BUFFER_DESC externalMemoryBufferDesc{};
    externalMemoryBufferDesc.offset = 0;
    externalMemoryBufferDesc.size = memoryRequirements.size;
    externalMemoryBufferDesc.flags = 0;
    cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer(
            &cudaDevicePtr, cudaExtMemVertexBuffer, &externalMemoryBufferDesc);
    checkCUresult(cuResult, "Error in cudaExternalMemoryGetMappedBuffer: ");
}

BufferCudaDriverApiExternalMemoryVk::~BufferCudaDriverApiExternalMemoryVk() {
#ifdef _WIN32
    CloseHandle(handle);
#else
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif
}

}}
