/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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
#include "InteropHIP.hpp"

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

HipDeviceApiFunctionTable g_hipDeviceApiFunctionTable{};
HiprtcFunctionTable g_hiprtcFunctionTable{};

#ifdef _WIN32
HMODULE g_hipLibraryHandle = nullptr;
HMODULE g_hiprtcLibraryHandle = nullptr;
#define dlsym GetProcAddress
#else
void* g_hipLibraryHandle = nullptr;
void* g_hiprtcLibraryHandle = nullptr;
#endif

bool initializeHipDeviceApiFunctionTable() {
    typedef hipError_t ( *PFN_hipInit )( unsigned int Flags );
    typedef hipError_t ( *PFN_hipDrvGetErrorString)( hipError_t error, const char **pStr );
    typedef hipError_t ( *PFN_hipDeviceGet )(hipDevice_t *device, int ordinal);
    typedef hipError_t ( *PFN_hipGetDeviceCount )(int *count);
    typedef hipError_t ( *PFN_hipDeviceGetUuid )(hipUUID *uuid, hipDevice_t dev);
    typedef hipError_t ( *PFN_hipDeviceGetAttribute )(int *pi, hipDeviceAttribute_t attrib, hipDevice_t dev);
    typedef hipError_t ( *PFN_hipGetDeviceProperties )(hipDeviceProp_t* prop, int deviceId);
    typedef hipError_t ( *PFN_hipCtxCreate )( hipCtx_t *pctx, unsigned int flags, hipDevice_t dev );
    typedef hipError_t ( *PFN_hipCtxDestroy )( hipCtx_t ctx );
    typedef hipError_t ( *PFN_hipCtxGetCurrent )( hipCtx_t* pctx );
    typedef hipError_t ( *PFN_hipCtxGetDevice )( hipDevice_t* device );
    typedef hipError_t ( *PFN_hipCtxSetCurrent )( hipCtx_t ctx );
    typedef hipError_t ( *PFN_hipStreamCreate )( hipStream_t *phStream, unsigned int Flags );
    typedef hipError_t ( *PFN_hipStreamDestroy )( hipStream_t hStream );
    typedef hipError_t ( *PFN_hipStreamSynchronize )( hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMalloc )( hipDeviceptr_t *dptr, size_t bytesize );
    typedef hipError_t ( *PFN_hipFree )( hipDeviceptr_t dptr );
    typedef hipError_t ( *PFN_hipMemcpyDtoH )( void *dstHost, hipDeviceptr_t srcDevice, size_t ByteCount );
    typedef hipError_t ( *PFN_hipMemcpyHtoD )( hipDeviceptr_t dstDevice, const void *srcHost, size_t ByteCount );
    typedef hipError_t ( *PFN_hipMallocAsync )( hipDeviceptr_t *dptr, size_t bytesize, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipFreeAsync )( hipDeviceptr_t dptr, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemsetD8Async )( hipDeviceptr_t dstDevice, unsigned char uc, size_t N, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemsetD16Async )( hipDeviceptr_t dstDevice, unsigned short us, size_t N, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemsetD32Async )( hipDeviceptr_t dstDevice, unsigned int ui, size_t N, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemcpyAsync )( hipDeviceptr_t dst, hipDeviceptr_t src, size_t ByteCount, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemcpyDtoHAsync )( void *dstHost, hipDeviceptr_t srcDevice, size_t ByteCount, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemcpyHtoDAsync )( hipDeviceptr_t dstDevice, const void *srcHost, size_t ByteCount, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipMemcpy2DToArrayAsync )( hipArray_t dst, size_t wOffset, size_t hOffset, const void* src, size_t spitch, size_t width, size_t height, hipMemcpyKind kind, hipStream_t stream );
    typedef hipError_t ( *PFN_hipMemcpy2DFromArrayAsync )( void* dst, size_t dpitch, hipArray_const_t src, size_t wOffset, size_t hOffset, size_t width, size_t height, hipMemcpyKind kind, hipStream_t stream );
    typedef hipError_t ( *PFN_hipDrvMemcpy3DAsync )( const HIP_MEMCPY3D* pCopy, hipStream_t hStream );
    typedef hipError_t ( *PFN_hipArrayCreate )( hipArray *pHandle, const HIP_ARRAY_DESCRIPTOR *pAllocateArray );
    typedef hipError_t ( *PFN_hipArray3DCreate )( hipArray *pHandle, const HIP_ARRAY3D_DESCRIPTOR *pAllocateArray );
    typedef hipError_t ( *PFN_hipArrayDestroy )( hipArray hArray );
    typedef hipError_t ( *PFN_hipMipmappedArrayCreate )( hipMipmappedArray_t *pHandle, const HIP_ARRAY3D_DESCRIPTOR *pMipmappedArrayDesc, unsigned int numMipmapLevels );
    typedef hipError_t ( *PFN_hipMipmappedArrayDestroy )( hipMipmappedArray_t hMipmappedArray );
    typedef hipError_t ( *PFN_hipMipmappedArrayGetLevel )( hipArray_t* pLevelArray, hipMipmappedArray_t hMipmappedArray, unsigned int level );
    typedef hipError_t ( *PFN_hipTexObjectCreate )( hipTextureObject_t *pTexObject, const HIP_RESOURCE_DESC *pResDesc, const HIP_TEXTURE_DESC *pTexDesc, const HIP_RESOURCE_VIEW_DESC *pResViewDesc );
    typedef hipError_t ( *PFN_hipTexObjectDestroy )( hipTextureObject_t texObject );
    typedef hipError_t ( *PFN_hipCreateTextureObject )( hipTextureObject_t* pTexObject, const hipResourceDesc* pResDesc, const hipTextureDesc* pTexDesc, const hipResourceViewDesc* pResViewDesc );
    typedef hipError_t ( *PFN_hipDestroyTextureObject )( hipTextureObject_t textureObject );
    typedef hipError_t ( *PFN_hipCreateSurfaceObject )( hipSurfaceObject_t* pSurfObject, const hipResourceDesc* pResDesc );
    typedef hipError_t ( *PFN_hipDestroySurfaceObject )( hipSurfaceObject_t surfaceObject );
    typedef hipError_t ( *PFN_hipImportExternalMemory )( hipExternalMemory_t *extMem_out, const hipExternalMemoryHandleDesc *memHandleDesc );
    typedef hipError_t ( *PFN_hipExternalMemoryGetMappedBuffer )( hipDeviceptr_t *devPtr, hipExternalMemory_t extMem, const hipExternalMemoryBufferDesc *bufferDesc );
    typedef hipError_t ( *PFN_hipExternalMemoryGetMappedMipmappedArray )( hipMipmappedArray_t* mipmap, hipExternalMemory_t extMem, const hipExternalMemoryMipmappedArrayDesc* mipmapDesc );
    typedef hipError_t ( *PFN_hipDestroyExternalMemory )( hipExternalMemory_t extMem );
    typedef hipError_t ( *PFN_hipImportExternalSemaphore )( hipExternalSemaphore_t *extSem_out, const hipExternalSemaphoreHandleDesc *semHandleDesc );
    typedef hipError_t ( *PFN_hipSignalExternalSemaphoresAsync )( const hipExternalSemaphore_t *extSemArray, const hipExternalSemaphoreSignalParams *paramsArray, unsigned int numExtSems, hipStream_t stream );
    typedef hipError_t ( *PFN_hipWaitExternalSemaphoresAsync )( const hipExternalSemaphore_t *extSemArray, const hipExternalSemaphoreWaitParams *paramsArray, unsigned int numExtSems, hipStream_t stream );
    typedef hipError_t ( *PFN_hipDestroyExternalSemaphore )( hipExternalSemaphore_t extSem );
    typedef hipError_t ( *PFN_hipModuleLoad )( hipModule_t* module, const char* fname );
    typedef hipError_t ( *PFN_hipModuleLoadData )( hipModule_t* module, const void* image );
    typedef hipError_t ( *PFN_hipModuleLoadDataEx )( hipModule_t* module, const void* image, unsigned int numOptions, hipJitOption* options, void** optionValues );
    typedef hipError_t ( *PFN_hipModuleUnload )( hipModule_t hmod );
    typedef hipError_t ( *PFN_hipModuleGetFunction )( hipFunction_t* hfunc, hipModule_t hmod, const char* name );
    typedef hipError_t ( *PFN_hipModuleGetGlobal )( hipDeviceptr_t* dptr, size_t* bytes, hipModule_t hmod, const char* name );
    typedef hipError_t ( *PFN_hipLaunchKernel )( hipFunction_t f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ, unsigned int sharedMemBytes, hipStream_t hStream, void** kernelParams, void** extra );
    typedef hipError_t ( *PFN_hipOccupancyMaxPotentialBlockSize )( int *minGridSize, int *blockSize, hipFunction_t func, const void* blockSizeToDynamicSMemSize, size_t dynamicSMemSize, int blockSizeLimit );

#if defined(__linux__)
    g_hipLibraryHandle = dlopen("libamdhip64.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_hipLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeHipDeviceApiFunctionTable: Could not load libamdhip64.so.");
        return false;
    }
