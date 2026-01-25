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

#include <Utils/StringUtils.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include "InteropCuda.hpp"

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace sgl {

CudaDeviceApiFunctionTable g_cudaDeviceApiFunctionTable{};
NvrtcFunctionTable g_nvrtcFunctionTable{};

#ifdef _WIN32
HMODULE g_cudaLibraryHandle = nullptr;
HMODULE g_nvrtcLibraryHandle = nullptr;
#define dlsym GetProcAddress
#else
void* g_cudaLibraryHandle = nullptr;
void* g_nvrtcLibraryHandle = nullptr;
#endif

bool initializeCudaDeviceApiFunctionTable() {
    typedef CUresult ( *PFN_cuInit )( unsigned int Flags );
    typedef CUresult ( *PFN_cuGetErrorString )( CUresult error, const char **pStr );
    typedef CUresult ( *PFN_cuDeviceGet )(CUdevice *device, int ordinal);
    typedef CUresult ( *PFN_cuDeviceGetCount )(int *count);
    typedef CUresult ( *PFN_cuDeviceGetUuid )(CUuuid *uuid, CUdevice dev);
    typedef CUresult ( *PFN_cuDeviceGetLuid )(char *luid, unsigned int *deviceNodeMask, CUdevice dev);
    typedef CUresult ( *PFN_cuDeviceGetAttribute )(int *pi, CUdevice_attribute attrib, CUdevice dev);
    typedef CUresult ( *PFN_cuCtxCreate )( CUcontext *pctx, unsigned int flags, CUdevice dev );
    typedef CUresult ( *PFN_cuCtxDestroy )( CUcontext ctx );
    typedef CUresult ( *PFN_cuCtxGetCurrent )( CUcontext* pctx );
    typedef CUresult ( *PFN_cuCtxGetDevice )( CUdevice* device );
    typedef CUresult ( *PFN_cuCtxSetCurrent )( CUcontext ctx );
    typedef CUresult ( *PFN_cuCtxPushCurrent )( CUcontext ctx );
    typedef CUresult ( *PFN_cuCtxPopCurrent )( CUcontext* pctx );
    typedef CUresult ( *PFN_cuDevicePrimaryCtxRetain )( CUcontext pctx, CUdevice dev );
    typedef CUresult ( *PFN_cuDevicePrimaryCtxRelease )( CUdevice dev );
    typedef CUresult ( *PFN_cuDevicePrimaryCtxReset )( CUdevice dev );
    typedef CUresult ( *PFN_cuStreamCreate )( CUstream *phStream, unsigned int Flags );
    typedef CUresult ( *PFN_cuStreamDestroy )( CUstream hStream );
    typedef CUresult ( *PFN_cuStreamSynchronize )( CUstream hStream );
    typedef CUresult ( *PFN_cuMemAlloc )( CUdeviceptr *dptr, size_t bytesize );
    typedef CUresult ( *PFN_cuMemFree )( CUdeviceptr dptr );
    typedef CUresult ( *PFN_cuMemcpyDtoH )( void *dstHost, CUdeviceptr srcDevice, size_t ByteCount );
    typedef CUresult ( *PFN_cuMemcpyHtoD )( CUdeviceptr dstDevice, const void *srcHost, size_t ByteCount );
    typedef CUresult ( *PFN_cuMemAllocAsync )( CUdeviceptr *dptr, size_t bytesize, CUstream hStream );
    typedef CUresult ( *PFN_cuMemFreeAsync )( CUdeviceptr dptr, CUstream hStream );
    typedef CUresult ( *PFN_cuMemsetD8Async )( CUdeviceptr dstDevice, unsigned char uc, size_t N, CUstream hStream );
    typedef CUresult ( *PFN_cuMemsetD16Async )( CUdeviceptr dstDevice, unsigned short us, size_t N, CUstream hStream );
    typedef CUresult ( *PFN_cuMemsetD32Async )( CUdeviceptr dstDevice, unsigned int ui, size_t N, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpyAsync )( CUdeviceptr dst, CUdeviceptr src, size_t ByteCount, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpyDtoHAsync )( void *dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpyHtoDAsync )( CUdeviceptr dstDevice, const void *srcHost, size_t ByteCount, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpy2DAsync )( const CUDA_MEMCPY2D* pCopy, CUstream hStream );
    typedef CUresult ( *PFN_cuMemcpy3DAsync )( const CUDA_MEMCPY3D* pCopy, CUstream hStream );
    typedef CUresult ( *PFN_cuArrayCreate )( CUarray *pHandle, const CUDA_ARRAY_DESCRIPTOR *pAllocateArray );
    typedef CUresult ( *PFN_cuArray3DCreate )( CUarray *pHandle, const CUDA_ARRAY3D_DESCRIPTOR *pAllocateArray );
    typedef CUresult ( *PFN_cuArrayDestroy )( CUarray hArray );
    typedef CUresult ( *PFN_cuMipmappedArrayCreate )( CUmipmappedArray *pHandle, const CUDA_ARRAY3D_DESCRIPTOR *pMipmappedArrayDesc, unsigned int numMipmapLevels );
    typedef CUresult ( *PFN_cuMipmappedArrayDestroy )( CUmipmappedArray hMipmappedArray );
    typedef CUresult ( *PFN_cuMipmappedArrayGetLevel )( CUarray* pLevelArray, CUmipmappedArray hMipmappedArray, unsigned int level );
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
    typedef CUresult ( *PFN_cuModuleLoad )( CUmodule* module, const char* fname );
    typedef CUresult ( *PFN_cuModuleLoadData )( CUmodule* module, const void* image );
    typedef CUresult ( *PFN_cuModuleLoadDataEx )( CUmodule* module, const void* image, unsigned int numOptions, CUjit_option* options, void** optionValues );
    typedef CUresult ( *PFN_cuModuleLoadFatBinary )( CUmodule* module, const void* fatCubin );
    typedef CUresult ( *PFN_cuModuleUnload )( CUmodule hmod );
    typedef CUresult ( *PFN_cuModuleGetFunction )( CUfunction* hfunc, CUmodule hmod, const char* name );
    typedef CUresult ( *PFN_cuModuleGetGlobal )( CUdeviceptr* dptr, size_t* bytes, CUmodule hmod, const char* name );
    typedef CUresult ( *PFN_cuLaunchKernel )( CUfunction f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ, unsigned int sharedMemBytes, CUstream hStream, void** kernelParams, void** extra );
    typedef CUresult ( *PFN_cuOccupancyMaxPotentialBlockSize )( int *minGridSize, int *blockSize, CUfunction func, CUoccupancyB2DSize blockSizeToDynamicSMemSize, size_t dynamicSMemSize, int blockSizeLimit );

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
    g_cudaDeviceApiFunctionTable.cuDeviceGetLuid = PFN_cuDeviceGetLuid(dlsym(g_cudaLibraryHandle, TOSTRING(cuDeviceGetLuid)));
    g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute = PFN_cuDeviceGetAttribute(dlsym(g_cudaLibraryHandle, TOSTRING(cuDeviceGetAttribute)));
    g_cudaDeviceApiFunctionTable.cuCtxCreate = PFN_cuCtxCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxCreate)));
    g_cudaDeviceApiFunctionTable.cuCtxDestroy = PFN_cuCtxDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxDestroy)));
    g_cudaDeviceApiFunctionTable.cuCtxGetCurrent = PFN_cuCtxGetCurrent(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxGetCurrent)));
    g_cudaDeviceApiFunctionTable.cuCtxGetDevice = PFN_cuCtxGetDevice(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxGetDevice)));
    g_cudaDeviceApiFunctionTable.cuCtxSetCurrent = PFN_cuCtxSetCurrent(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxSetCurrent)));
    g_cudaDeviceApiFunctionTable.cuCtxPushCurrent = PFN_cuCtxPushCurrent(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxPushCurrent)));
    g_cudaDeviceApiFunctionTable.cuCtxPopCurrent = PFN_cuCtxPopCurrent(dlsym(g_cudaLibraryHandle, TOSTRING(cuCtxPopCurrent)));
    g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxRetain = PFN_cuDevicePrimaryCtxRetain(dlsym(g_cudaLibraryHandle, TOSTRING(cuDevicePrimaryCtxRetain)));
    g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxRelease = PFN_cuDevicePrimaryCtxRelease(dlsym(g_cudaLibraryHandle, TOSTRING(cuDevicePrimaryCtxRelease)));
    g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxReset = PFN_cuDevicePrimaryCtxReset(dlsym(g_cudaLibraryHandle, TOSTRING(cuDevicePrimaryCtxReset)));
    g_cudaDeviceApiFunctionTable.cuStreamCreate = PFN_cuStreamCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuStreamCreate)));
    g_cudaDeviceApiFunctionTable.cuStreamDestroy = PFN_cuStreamDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuStreamDestroy)));
    g_cudaDeviceApiFunctionTable.cuStreamSynchronize = PFN_cuStreamSynchronize(dlsym(g_cudaLibraryHandle, TOSTRING(cuStreamSynchronize)));
    g_cudaDeviceApiFunctionTable.cuMemAlloc = PFN_cuMemAlloc(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemAlloc)));
    g_cudaDeviceApiFunctionTable.cuMemFree = PFN_cuMemFree(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemFree)));
    g_cudaDeviceApiFunctionTable.cuMemcpyDtoH = PFN_cuMemcpyDtoH(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyDtoH)));
    g_cudaDeviceApiFunctionTable.cuMemcpyHtoD = PFN_cuMemcpyHtoD(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyHtoD)));
    g_cudaDeviceApiFunctionTable.cuMemAllocAsync = PFN_cuMemAllocAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemAllocAsync)));
    g_cudaDeviceApiFunctionTable.cuMemFreeAsync = PFN_cuMemFreeAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemFreeAsync)));
    g_cudaDeviceApiFunctionTable.cuMemsetD8Async = PFN_cuMemsetD8Async(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemsetD8Async)));
    g_cudaDeviceApiFunctionTable.cuMemsetD16Async = PFN_cuMemsetD16Async(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemsetD16Async)));
    g_cudaDeviceApiFunctionTable.cuMemsetD32Async = PFN_cuMemsetD32Async(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemsetD32Async)));
    g_cudaDeviceApiFunctionTable.cuMemcpyAsync = PFN_cuMemcpyAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyAsync)));
    g_cudaDeviceApiFunctionTable.cuMemcpyDtoHAsync = PFN_cuMemcpyDtoHAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyDtoHAsync)));
    g_cudaDeviceApiFunctionTable.cuMemcpyHtoDAsync = PFN_cuMemcpyHtoDAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpyHtoDAsync)));
    g_cudaDeviceApiFunctionTable.cuMemcpy2DAsync = PFN_cuMemcpy2DAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpy2DAsync)));
    g_cudaDeviceApiFunctionTable.cuMemcpy3DAsync = PFN_cuMemcpy3DAsync(dlsym(g_cudaLibraryHandle, TOSTRING(cuMemcpy3DAsync)));
    g_cudaDeviceApiFunctionTable.cuArrayCreate = PFN_cuArrayCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuArrayCreate)));
    g_cudaDeviceApiFunctionTable.cuArray3DCreate = PFN_cuArray3DCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuArray3DCreate)));
    g_cudaDeviceApiFunctionTable.cuArrayDestroy = PFN_cuArrayDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuArrayDestroy)));
    g_cudaDeviceApiFunctionTable.cuMipmappedArrayCreate = PFN_cuMipmappedArrayCreate(dlsym(g_cudaLibraryHandle, TOSTRING(cuMipmappedArrayCreate)));
    g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy = PFN_cuMipmappedArrayDestroy(dlsym(g_cudaLibraryHandle, TOSTRING(cuMipmappedArrayDestroy)));
    g_cudaDeviceApiFunctionTable.cuMipmappedArrayGetLevel = PFN_cuMipmappedArrayGetLevel(dlsym(g_cudaLibraryHandle, TOSTRING(cuMipmappedArrayGetLevel)));
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
    g_cudaDeviceApiFunctionTable.cuModuleLoad = PFN_cuModuleLoad(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleLoad)));
    g_cudaDeviceApiFunctionTable.cuModuleLoadData = PFN_cuModuleLoadData(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleLoadData)));
    g_cudaDeviceApiFunctionTable.cuModuleLoadDataEx = PFN_cuModuleLoadDataEx(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleLoadDataEx)));
    g_cudaDeviceApiFunctionTable.cuModuleLoadFatBinary = PFN_cuModuleLoadFatBinary(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleLoadFatBinary)));
    g_cudaDeviceApiFunctionTable.cuModuleUnload = PFN_cuModuleUnload(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleUnload)));
    g_cudaDeviceApiFunctionTable.cuModuleGetFunction = PFN_cuModuleGetFunction(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleGetFunction)));
    g_cudaDeviceApiFunctionTable.cuModuleGetGlobal = PFN_cuModuleGetGlobal(dlsym(g_cudaLibraryHandle, TOSTRING(cuModuleGetGlobal)));
    g_cudaDeviceApiFunctionTable.cuLaunchKernel = PFN_cuLaunchKernel(dlsym(g_cudaLibraryHandle, TOSTRING(cuLaunchKernel)));
    g_cudaDeviceApiFunctionTable.cuOccupancyMaxPotentialBlockSize = PFN_cuOccupancyMaxPotentialBlockSize(dlsym(g_cudaLibraryHandle, TOSTRING(cuOccupancyMaxPotentialBlockSize)));

    if (!g_cudaDeviceApiFunctionTable.cuInit
            || !g_cudaDeviceApiFunctionTable.cuGetErrorString
            || !g_cudaDeviceApiFunctionTable.cuDeviceGet
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetCount
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetUuid
#ifdef _WIN32
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetLuid
#endif
            || !g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute
            || !g_cudaDeviceApiFunctionTable.cuCtxCreate
            || !g_cudaDeviceApiFunctionTable.cuCtxDestroy
            || !g_cudaDeviceApiFunctionTable.cuCtxGetCurrent
            || !g_cudaDeviceApiFunctionTable.cuCtxGetDevice
            || !g_cudaDeviceApiFunctionTable.cuCtxSetCurrent
            || !g_cudaDeviceApiFunctionTable.cuCtxPushCurrent
            || !g_cudaDeviceApiFunctionTable.cuCtxPopCurrent
            //|| !g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxRetain
            //|| !g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxRelease
            //|| !g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxReset
            || !g_cudaDeviceApiFunctionTable.cuStreamCreate
            || !g_cudaDeviceApiFunctionTable.cuStreamDestroy
            || !g_cudaDeviceApiFunctionTable.cuStreamSynchronize
            || !g_cudaDeviceApiFunctionTable.cuMemAlloc
            || !g_cudaDeviceApiFunctionTable.cuMemFree
            || !g_cudaDeviceApiFunctionTable.cuMemcpyDtoH
            || !g_cudaDeviceApiFunctionTable.cuMemcpyHtoD
            || !g_cudaDeviceApiFunctionTable.cuMemAllocAsync
            || !g_cudaDeviceApiFunctionTable.cuMemFreeAsync
            || !g_cudaDeviceApiFunctionTable.cuMemsetD8Async
            || !g_cudaDeviceApiFunctionTable.cuMemsetD16Async
            || !g_cudaDeviceApiFunctionTable.cuMemsetD32Async
            || !g_cudaDeviceApiFunctionTable.cuMemcpyAsync
            || !g_cudaDeviceApiFunctionTable.cuMemcpyDtoHAsync
            || !g_cudaDeviceApiFunctionTable.cuMemcpyHtoDAsync
            || !g_cudaDeviceApiFunctionTable.cuMemcpy2DAsync
            || !g_cudaDeviceApiFunctionTable.cuMemcpy3DAsync
            || !g_cudaDeviceApiFunctionTable.cuArrayCreate
            || !g_cudaDeviceApiFunctionTable.cuArray3DCreate
            || !g_cudaDeviceApiFunctionTable.cuArrayDestroy
            || !g_cudaDeviceApiFunctionTable.cuMipmappedArrayCreate
            || !g_cudaDeviceApiFunctionTable.cuMipmappedArrayDestroy
            || !g_cudaDeviceApiFunctionTable.cuMipmappedArrayGetLevel
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
            || !g_cudaDeviceApiFunctionTable.cuDestroyExternalSemaphore
            || !g_cudaDeviceApiFunctionTable.cuModuleLoad
            || !g_cudaDeviceApiFunctionTable.cuModuleLoadData
            || !g_cudaDeviceApiFunctionTable.cuModuleLoadDataEx
            || !g_cudaDeviceApiFunctionTable.cuModuleLoadFatBinary
            || !g_cudaDeviceApiFunctionTable.cuModuleUnload
            || !g_cudaDeviceApiFunctionTable.cuModuleGetFunction
            || !g_cudaDeviceApiFunctionTable.cuModuleGetGlobal
            || !g_cudaDeviceApiFunctionTable.cuLaunchKernel
            || !g_cudaDeviceApiFunctionTable.cuOccupancyMaxPotentialBlockSize) {
        sgl::Logfile::get()->throwError(
                "Error in initializeCudaDeviceApiFunctionTable: "
                "At least one function pointer could not be loaded.");
    }

    return true;
}

