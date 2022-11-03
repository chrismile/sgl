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

#include <cstring>
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
    typedef CUresult ( *PFN_cuDeviceGet )(CUdevice *device, int ordinal);
    typedef CUresult ( *PFN_cuDeviceGetCount )(int *count);
    typedef CUresult ( *PFN_cuDeviceGetUuid )(CUuuid *uuid, CUdevice dev);
    typedef CUresult ( *PFN_cuDeviceGetAttribute )(int *pi, CUdevice_attribute attrib, CUdevice dev);
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
    typedef CUresult ( *PFN_cuMipmappedArrayDestroy )( CUmipmappedArray hMipmappedArray );
    typedef CUresult ( *PFN_cuTexObjectCreate )( CUtexObject *pTexObject, const CUDA_RESOURCE_DESC *pResDesc, const CUDA_TEXTURE_DESC *pTexDesc, const CUDA_RESOURCE_VIEW_DESC *pResViewDesc );
    typedef CUresult ( *PFN_cuTexObjectDestroy )( CUtexObject texObject );
    typedef CUresult ( *PFN_cuSurfObjectCreate )( CUsurfObject *pSurfObject, const CUDA_RESOURCE_DESC *pResDesc );
    typedef CUresult ( *PFN_cuSurfObjectDestroy )( CUsurfObject surfObject );
    typedef CUresult ( *PFN_cuImportExternalMemory )( CUexternalMemory *extMem_out, const CUDA_EXTERNAL_MEMORY_HANDLE_DESC *memHandleDesc );
    typedef CUresult ( *PFN_cuExternalMemoryGetMappedBuffer )( CUdeviceptr *devPtr, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_BUFFER_DESC *bufferDesc );
    typedef CUresult ( *PFN_cuExternalMemoryGetMappedMipmappedArray )( CUmipmappedArray *mipmap, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC *mipmapDesc );
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
        sgl::Logfile::get()->writeInfo("initializeCudaDeviceApiFunctionTable: Could not load nvcuda.dll.");
        return false;
    }