#elif defined(_WIN32)
    g_hipLibraryHandle = LoadLibraryA("amdhip64.dll");
    if (!g_hipLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeHipDeviceApiFunctionTable: Could not load amdhip64.dll.");
        return false;
    }
#endif
    g_hipDeviceApiFunctionTable.hipInit = PFN_hipInit(dlsym(g_hipLibraryHandle, TOSTRING(hipInit)));
    g_hipDeviceApiFunctionTable.hipDrvGetErrorString = PFN_hipDrvGetErrorString(dlsym(g_hipLibraryHandle, TOSTRING(hipDrvGetErrorString)));
    g_hipDeviceApiFunctionTable.hipDeviceGet = PFN_hipDeviceGet(dlsym(g_hipLibraryHandle, TOSTRING(hipDeviceGet)));
    g_hipDeviceApiFunctionTable.hipGetDeviceCount = PFN_hipGetDeviceCount(dlsym(g_hipLibraryHandle, TOSTRING(hipGetDeviceCount)));
    g_hipDeviceApiFunctionTable.hipDeviceGetUuid = PFN_hipDeviceGetUuid(dlsym(g_hipLibraryHandle, TOSTRING(hipDeviceGetUuid)));
    g_hipDeviceApiFunctionTable.hipDeviceGetAttribute = PFN_hipDeviceGetAttribute(dlsym(g_hipLibraryHandle, TOSTRING(hipDeviceGetAttribute)));
    g_hipDeviceApiFunctionTable.hipGetDeviceProperties = PFN_hipGetDeviceProperties(dlsym(g_hipLibraryHandle, TOSTRING(hipGetDeviceProperties)));
    g_hipDeviceApiFunctionTable.hipCtxCreate = PFN_hipCtxCreate(dlsym(g_hipLibraryHandle, TOSTRING(hipCtxCreate)));
    g_hipDeviceApiFunctionTable.hipCtxDestroy = PFN_hipCtxDestroy(dlsym(g_hipLibraryHandle, TOSTRING(hipCtxDestroy)));
    g_hipDeviceApiFunctionTable.hipCtxGetCurrent = PFN_hipCtxGetCurrent(dlsym(g_hipLibraryHandle, TOSTRING(hipCtxGetCurrent)));
    g_hipDeviceApiFunctionTable.hipCtxGetDevice = PFN_hipCtxGetDevice(dlsym(g_hipLibraryHandle, TOSTRING(hipCtxGetDevice)));
    g_hipDeviceApiFunctionTable.hipCtxSetCurrent = PFN_hipCtxSetCurrent(dlsym(g_hipLibraryHandle, TOSTRING(hipCtxSetCurrent)));
    g_hipDeviceApiFunctionTable.hipStreamCreate = PFN_hipStreamCreate(dlsym(g_hipLibraryHandle, TOSTRING(hipStreamCreate)));
    g_hipDeviceApiFunctionTable.hipStreamDestroy = PFN_hipStreamDestroy(dlsym(g_hipLibraryHandle, TOSTRING(hipStreamDestroy)));
    g_hipDeviceApiFunctionTable.hipStreamSynchronize = PFN_hipStreamSynchronize(dlsym(g_hipLibraryHandle, TOSTRING(hipStreamSynchronize)));
    g_hipDeviceApiFunctionTable.hipMalloc = PFN_hipMalloc(dlsym(g_hipLibraryHandle, TOSTRING(hipMalloc)));
    g_hipDeviceApiFunctionTable.hipFree = PFN_hipFree(dlsym(g_hipLibraryHandle, TOSTRING(hipFree)));
    g_hipDeviceApiFunctionTable.hipMemcpyDtoH = PFN_hipMemcpyDtoH(dlsym(g_hipLibraryHandle, TOSTRING(hipMemcpyDtoH)));
    g_hipDeviceApiFunctionTable.hipMemcpyHtoD = PFN_hipMemcpyHtoD(dlsym(g_hipLibraryHandle, TOSTRING(hipMemcpyHtoD)));
    g_hipDeviceApiFunctionTable.hipMallocAsync = PFN_hipMallocAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipMallocAsync)));
    g_hipDeviceApiFunctionTable.hipFreeAsync = PFN_hipFreeAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipFreeAsync)));
    g_hipDeviceApiFunctionTable.hipMemsetD8Async = PFN_hipMemsetD8Async(dlsym(g_hipLibraryHandle, TOSTRING(hipMemsetD8Async)));
    g_hipDeviceApiFunctionTable.hipMemsetD16Async = PFN_hipMemsetD16Async(dlsym(g_hipLibraryHandle, TOSTRING(hipMemsetD16Async)));
    g_hipDeviceApiFunctionTable.hipMemsetD32Async = PFN_hipMemsetD32Async(dlsym(g_hipLibraryHandle, TOSTRING(hipMemsetD32Async)));
    g_hipDeviceApiFunctionTable.hipMemcpyAsync = PFN_hipMemcpyAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipMemcpyAsync)));
    g_hipDeviceApiFunctionTable.hipMemcpyDtoHAsync = PFN_hipMemcpyDtoHAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipMemcpyDtoHAsync)));
    g_hipDeviceApiFunctionTable.hipMemcpyHtoDAsync = PFN_hipMemcpyHtoDAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipMemcpyHtoDAsync)));
    g_hipDeviceApiFunctionTable.hipMemcpy2DToArrayAsync = PFN_hipMemcpy2DToArrayAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipDrvMemcpy2DAsync)));
    g_hipDeviceApiFunctionTable.hipMemcpy2DFromArrayAsync = PFN_hipMemcpy2DFromArrayAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipDrvMemcpy2DAsync)));
    g_hipDeviceApiFunctionTable.hipDrvMemcpy3DAsync = PFN_hipDrvMemcpy3DAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipDrvMemcpy3DAsync)));
    g_hipDeviceApiFunctionTable.hipArrayCreate = PFN_hipArrayCreate(dlsym(g_hipLibraryHandle, TOSTRING(hipArrayCreate)));
    g_hipDeviceApiFunctionTable.hipArray3DCreate = PFN_hipArray3DCreate(dlsym(g_hipLibraryHandle, TOSTRING(hipArray3DCreate)));
    g_hipDeviceApiFunctionTable.hipArrayDestroy = PFN_hipArrayDestroy(dlsym(g_hipLibraryHandle, TOSTRING(hipArrayDestroy)));
    g_hipDeviceApiFunctionTable.hipMipmappedArrayCreate = PFN_hipMipmappedArrayCreate(dlsym(g_hipLibraryHandle, TOSTRING(hipMipmappedArrayCreate)));
    g_hipDeviceApiFunctionTable.hipMipmappedArrayDestroy = PFN_hipMipmappedArrayDestroy(dlsym(g_hipLibraryHandle, TOSTRING(hipMipmappedArrayDestroy)));
    g_hipDeviceApiFunctionTable.hipMipmappedArrayGetLevel = PFN_hipMipmappedArrayGetLevel(dlsym(g_hipLibraryHandle, TOSTRING(hipMipmappedArrayGetLevel)));
    g_hipDeviceApiFunctionTable.hipTexObjectCreate = PFN_hipTexObjectCreate(dlsym(g_hipLibraryHandle, TOSTRING(hipTexObjectCreate)));
    g_hipDeviceApiFunctionTable.hipTexObjectDestroy = PFN_hipTexObjectDestroy(dlsym(g_hipLibraryHandle, TOSTRING(hipTexObjectDestroy)));
    g_hipDeviceApiFunctionTable.hipCreateTextureObject = PFN_hipCreateTextureObject(dlsym(g_hipLibraryHandle, TOSTRING(hipCreateTextureObject)));
    g_hipDeviceApiFunctionTable.hipDestroyTextureObject = PFN_hipDestroyTextureObject(dlsym(g_hipLibraryHandle, TOSTRING(hipDestroyTextureObject)));
    g_hipDeviceApiFunctionTable.hipCreateSurfaceObject = PFN_hipCreateSurfaceObject(dlsym(g_hipLibraryHandle, TOSTRING(hipCreateSurfaceObject)));
    g_hipDeviceApiFunctionTable.hipDestroySurfaceObject = PFN_hipDestroySurfaceObject(dlsym(g_hipLibraryHandle, TOSTRING(hipDestroySurfaceObject)));
    g_hipDeviceApiFunctionTable.hipImportExternalMemory = PFN_hipImportExternalMemory(dlsym(g_hipLibraryHandle, TOSTRING(hipImportExternalMemory)));
    g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedBuffer = PFN_hipExternalMemoryGetMappedBuffer(dlsym(g_hipLibraryHandle, TOSTRING(hipExternalMemoryGetMappedBuffer)));
    g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedMipmappedArray = PFN_hipExternalMemoryGetMappedMipmappedArray(dlsym(g_hipLibraryHandle, TOSTRING(hipExternalMemoryGetMappedMipmappedArray)));
    g_hipDeviceApiFunctionTable.hipDestroyExternalMemory = PFN_hipDestroyExternalMemory(dlsym(g_hipLibraryHandle, TOSTRING(hipDestroyExternalMemory)));
    g_hipDeviceApiFunctionTable.hipImportExternalSemaphore = PFN_hipImportExternalSemaphore(dlsym(g_hipLibraryHandle, TOSTRING(hipImportExternalSemaphore)));
    g_hipDeviceApiFunctionTable.hipSignalExternalSemaphoresAsync = PFN_hipSignalExternalSemaphoresAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipSignalExternalSemaphoresAsync)));
    g_hipDeviceApiFunctionTable.hipWaitExternalSemaphoresAsync = PFN_hipWaitExternalSemaphoresAsync(dlsym(g_hipLibraryHandle, TOSTRING(hipWaitExternalSemaphoresAsync)));
    g_hipDeviceApiFunctionTable.hipDestroyExternalSemaphore = PFN_hipDestroyExternalSemaphore(dlsym(g_hipLibraryHandle, TOSTRING(hipDestroyExternalSemaphore)));
    g_hipDeviceApiFunctionTable.hipModuleLoad = PFN_hipModuleLoad(dlsym(g_hipLibraryHandle, TOSTRING(hipModuleLoad)));
    g_hipDeviceApiFunctionTable.hipModuleLoadData = PFN_hipModuleLoadData(dlsym(g_hipLibraryHandle, TOSTRING(hipModuleLoadData)));
    g_hipDeviceApiFunctionTable.hipModuleLoadDataEx = PFN_hipModuleLoadDataEx(dlsym(g_hipLibraryHandle, TOSTRING(hipModuleLoadDataEx)));
    g_hipDeviceApiFunctionTable.hipModuleUnload = PFN_hipModuleUnload(dlsym(g_hipLibraryHandle, TOSTRING(hipModuleUnload)));
    g_hipDeviceApiFunctionTable.hipModuleGetFunction = PFN_hipModuleGetFunction(dlsym(g_hipLibraryHandle, TOSTRING(hipModuleGetFunction)));
    g_hipDeviceApiFunctionTable.hipModuleGetGlobal = PFN_hipModuleGetGlobal(dlsym(g_hipLibraryHandle, TOSTRING(hipModuleGetGlobal)));
    g_hipDeviceApiFunctionTable.hipLaunchKernel = PFN_hipLaunchKernel(dlsym(g_hipLibraryHandle, TOSTRING(hipLaunchKernel)));
    g_hipDeviceApiFunctionTable.hipOccupancyMaxPotentialBlockSize = PFN_hipOccupancyMaxPotentialBlockSize(dlsym(g_hipLibraryHandle, TOSTRING(hipOccupancyMaxPotentialBlockSize)));

    if (!g_hipDeviceApiFunctionTable.hipInit
        || !g_hipDeviceApiFunctionTable.hipDrvGetErrorString
        || !g_hipDeviceApiFunctionTable.hipDeviceGet
        || !g_hipDeviceApiFunctionTable.hipGetDeviceCount
        || !g_hipDeviceApiFunctionTable.hipDeviceGetUuid
        || !g_hipDeviceApiFunctionTable.hipDeviceGetAttribute
        || !g_hipDeviceApiFunctionTable.hipGetDeviceProperties
        || !g_hipDeviceApiFunctionTable.hipCtxCreate
        || !g_hipDeviceApiFunctionTable.hipCtxDestroy
        || !g_hipDeviceApiFunctionTable.hipCtxGetCurrent
        || !g_hipDeviceApiFunctionTable.hipCtxGetDevice
        || !g_hipDeviceApiFunctionTable.hipCtxSetCurrent
        || !g_hipDeviceApiFunctionTable.hipStreamCreate
        || !g_hipDeviceApiFunctionTable.hipStreamDestroy
        || !g_hipDeviceApiFunctionTable.hipStreamSynchronize
        || !g_hipDeviceApiFunctionTable.hipMalloc
        || !g_hipDeviceApiFunctionTable.hipFree
        || !g_hipDeviceApiFunctionTable.hipMemcpyDtoH
        || !g_hipDeviceApiFunctionTable.hipMemcpyHtoD
        || !g_hipDeviceApiFunctionTable.hipMallocAsync
        || !g_hipDeviceApiFunctionTable.hipFreeAsync
        || !g_hipDeviceApiFunctionTable.hipMemsetD8Async
        || !g_hipDeviceApiFunctionTable.hipMemsetD16Async
        || !g_hipDeviceApiFunctionTable.hipMemsetD32Async
        || !g_hipDeviceApiFunctionTable.hipMemcpyAsync
        || !g_hipDeviceApiFunctionTable.hipMemcpyDtoHAsync
        || !g_hipDeviceApiFunctionTable.hipMemcpyHtoDAsync
        || !g_hipDeviceApiFunctionTable.hipMemcpy2DToArrayAsync
        || !g_hipDeviceApiFunctionTable.hipMemcpy2DFromArrayAsync
        || !g_hipDeviceApiFunctionTable.hipDrvMemcpy3DAsync
        || !g_hipDeviceApiFunctionTable.hipArrayCreate
        || !g_hipDeviceApiFunctionTable.hipArray3DCreate
        || !g_hipDeviceApiFunctionTable.hipArrayDestroy
        || !g_hipDeviceApiFunctionTable.hipMipmappedArrayCreate
        || !g_hipDeviceApiFunctionTable.hipMipmappedArrayDestroy
        || !g_hipDeviceApiFunctionTable.hipMipmappedArrayGetLevel
        || !g_hipDeviceApiFunctionTable.hipTexObjectCreate
        || !g_hipDeviceApiFunctionTable.hipTexObjectDestroy
        || !g_hipDeviceApiFunctionTable.hipCreateTextureObject
        || !g_hipDeviceApiFunctionTable.hipDestroyTextureObject
        || !g_hipDeviceApiFunctionTable.hipCreateSurfaceObject
        || !g_hipDeviceApiFunctionTable.hipDestroySurfaceObject
        || !g_hipDeviceApiFunctionTable.hipImportExternalMemory
        || !g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedBuffer
        || !g_hipDeviceApiFunctionTable.hipExternalMemoryGetMappedMipmappedArray
        || !g_hipDeviceApiFunctionTable.hipDestroyExternalMemory
        || !g_hipDeviceApiFunctionTable.hipImportExternalSemaphore
        || !g_hipDeviceApiFunctionTable.hipSignalExternalSemaphoresAsync
        || !g_hipDeviceApiFunctionTable.hipWaitExternalSemaphoresAsync
        || !g_hipDeviceApiFunctionTable.hipDestroyExternalSemaphore
        || !g_hipDeviceApiFunctionTable.hipModuleLoad
        || !g_hipDeviceApiFunctionTable.hipModuleLoadData
        || !g_hipDeviceApiFunctionTable.hipModuleLoadDataEx
        || !g_hipDeviceApiFunctionTable.hipModuleUnload
        || !g_hipDeviceApiFunctionTable.hipModuleGetFunction
        || !g_hipDeviceApiFunctionTable.hipModuleGetGlobal
        || !g_hipDeviceApiFunctionTable.hipLaunchKernel
        || !g_hipDeviceApiFunctionTable.hipOccupancyMaxPotentialBlockSize) {
        sgl::Logfile::get()->throwError(
                "Error in initializeHipDeviceApiFunctionTable: "
                "At least one function pointer could not be loaded.");
    }

    return true;
}