bool initializeNvrtcFunctionTable() {
    typedef const char* ( *PFN_nvrtcGetErrorString )( nvrtcResult result );
    typedef nvrtcResult ( *PFN_nvrtcCreateProgram )( nvrtcProgram* prog, const char* src, const char* name, int numHeaders, const char* const* headers, const char* const* includeNames );
    typedef nvrtcResult ( *PFN_nvrtcDestroyProgram )( nvrtcProgram* prog );
    typedef nvrtcResult ( *PFN_nvrtcCompileProgram )( nvrtcProgram prog, int numOptions, const char* const* options );
    typedef nvrtcResult ( *PFN_nvrtcGetProgramLogSize )( nvrtcProgram prog, size_t* logSizeRet );
    typedef nvrtcResult ( *PFN_nvrtcGetProgramLog )( nvrtcProgram prog, char* log );
    typedef nvrtcResult ( *PFN_nvrtcGetPTXSize )( nvrtcProgram prog, size_t* ptxSizeRet );
    typedef nvrtcResult ( *PFN_nvrtcGetPTX )( nvrtcProgram prog, char* ptx );

#if defined(__linux__)
    g_nvrtcLibraryHandle = dlopen("libnvrtc.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_nvrtcLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeNvrtcFunctionTable: Could not load libnvrtc.so.");
        return false;
    }