#endif
    g_cudaDeviceApiFunctionTable.cuInit = PFN_cuInit(dlsym(g_cudaLibraryHandle, TOSTRING(cuInit)));
    g_cudaDeviceApiFunctionTable.cuGetErrorString = PFN_cuGetErrorString(dlsym(g_cudaLibraryHandle, TOSTRING(cuGetErrorString)));
    g_cudaDeviceApiFunctionTable.cuDeviceGet = PFN_cuDeviceGet(dlsym(g_cudaLibraryHandle, TOSTRING(cuDeviceGet)));
    g_cudaDeviceApiFunctionTable.cuDeviceGetCount = PFN_cuDeviceGetCount(dlsym(g_cudaLibraryHandle, TOSTRING(cuDeviceGetCount)));
    g_cudaDeviceApiFunctionTable.cuDeviceGetUuid = PFN_cuDeviceGetUuid(dlsym(g_cudaLibraryHandle, TOSTRING(cuDeviceGetUuid)));
    g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute = PFN_cuDeviceGetAttribute(dlsym(g_cudaLibraryHandle, TOSTRING(cuDeviceGetAttribute)));
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
    g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy = PFN_cuMipmappedArrayDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuMipmappedArrayDestroy)));
    g_cudaDeviceApiFunctionTable.cuTexObjectCreate = PFN_cuTexObjectCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuTexObjectCreate)));
    g_cudaDeviceApiFunctionTable.cuTexObjectDestroy = PFN_cuTexObjectDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuTexObjectDestroy)));
    g_cudaDeviceApiFunctionTable.cuSurfObjectCreate = PFN_cuSurfObjectCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuSurfObjectCreate)));
    g_cudaDeviceApiFunctionTable.cuSurfObjectDestroy = PFN_cuSurfObjectDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuSurfObjectDestroy)));
    g_cudaDeviceApiFunctionTable.cuImportExternalMemory = PFN_cuImportExternalMemory(dlsym(g_cudaLibraryHandle, TOSTRING(cuImportExternalMemory)));
    g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer = PFN_cuExternalMemoryGetMappedBuffer(dlsym(g_cudaLibraryHandle, TOSTRING(cuExternalMemoryGetMappedBuffer)));
    g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedMipmappedArray = PFN_cuExternalMemoryGetMappedMipmappedArray(dlsym(g_cudaLibraryHandle, TOSTRING(cuExternalMemoryGetMappedMipmappedArray)));
    g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory = PFN_cuDestroyExternalMemory(dlsym(g_cudaLibraryHandle, TOSTRING(cuDestroyExternalMemory)));
    g_cudaDeviceApiFunctionTable.cuImportExternalSemaphore = PFN_cuImportExternalSemaphore(dlsym(g_cudaLibraryHandle, TOSTRING(cuImportExternalSemaphore)));
    g_cudaDeviceApiFunctionTable.cuSignalExternalSemaphoresAsync = PFN_cuSignalExternalSemaphoresAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuSignalExternalSemaphoresAsync)));
    g_cudaDeviceApiFunctionTable.cuWaitExternalSemaphoresAsync = PFN_cuWaitExternalSemaphoresAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuWaitExternalSemaphoresAsync)));
    g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore = PFN_cuDestroyExternalSemaphore(dlsym(g_cudaLibraryHandle, TOSTRING(cuDestroyExternalSemaphore)));

    if (!g_cudaDeviceApiFunctionTable.cuInit
            || !g_cudaDeviceApiFunctionTable.cuGetErrorString
            || !g_cudaDeviceApiFunctionTable.cuDeviceGet
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetCount
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetUuid
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute
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
            || !g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy
            || !g_cudaDeviceApiFunctionTable.cuTexObjectCreate
            || !g_cudaDeviceApiFunctionTable.cuTexObjectDestroy
            || !g_cudaDeviceApiFunctionTable.cuSurfObjectCreate
            || !g_cudaDeviceApiFunctionTable.cuSurfObjectDestroy
            || !g_cudaDeviceApiFunctionTable.cuImportExternalMemory
            || !g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer
            || !g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedMipmappedArray
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

bool getMatchingCudaDevice(sgl::vk::Device* device, CUdevice* cuDevice) {
    const VkPhysicalDeviceIDProperties& deviceIdProperties = device->getDeviceIDProperties();
    bool foundDevice = false;

    int numDevices = 0;
    CUresult cuResult = sgl::vk::g_cudaDeviceApiFunctionTable.cuDeviceGetCount(&numDevices);
    sgl::vk::checkCUresult(cuResult, "Error in cuDeviceGetCount: ");

    for (int deviceIdx = 0; deviceIdx < numDevices; deviceIdx++) {
        CUdevice currDevice = 0;
        cuResult = sgl::vk::g_cudaDeviceApiFunctionTable.cuDeviceGet(&currDevice, deviceIdx);
        sgl::vk::checkCUresult(cuResult, "Error in cuDeviceGet: ");

        CUuuid currUuid = {};
        cuResult = sgl::vk::g_cudaDeviceApiFunctionTable.cuDeviceGetUuid(&currUuid, currDevice);
        sgl::vk::checkCUresult(cuResult, "Error in cuDeviceGetUuid: ");

        bool isSameUuid = true;
        for (int i = 0; i < 16; i++) {
            if (deviceIdProperties.deviceUUID[i] != reinterpret_cast<uint8_t*>(&currUuid.bytes)[i]) {
                isSameUuid = false;
                break;
            }
        }
        if (isSameUuid) {
            foundDevice = true;
            *cuDevice = currDevice;
            break;
        }
    }

    return foundDevice;
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
                "Error in BufferCudaDriverApiExternalMemoryVk::BufferCudaDriverApiExternalMemoryVk: "
                "vkGetMemoryWin32HandleKHR was not found!");
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

    cudaExternalMemoryBuffer = {};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
            &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
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
    externalMemoryBufferDesc.offset = vulkanBuffer->getDeviceMemoryOffset();
    externalMemoryBufferDesc.size = memoryRequirements.size;
    externalMemoryBufferDesc.flags = 0;
    cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedBuffer(
            &cudaDevicePtr, cudaExternalMemoryBuffer, &externalMemoryBufferDesc);
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
    if (cudaExternalMemoryBuffer) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMemFree(cudaDevicePtr);
        checkCUresult(cuResult, "Error in cuMemFree: ");
        cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory(cudaExternalMemoryBuffer);
        checkCUresult(cuResult, "Error in cuDestroyExternalMemory: ");
    }
}