bool initializeHiprtcFunctionTable() {
    typedef const char* ( *PFN_hiprtcGetErrorString )( hiprtcResult result );
    typedef hiprtcResult ( *PFN_hiprtcCreateProgram )( hiprtcProgram* prog, const char* src, const char* name, int numHeaders, const char* const* headers, const char* const* includeNames );
    typedef hiprtcResult ( *PFN_hiprtcDestroyProgram )( hiprtcProgram* prog );
    typedef hiprtcResult ( *PFN_hiprtcCompileProgram )( hiprtcProgram prog, int numOptions, const char* const* options );
    typedef hiprtcResult ( *PFN_hiprtcGetProgramLogSize )( hiprtcProgram prog, size_t* logSizeRet );
    typedef hiprtcResult ( *PFN_hiprtcGetProgramLog )( hiprtcProgram prog, char* log );
    typedef hiprtcResult ( *PFN_hiprtcGetCodeSize )( hiprtcProgram prog, size_t* codeSizeRet );
    typedef hiprtcResult ( *PFN_hiprtcGetCode )( hiprtcProgram prog, char* code );

#if defined(__linux__)
    g_hiprtcLibraryHandle = dlopen("libhiprtc.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_hiprtcLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeHiprtcFunctionTable: Could not load libhiprtc.so.");
        return false;
    }
