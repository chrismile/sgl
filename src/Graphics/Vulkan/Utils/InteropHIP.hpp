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

#ifndef SGL_INTEROPHIP_HPP
#define SGL_INTEROPHIP_HPP

#if __has_include(<hip/hip_runtime.h>)
#ifndef __HIP_PLATFORM_AMD__
#define __HIP_PLATFORM_AMD__
//#define __HIP_PLATFORM_HCC__
#endif
#include <hip/hip_runtime.h>
#include <hip/hip_runtime_api.h>
#include <hip/hiprtc.h>
#else
#error "HIP headers could not be found."
#endif

namespace sgl { namespace vk {

class Device;

/*
 * Utility functions for Vulkan-HIP driver API interoperability.
 */
struct HipDeviceApiFunctionTable {
    hipError_t ( *hipInit )( unsigned int Flags );
    hipError_t ( *hipDrvGetErrorString )( hipError_t error, const char** pStr );

    hipError_t ( *hipDeviceGet )( hipDevice_t* device, int ordinal );
    hipError_t ( *hipGetDeviceCount )( int* count );
    hipError_t ( *hipDeviceGetUuid )( hipUUID* uuid, hipDevice_t dev );
    hipError_t ( *hipDeviceGetAttribute )( int* pi, hipDeviceAttribute_t attrib, hipDevice_t dev );

    hipError_t ( *hipCtxCreate )( hipCtx_t* pctx, unsigned int flags, hipDevice_t dev );
    hipError_t ( *hipCtxDestroy )( hipCtx_t ctx );
    hipError_t ( *hipCtxGetCurrent )( hipCtx_t* pctx );
    hipError_t ( *hipCtxGetDevice )( hipDevice_t* device );
    hipError_t ( *hipCtxSetCurrent )( hipCtx_t ctx );

    hipError_t ( *hipStreamCreate )( hipStream_t* phStream, unsigned int Flags );
    hipError_t ( *hipStreamDestroy )( hipStream_t hStream );
    hipError_t ( *hipStreamSynchronize )( hipStream_t hStream );

    hipError_t ( *hipMalloc )( void** dptr, size_t bytesize );
    hipError_t ( *hipFree )( void* dptr );
    hipError_t ( *hipMemcpyDtoH )( void* dstHost, hipDeviceptr_t srcDevice, size_t ByteCount );
    hipError_t ( *hipMemcpyHtoD )( hipDeviceptr_t dstDevice, const void* srcHost, size_t ByteCount );
    hipError_t ( *hipMallocAsync )( void** dptr, size_t bytesize, hipStream_t hStream );
    hipError_t ( *hipFreeAsync )( void* dptr, hipStream_t hStream );
    hipError_t ( *hipMemsetD8Async )( hipDeviceptr_t dstDevice, unsigned char uc, size_t N, hipStream_t hStream );
    hipError_t ( *hipMemsetD16Async )( hipDeviceptr_t dstDevice, unsigned short us, size_t N, hipStream_t hStream );
    hipError_t ( *hipMemsetD32Async )( hipDeviceptr_t dstDevice, unsigned int ui, size_t N, hipStream_t hStream );
    hipError_t ( *hipMemcpyAsync )( hipDeviceptr_t dst, hipDeviceptr_t src, size_t ByteCount, hipStream_t hStream );
    hipError_t ( *hipMemcpyDtoHAsync )( void* dstHost, hipDeviceptr_t srcDevice, size_t ByteCount, hipStream_t hStream );
    hipError_t ( *hipMemcpyHtoDAsync )( hipDeviceptr_t dstDevice, const void* srcHost, size_t ByteCount, hipStream_t hStream );
    //hipError_t ( *hipDrvMemcpy2DAsync )( const HIP_MEMCPY2D* pCopy, hipStream_t hStream );
    hipError_t ( *hipDrvMemcpy3DAsync )( const HIP_MEMCPY3D* pCopy, hipStream_t hStream );

    hipError_t ( *hipArrayCreate )( hipArray* pHandle, const HIP_ARRAY_DESCRIPTOR* pAllocateArray );
    hipError_t ( *hipArray3DCreate )( hipArray* pHandle, const HIP_ARRAY3D_DESCRIPTOR* pAllocateArray );
    hipError_t ( *hipArrayDestroy )( hipArray hArray );
    hipError_t ( *hipMipmappedArrayCreate )( hipMipmappedArray_t* pHandle, const HIP_ARRAY3D_DESCRIPTOR* pMipmappedArrayDesc, unsigned int numMipmapLevels );
    hipError_t ( *hipMipmappedArrayDestroy )( hipMipmappedArray_t hMipmappedArray );
    hipError_t ( *hipMipmappedArrayGetLevel )( hipArray_t* pLevelArray, hipMipmappedArray_t hMipmappedArray, unsigned int level );

    hipError_t ( *hipTexObjectCreate )( hipTextureObject_t* pTexObject, const HIP_RESOURCE_DESC* pResDesc, const HIP_TEXTURE_DESC* pTexDesc, const HIP_RESOURCE_VIEW_DESC* pResViewDesc );
    hipError_t ( *hipTexObjectDestroy )( hipTextureObject_t texObject );
    hipError_t ( *hipCreateTextureObject )( hipTextureObject_t* pTexObject, const hipResourceDesc* pResDesc, const hipTextureDesc* pTexDesc, const hipResourceViewDesc* pResViewDesc );
    hipError_t ( *hipDestroyTextureObject )( hipTextureObject_t textureObject );
    hipError_t ( *hipCreateSurfaceObject )( hipSurfaceObject_t* pSurfObject, const hipResourceDesc* pResDesc );
    hipError_t ( *hipDestroySurfaceObject )( hipSurfaceObject_t surfaceObject );