ImageCudaExternalMemoryVk::ImageCudaExternalMemoryVk(vk::ImagePtr& vulkanImage) {
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    const auto& imageSettings = vulkanImage->getImageSettings();
    if (imageSettings.imageType == VK_IMAGE_TYPE_1D) {
        imageViewType = VK_IMAGE_VIEW_TYPE_1D;
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_2D) {
        imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_3D) {
        imageViewType = VK_IMAGE_VIEW_TYPE_3D;
    }
    bool surfaceLoadStore = (imageSettings.usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
    _initialize(vulkanImage, imageViewType, surfaceLoadStore);
}

ImageCudaExternalMemoryVk::ImageCudaExternalMemoryVk(
        vk::ImagePtr& vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore) {
    _initialize(vulkanImage, imageViewType, surfaceLoadStore);
}

static CUarray_format getCudaArrayFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_S8_UINT:
            return CU_AD_FORMAT_UNSIGNED_INT8;
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16A16_UINT:
            return CU_AD_FORMAT_UNSIGNED_INT16;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32A32_UINT:
            return CU_AD_FORMAT_UNSIGNED_INT32;
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            return CU_AD_FORMAT_SIGNED_INT8;
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16A16_SINT:
            return CU_AD_FORMAT_SIGNED_INT16;
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32A32_SINT:
            return CU_AD_FORMAT_SIGNED_INT32;
        case VK_FORMAT_R8_UNORM:
            return CU_AD_FORMAT_UNORM_INT8X1;
        case VK_FORMAT_R8G8_UNORM:
            return CU_AD_FORMAT_UNORM_INT8X2;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            return CU_AD_FORMAT_UNORM_INT8X4;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_D16_UNORM:
            return CU_AD_FORMAT_UNORM_INT16X1;
        case VK_FORMAT_R16G16_UNORM:
            return CU_AD_FORMAT_UNORM_INT16X2;
        case VK_FORMAT_R16G16B16A16_UNORM:
            return CU_AD_FORMAT_UNORM_INT16X4;
        case VK_FORMAT_R8_SNORM:
            return CU_AD_FORMAT_SNORM_INT8X1;
        case VK_FORMAT_R8G8_SNORM:
            return CU_AD_FORMAT_SNORM_INT8X2;
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            return CU_AD_FORMAT_SNORM_INT8X4;
        case VK_FORMAT_R16_SNORM:
            return CU_AD_FORMAT_SNORM_INT16X1;
        case VK_FORMAT_R16G16_SNORM:
            return CU_AD_FORMAT_SNORM_INT16X2;
        case VK_FORMAT_R16G16B16A16_SNORM:
            return CU_AD_FORMAT_SNORM_INT16X4;
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return CU_AD_FORMAT_HALF;
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT:
            return CU_AD_FORMAT_FLOAT;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaArrayFormat: Unsupported format.");
            return CU_AD_FORMAT_FLOAT;
    }
}

void ImageCudaExternalMemoryVk::_initialize(
        vk::ImagePtr& _vulkanImage, VkImageViewType imageViewType, bool surfaceLoadStore) {
    vulkanImage = _vulkanImage;

    VkDevice device = vulkanImage->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanImage->getVkDeviceMemory();

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device, vulkanImage->getVkImage(), &memoryRequirements);

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemoryHandleDesc{};
    externalMemoryHandleDesc.size = memoryRequirements.size;