#elif defined(_WIN32)
    std::vector<std::string> pathList;
#ifdef _MSC_VER
    char* pathEnvVar = nullptr;
    size_t stringSize = 0;
    if (_dupenv_s(&pathEnvVar, &stringSize, "HIP_PATH") != 0) {
        pathEnvVar = nullptr;
    }
    if (pathEnvVar) {
        sgl::splitString(pathEnvVar, ';', pathList);
        free(pathEnvVar);
    }
    pathEnvVar = nullptr;
#else
    const char* pathEnvVar = getenv("HIP_PATH");
    if (pathEnvVar) {
        sgl::splitString(pathEnvVar, ';', pathList);
    }
#endif

    std::string hiprtcDllFileName;
    for (const std::string& pathDir : pathList) {
        std::string binDir = pathDir + "/bin";
        if (!sgl::FileUtils::get()->isDirectory(binDir)) {
            continue;
        }
        std::vector<std::string> filesInDir = sgl::FileUtils::get()->getFilesInDirectoryVector(binDir);
        for (const std::string& fileInDir : filesInDir) {
            std::string fileName = sgl::FileUtils::get()->getPureFilename(fileInDir);
            if (sgl::startsWith(fileName, "hiprtc") && sgl::endsWith(fileName, ".dll")) {
                hiprtcDllFileName = binDir + "/" + fileName;
            }
        }
    }
    if (hiprtcDllFileName.empty()) {
        sgl::Logfile::get()->writeInfo("initializeHiprtcFunctionTable: Could not find hiprtc.dll.");
        return false;
    }

    g_hiprtcLibraryHandle = LoadLibraryA(hiprtcDllFileName.c_str());
    if (!g_hiprtcLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeHiprtcFunctionTable: Could not load " + hiprtcDllFileName + ".");
        return false;
    }