    hipError_t ( *hipImportExternalMemory )( hipExternalMemory_t* extMem_out, const hipExternalMemoryHandleDesc* memHandleDesc );
    hipError_t ( *hipExternalMemoryGetMappedBuffer )( hipDeviceptr_t* devPtr, hipExternalMemory_t extMem, const hipExternalMemoryBufferDesc* bufferDesc );
    hipError_t ( *hipExternalMemoryGetMappedMipmappedArray )( hipMipmappedArray_t* mipmap, hipExternalMemory_t extMem, const hipExternalMemoryMipmappedArrayDesc* mipmapDesc );
    hipError_t ( *hipDestroyExternalMemory )( hipExternalMemory_t extMem );

    hipError_t ( *hipImportExternalSemaphore )( hipExternalSemaphore_t* extSem_out, const hipExternalSemaphoreHandleDesc* semHandleDesc );
    hipError_t ( *hipSignalExternalSemaphoresAsync )( const hipExternalSemaphore_t* extSemArray, const hipExternalSemaphoreSignalParams* paramsArray, unsigned int numExtSems, hipStream_t stream );
    hipError_t ( *hipWaitExternalSemaphoresAsync )( const hipExternalSemaphore_t* extSemArray, const hipExternalSemaphoreWaitParams* paramsArray, unsigned int numExtSems, hipStream_t stream );
    hipError_t ( *hipDestroyExternalSemaphore )( hipExternalSemaphore_t extSem );

    hipError_t ( *hipModuleLoad )( hipModule_t* module, const char* fname );
    hipError_t ( *hipModuleLoadData )( hipModule_t* module, const void* image );
    hipError_t ( *hipModuleLoadDataEx )( hipModule_t* module, const void* image, unsigned int numOptions, hipJitOption* options, void** optionValues );
    hipError_t ( *hipModuleUnload )( hipModule_t hmod );
    hipError_t ( *hipModuleGetFunction )( hipFunction_t* hfunc, hipModule_t hmod, const char* name );
    hipError_t ( *hipModuleGetGlobal )( hipDeviceptr_t* dptr, size_t* bytes, hipModule_t hmod, const char* name );
    hipError_t ( *hipLaunchKernel )( hipFunction_t f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ, unsigned int sharedMemBytes, hipStream_t hStream, void** kernelParams, void** extra );
    hipError_t ( *hipOccupancyMaxPotentialBlockSize )( int* minGridSize, int* blockSize, hipFunction_t func, const void* blockSizeToDynamicSMemSize, size_t dynamicSMemSize, int blockSizeLimit );
};

DLL_OBJECT extern HipDeviceApiFunctionTable g_hipDeviceApiFunctionTable;

struct HiprtcFunctionTable {
    const char* ( *hiprtcGetErrorString )( hiprtcResult result );
    hiprtcResult ( *hiprtcCreateProgram )( hiprtcProgram* prog, const char* src, const char* name, int numHeaders, const char* const* headers, const char* const* includeNames );
    hiprtcResult ( *hiprtcDestroyProgram )( hiprtcProgram* prog );
    hiprtcResult ( *hiprtcCompileProgram )( hiprtcProgram prog, int numOptions, const char* const* options );
    hiprtcResult ( *hiprtcGetProgramLogSize )( hiprtcProgram prog, size_t* logSizeRet );
    hiprtcResult ( *hiprtcGetProgramLog )( hiprtcProgram prog, char* log );
    hiprtcResult ( *hiprtcGetCodeSize )( hiprtcProgram prog, size_t* codeSizeRet );
    hiprtcResult ( *hiprtcGetCode )( hiprtcProgram prog, char* code );
};

DLL_OBJECT extern HiprtcFunctionTable g_hiprtcFunctionTable;

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

DLL_OBJECT void _checkHipResult(hipError_t hipResult, const char* text, const char* locationText);
#define checkHipResult(hipResult, text) _checkHipResult(hipResult, text, __FILE__ ":" TOSTRING(__LINE__))

DLL_OBJECT bool initializeHipDeviceApiFunctionTable();
DLL_OBJECT bool getIsHipDeviceApiFunctionTableInitialized();
DLL_OBJECT void freeHipDeviceApiFunctionTable();

DLL_OBJECT void _checkHiprtcResult(hiprtcResult result, const char* text, const char* locationText);
#define checkHiprtcResult(result, text) _checkHiprtcResult(result, text, __FILE__ ":" TOSTRING(__LINE__))

DLL_OBJECT bool initializeHiprtcFunctionTable();
DLL_OBJECT bool getIsHiprtcFunctionTableInitialized();
DLL_OBJECT void freeHiprtcFunctionTable();

/**
 * Returns the closest matching HIP device.
 * @param device The Vulkan device.
 * @param hipDevice The HIP device (if true is returned).
 * @return Whether a matching HIP device was found.
 */
DLL_OBJECT bool getMatchingHipDevice(sgl::vk::Device* device, hipDevice_t* hipDevice);

}}

#endif //SGL_INTEROPHIP_HPP