#elif defined(_WIN32)
    std::vector<std::string> pathList;
#ifdef _MSC_VER
    char* pathEnvVar = nullptr;
    size_t stringSize = 0;
    if (_dupenv_s(&pathEnvVar, &stringSize, "PATH") != 0) {
        pathEnvVar = nullptr;
    }
    sgl::splitString(pathEnvVar, ';', pathList);
    free(pathEnvVar);
    pathEnvVar = nullptr;
#else
    const char* pathEnvVar = getenv("PATH");
    sgl::splitString(pathEnvVar, ';', pathList);
#endif

    std::string nvrtcDllFileName;
    for (const std::string& pathDir : pathList) {
        if (!sgl::FileUtils::get()->isDirectory(pathDir)) {
            continue;
        }
        std::vector<std::string> filesInDir = sgl::FileUtils::get()->getFilesInDirectoryVector(pathDir);
        for (const std::string& fileInDir : filesInDir) {
            std::string fileName = sgl::FileUtils::get()->getPureFilename(fileInDir);
            if (sgl::startsWith(fileName, "nvrtc64_") && sgl::endsWith(fileName, ".dll")) {
                nvrtcDllFileName = fileName;
            }
        }
    }
    if (nvrtcDllFileName.empty()) {
        sgl::Logfile::get()->writeInfo("initializeNvrtcFunctionTable: Could not find nvrtc.dll.");
        return false;
    }

    g_nvrtcLibraryHandle = LoadLibraryA(nvrtcDllFileName.c_str());
    if (!g_nvrtcLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeNvrtcFunctionTable: Could not load " + nvrtcDllFileName + ".");
        return false;
    }