#endif

    g_hiprtcFunctionTable.hiprtcGetErrorString = PFN_hiprtcGetErrorString(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcGetErrorString)));
    g_hiprtcFunctionTable.hiprtcCreateProgram = PFN_hiprtcCreateProgram(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcCreateProgram)));
    g_hiprtcFunctionTable.hiprtcDestroyProgram = PFN_hiprtcDestroyProgram(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcDestroyProgram)));
    g_hiprtcFunctionTable.hiprtcCompileProgram = PFN_hiprtcCompileProgram(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcCompileProgram)));
    g_hiprtcFunctionTable.hiprtcGetProgramLogSize = PFN_hiprtcGetProgramLogSize(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcGetProgramLogSize)));
    g_hiprtcFunctionTable.hiprtcGetProgramLog = PFN_hiprtcGetProgramLog(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcGetProgramLog)));
    g_hiprtcFunctionTable.hiprtcGetCodeSize = PFN_hiprtcGetCodeSize(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcGetCodeSize)));
    g_hiprtcFunctionTable.hiprtcGetCode = PFN_hiprtcGetCode(dlsym(g_hiprtcLibraryHandle, TOSTRING(hiprtcGetCode)));

    if (!g_hiprtcFunctionTable.hiprtcGetErrorString
        || !g_hiprtcFunctionTable.hiprtcCreateProgram
        || !g_hiprtcFunctionTable.hiprtcDestroyProgram
        || !g_hiprtcFunctionTable.hiprtcCompileProgram
        || !g_hiprtcFunctionTable.hiprtcGetProgramLogSize
        || !g_hiprtcFunctionTable.hiprtcGetProgramLog
        || !g_hiprtcFunctionTable.hiprtcGetCodeSize
        || !g_hiprtcFunctionTable.hiprtcGetCode) {
        sgl::Logfile::get()->throwError(
                "Error in initializeHiprtcFunctionTable: "
                "At least one function pointer could not be loaded.");
    }

    return true;
}