#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in ImageCudaExternalMemoryVk::ImageCudaExternalMemoryVk: "
                "vkGetMemoryWin32HandleKHR was not found!");
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

    cudaExternalMemoryBuffer = {};
    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuImportExternalMemory(
            &cudaExternalMemoryBuffer, &externalMemoryHandleDesc);
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

    const auto& imageSettings = vulkanImage->getImageSettings();

    CUDA_ARRAY3D_DESCRIPTOR arrayDescriptor{};
    arrayDescriptor.Width = imageSettings.width;
    if (imageViewType == VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_3D
            || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Height = imageSettings.height;
    }
    if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        arrayDescriptor.Depth = imageSettings.depth;
    } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Depth = imageSettings.arrayLayers;
    }
    arrayDescriptor.Format = getCudaArrayFormat(imageSettings.format);
    arrayDescriptor.NumChannels = getImageFormatNumChannels(imageSettings.format);
    if (imageSettings.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_COLOR_ATTACHMENT;
    }
    if (surfaceLoadStore) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_SURFACE_LDST;
    }
    if (isDepthStencilFormat(imageSettings.format)) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_DEPTH_TEXTURE;
    }
    if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_CUBEMAP;
    }
    if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        arrayDescriptor.Flags |= CUDA_ARRAY3D_LAYERED;
    }

    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC externalMemoryMipmappedArrayDesc{};
    externalMemoryMipmappedArrayDesc.offset = vulkanImage->getDeviceMemoryOffset();
    externalMemoryMipmappedArrayDesc.numLevels = imageSettings.mipLevels;
    externalMemoryMipmappedArrayDesc.arrayDesc = arrayDescriptor;
    cuResult = g_cudaDeviceApiFunctionTable.cuExternalMemoryGetMappedMipmappedArray(
            &cudaMipmappedArray, cudaExternalMemoryBuffer, &externalMemoryMipmappedArrayDesc);
    checkCUresult(cuResult, "Error in cuExternalMemoryGetMappedMipmappedArray: ");
}

ImageCudaExternalMemoryVk::~ImageCudaExternalMemoryVk() {
#ifdef _WIN32
    CloseHandle(handle);
#else
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif
    if (cudaExternalMemoryBuffer) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy(cudaMipmappedArray);
        checkCUresult(cuResult, "Error in cuMipmappedArrayDestroy: ");
        cuResult = g_cudaDeviceApiFunctionTable.cuDestroyExternalMemory(cudaExternalMemoryBuffer);
        checkCUresult(cuResult, "Error in cuDestroyExternalMemory: ");
    }
}