#endif

    g_nvrtcFunctionTable.nvrtcGetErrorString = PFN_nvrtcGetErrorString(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcGetErrorString)));
    g_nvrtcFunctionTable.nvrtcCreateProgram = PFN_nvrtcCreateProgram(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcCreateProgram)));
    g_nvrtcFunctionTable.nvrtcDestroyProgram = PFN_nvrtcDestroyProgram(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcDestroyProgram)));
    g_nvrtcFunctionTable.nvrtcCompileProgram = PFN_nvrtcCompileProgram(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcCompileProgram)));
    g_nvrtcFunctionTable.nvrtcGetProgramLogSize = PFN_nvrtcGetProgramLogSize(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcGetProgramLogSize)));
    g_nvrtcFunctionTable.nvrtcGetProgramLog = PFN_nvrtcGetProgramLog(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcGetProgramLog)));
    g_nvrtcFunctionTable.nvrtcGetPTXSize = PFN_nvrtcGetPTXSize(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcGetPTXSize)));
    g_nvrtcFunctionTable.nvrtcGetPTX = PFN_nvrtcGetPTX(dlsym(g_nvrtcLibraryHandle, TOSTRING(nvrtcGetPTX)));

    if (!g_nvrtcFunctionTable.nvrtcGetErrorString
            || !g_nvrtcFunctionTable.nvrtcCreateProgram
            || !g_nvrtcFunctionTable.nvrtcDestroyProgram
            || !g_nvrtcFunctionTable.nvrtcCompileProgram
            || !g_nvrtcFunctionTable.nvrtcGetProgramLogSize
            || !g_nvrtcFunctionTable.nvrtcGetProgramLog
            || !g_nvrtcFunctionTable.nvrtcGetPTXSize
            || !g_nvrtcFunctionTable.nvrtcGetPTX) {
        sgl::Logfile::get()->throwError(
                "Error in initializeNvrtcFunctionTable: "
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

bool getIsNvrtcFunctionTableInitialized() {
    return g_nvrtcLibraryHandle != nullptr;
}

void freeNvrtcFunctionTable() {
    if (g_nvrtcLibraryHandle) {
#if defined(__linux__)
        dlclose(g_nvrtcLibraryHandle);
#elif defined(_WIN32)
        FreeLibrary(g_nvrtcLibraryHandle);
#endif
        g_nvrtcLibraryHandle = {};
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

void _checkNvrtcResult(nvrtcResult result, const char* text, const char* locationText) {
    if (result != NVRTC_SUCCESS) {
        throw std::runtime_error(
                std::string() + locationText + ": " + text + g_nvrtcFunctionTable.nvrtcGetErrorString(result));
    }
}

}