#ifdef _WIN32
#undef dlsym
#endif

bool getIsHipDeviceApiFunctionTableInitialized() {
    return g_hipLibraryHandle != nullptr;
}

void freeHipDeviceApiFunctionTable() {
    if (g_hipLibraryHandle) {
#if defined(__linux__)
        dlclose(g_hipLibraryHandle);
#elif defined(_WIN32)
        FreeLibrary(g_hipLibraryHandle);
#endif
        g_hipLibraryHandle = {};
    }
}

bool getIsHiprtcFunctionTableInitialized() {
    return g_hiprtcLibraryHandle != nullptr;
}

void freeHiprtcFunctionTable() {
    if (g_hiprtcLibraryHandle) {
#if defined(__linux__)
        dlclose(g_hiprtcLibraryHandle);
#elif defined(_WIN32)
        FreeLibrary(g_hiprtcLibraryHandle);
#endif
        g_hiprtcLibraryHandle = {};
    }
}

void _checkHipResult(hipError_t hipResult, const char* text, const char* locationText) {
    if (hipResult != hipSuccess) {
        const char* errorString = nullptr;
        hipResult = g_hipDeviceApiFunctionTable.hipDrvGetErrorString(hipResult, &errorString);
        if (hipResult == hipSuccess) {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + errorString);
        } else {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + "Error in hipDrvGetErrorString.");
        }
    }
}

void _checkHiprtcResult(hiprtcResult result, const char* text, const char* locationText) {
    if (result != HIPRTC_SUCCESS) {
        throw std::runtime_error(
                std::string() + locationText + ": " + text + g_hiprtcFunctionTable.hiprtcGetErrorString(result));
    }
}

}