static CUresourceViewFormat getCudaResourceViewFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_S8_UINT:
            return CU_RES_VIEW_FORMAT_UINT_1X8;
        case VK_FORMAT_R8G8_UINT:
            return CU_RES_VIEW_FORMAT_UINT_2X8;
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            return CU_RES_VIEW_FORMAT_UINT_4X8;
        case VK_FORMAT_R16_UINT:
            return CU_RES_VIEW_FORMAT_UINT_1X16;
        case VK_FORMAT_R32_UINT:
            return CU_RES_VIEW_FORMAT_UINT_1X32;
        case VK_FORMAT_R16G16_UINT:
            return CU_RES_VIEW_FORMAT_UINT_2X16;
        case VK_FORMAT_R32G32_UINT:
            return CU_RES_VIEW_FORMAT_UINT_2X32;
        case VK_FORMAT_R16G16B16A16_UINT:
            return CU_RES_VIEW_FORMAT_UINT_4X16;
        case VK_FORMAT_R32G32B32A32_UINT:
            return CU_RES_VIEW_FORMAT_UINT_4X32;
        case VK_FORMAT_R8_SINT:
            return CU_RES_VIEW_FORMAT_SINT_1X8;
        case VK_FORMAT_R8G8_SINT:
            return CU_RES_VIEW_FORMAT_SINT_2X8;
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            return CU_RES_VIEW_FORMAT_SINT_4X8;
        case VK_FORMAT_R16_SINT:
            return CU_RES_VIEW_FORMAT_SINT_1X16;
        case VK_FORMAT_R32_SINT:
            return CU_RES_VIEW_FORMAT_SINT_1X32;
        case VK_FORMAT_R16G16_SINT:
            return CU_RES_VIEW_FORMAT_SINT_2X16;
        case VK_FORMAT_R32G32_SINT:
            return CU_RES_VIEW_FORMAT_SINT_2X32;
        case VK_FORMAT_R16G16B16A16_SINT:
            return CU_RES_VIEW_FORMAT_SINT_4X16;
        case VK_FORMAT_R32G32B32A32_SINT:
            return CU_RES_VIEW_FORMAT_SINT_4X32;

        // TODO: Test if UNORM formats should really use float32.
        case VK_FORMAT_R8_UNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_1X32;
        case VK_FORMAT_R8G8_UNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_2X32;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            return CU_RES_VIEW_FORMAT_FLOAT_4X32;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_D16_UNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_1X32;
        case VK_FORMAT_R16G16_UNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_2X32;
        case VK_FORMAT_R16G16B16A16_UNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_4X32;
        case VK_FORMAT_R8_SNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_1X32;
        case VK_FORMAT_R8G8_SNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_2X32;
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            return CU_RES_VIEW_FORMAT_FLOAT_4X32;
        case VK_FORMAT_R16_SNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_1X32;
        case VK_FORMAT_R16G16_SNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_2X32;
        case VK_FORMAT_R16G16B16A16_SNORM:
            return CU_RES_VIEW_FORMAT_FLOAT_4X32;

        case VK_FORMAT_R16_SFLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_1X16;
        case VK_FORMAT_R16G16_SFLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_2X16;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_4X16;
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_1X32;
        case VK_FORMAT_R32G32_SFLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_2X32;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return CU_RES_VIEW_FORMAT_FLOAT_4X32;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaResourceViewFormat: Unsupported format.");
            return CU_RES_VIEW_FORMAT_NONE;
    }
}

static CUaddress_mode getCudaSamplerAddressMode(VkSamplerAddressMode samplerAddressModeVk) {
    switch (samplerAddressModeVk) {
        case VK_SAMPLER_ADDRESS_MODE_REPEAT:
            return CU_TR_ADDRESS_MODE_WRAP;
        case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
            return CU_TR_ADDRESS_MODE_MIRROR;
        case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
            return CU_TR_ADDRESS_MODE_CLAMP;
        case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
            return CU_TR_ADDRESS_MODE_BORDER;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaSamplerAddressMode: Unsupported format.");
            return CU_TR_ADDRESS_MODE_WRAP;
    }
}

static CUfilter_mode getCudaFilterFormat(VkFilter filterVk) {
    switch (filterVk) {
        case VK_FILTER_NEAREST:
            return CU_TR_FILTER_MODE_POINT;
        case VK_FILTER_LINEAR:
            return CU_TR_FILTER_MODE_LINEAR;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaFilterFormat: Unsupported filter format.");
            return CU_TR_FILTER_MODE_POINT;
    }
}

static CUfilter_mode getCudaMipmapFilterFormat(VkSamplerMipmapMode samplerMipmapMode) {
    switch (samplerMipmapMode) {
        case VK_SAMPLER_MIPMAP_MODE_NEAREST:
            return CU_TR_FILTER_MODE_POINT;
        case VK_SAMPLER_MIPMAP_MODE_LINEAR:
            return CU_TR_FILTER_MODE_LINEAR;
        default:
            sgl::Logfile::get()->throwError("Error in getCudaMipmapFilterFormat: Unsupported filter format.");
            return CU_TR_FILTER_MODE_POINT;
    }
}

static std::array<float, 4> getCudaBorderColor(VkBorderColor borderColor) {
    switch (borderColor) {
        case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
        case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
        case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
            return { 0.0f, 0.0f, 0.0f, 1.0f };
        case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
        case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
            return { 1.0f, 1.0f, 1.0f, 1.0f };
        default:
            sgl::Logfile::get()->throwError("Error in getCudaBorderColor: Unsupported border color.");
            return { 0.0f, 0.0f, 0.0f, 0.0f };
    }
}

TextureCudaExternalMemoryVk::TextureCudaExternalMemoryVk(vk::TexturePtr& vulkanTexture)
        : TextureCudaExternalMemoryVk(
                vulkanTexture->getImage(), vulkanTexture->getImageSampler()->getImageSamplerSettings(),
                vulkanTexture->getImageView()->getVkImageViewType(),
                vulkanTexture->getImageView()->getVkImageSubresourceRange()){
}

TextureCudaExternalMemoryVk::TextureCudaExternalMemoryVk(
        vk::ImagePtr& vulkanImage, const ImageSamplerSettings& samplerSettings)
        : TextureCudaExternalMemoryVk(
                vulkanImage, samplerSettings, VkImageViewType(vulkanImage->getImageSettings().imageType),
                VkImageSubresourceRange{
                    VK_IMAGE_ASPECT_COLOR_BIT, 0, vulkanImage->getImageSettings().mipLevels,
                    0, vulkanImage->getImageSettings().arrayLayers }){
}

TextureCudaExternalMemoryVk::TextureCudaExternalMemoryVk(
        vk::ImagePtr& vulkanImage, const ImageSamplerSettings& samplerSettings,
        VkImageViewType imageViewType)
        : TextureCudaExternalMemoryVk(
                vulkanImage, samplerSettings, imageViewType,
                VkImageSubresourceRange{
                    VK_IMAGE_ASPECT_COLOR_BIT, 0, vulkanImage->getImageSettings().mipLevels,
                    0, vulkanImage->getImageSettings().arrayLayers }) {
}

TextureCudaExternalMemoryVk::TextureCudaExternalMemoryVk(
        vk::ImagePtr& vulkanImage, const ImageSamplerSettings& samplerSettings,
        VkImageViewType imageViewType, VkImageSubresourceRange imageSubresourceRange) {
    imageCudaExternalMemory = std::make_shared<ImageCudaExternalMemoryVk>(vulkanImage, imageViewType, false);
    const auto& imageSettings = vulkanImage->getImageSettings();

    CUDA_RESOURCE_DESC cudaResourceDesc{};
    cudaResourceDesc.resType = CU_RESOURCE_TYPE_MIPMAPPED_ARRAY;
    cudaResourceDesc.res.mipmap.hMipmappedArray = imageCudaExternalMemory->getCudaMipmappedArray();

    CUDA_TEXTURE_DESC cudaTextureDesc{};
    cudaTextureDesc.addressMode[0] = getCudaSamplerAddressMode(samplerSettings.addressModeU);
    cudaTextureDesc.addressMode[1] = getCudaSamplerAddressMode(samplerSettings.addressModeV);
    cudaTextureDesc.addressMode[2] = getCudaSamplerAddressMode(samplerSettings.addressModeW);
    cudaTextureDesc.filterMode = getCudaFilterFormat(samplerSettings.minFilter);
    cudaTextureDesc.mipmapFilterMode = getCudaMipmapFilterFormat(samplerSettings.mipmapMode);
    cudaTextureDesc.mipmapLevelBias = samplerSettings.mipLodBias;
    uint32_t maxAnisotropy = 0;
    if (samplerSettings.anisotropyEnable) {
        if (samplerSettings.maxAnisotropy < 0.0f) {
            auto* device = vulkanImage->getDevice();
            maxAnisotropy = uint32_t(device->getPhysicalDeviceProperties().limits.maxSamplerAnisotropy);
        } else {
            maxAnisotropy = uint32_t(samplerSettings.maxAnisotropy);
        }
    }
    cudaTextureDesc.maxAnisotropy = maxAnisotropy;
    cudaTextureDesc.minMipmapLevelClamp = imageSettings.mipLevels <= 1 ? 0.0f : samplerSettings.minLod;
    cudaTextureDesc.maxMipmapLevelClamp = imageSettings.mipLevels <= 1 ? 0.0f : samplerSettings.maxLod;
    std::array<float, 4> borderColor = getCudaBorderColor(samplerSettings.borderColor);
    memcpy(cudaTextureDesc.borderColor, borderColor.data(), sizeof(float) * 4);

    CUDA_RESOURCE_VIEW_DESC cudaResourceViewDesc{};
    cudaResourceViewDesc.format = getCudaResourceViewFormat(imageSettings.format);
    cudaResourceViewDesc.width = imageSettings.width;
    if (imageViewType == VK_IMAGE_VIEW_TYPE_2D || imageViewType == VK_IMAGE_VIEW_TYPE_3D
            || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY
            || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        cudaResourceViewDesc.height = imageSettings.height;
    }
    if (imageViewType == VK_IMAGE_VIEW_TYPE_3D) {
        cudaResourceViewDesc.depth = imageSettings.depth;
    } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE || imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY
            || imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        cudaResourceViewDesc.depth = imageSettings.arrayLayers;
    }
    cudaResourceViewDesc.firstMipmapLevel = imageSubresourceRange.baseMipLevel;
    cudaResourceViewDesc.lastMipmapLevel = imageSettings.mipLevels <= 1 ? 0 : imageSubresourceRange.levelCount;
    cudaResourceViewDesc.firstLayer = imageSubresourceRange.baseArrayLayer;
    cudaResourceViewDesc.lastLayer = imageSettings.arrayLayers <= 1 ? 0 : imageSubresourceRange.layerCount;

    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuTexObjectCreate(
            &cudaTextureObject, &cudaResourceDesc, &cudaTextureDesc, &cudaResourceViewDesc);
    checkCUresult(cuResult, "Error in cuTexObjectCreate: ");
}

TextureCudaExternalMemoryVk::~TextureCudaExternalMemoryVk() {
    if (cudaTextureObject) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuTexObjectDestroy(cudaTextureObject);
        checkCUresult(cuResult, "Error in cuTexObjectDestroy: ");
    }
}


SurfaceCudaExternalMemoryVk::SurfaceCudaExternalMemoryVk(vk::ImagePtr& vulkanImage, VkImageViewType imageViewType) {
    imageCudaExternalMemory = std::make_shared<ImageCudaExternalMemoryVk>(vulkanImage, imageViewType, true);

    CUDA_RESOURCE_DESC cudaResourceDesc{};
    cudaResourceDesc.resType = CU_RESOURCE_TYPE_MIPMAPPED_ARRAY;
    cudaResourceDesc.res.mipmap.hMipmappedArray = imageCudaExternalMemory->getCudaMipmappedArray();

    CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSurfObjectCreate(&cudaSurfaceObject, &cudaResourceDesc);
    checkCUresult(cuResult, "Error in cuSurfObjectDestroy: ");
}

SurfaceCudaExternalMemoryVk::SurfaceCudaExternalMemoryVk(vk::ImageViewPtr& vulkanImageView)
        : SurfaceCudaExternalMemoryVk(vulkanImageView->getImage(), vulkanImageView->getVkImageViewType()) {
}

SurfaceCudaExternalMemoryVk::~SurfaceCudaExternalMemoryVk() {
    if (cudaSurfaceObject) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuSurfObjectDestroy(cudaSurfaceObject);
        checkCUresult(cuResult, "Error in cuSurfObjectDestroy: ");
    }
}

}}
