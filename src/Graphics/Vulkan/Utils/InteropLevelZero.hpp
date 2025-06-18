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

#ifndef SGL_INTEROPLEVELZERO_HPP
#define SGL_INTEROPLEVELZERO_HPP

/*
 * Interoperability with Intel's oneAPI Level Zero API.
 *
 * For an API documentation, please refer to:
 * https://oneapi-src.github.io/level-zero-spec/level-zero/latest/core/api.html
 *
 * We ship the Level Zero headers with sgl, as they are released under the MIT license.
 * For Windows, it seems like the loader DLL is automatically installed with the drivers at C:/Windows/System32/ze_loader.dll.
 * On Linux, the necessary components can be obtained from here:
 * - Level Zero driver (intel-level-zero-gpu): https://github.com/intel/compute-runtime/releases
 * - Level Zero loader (level-zero, level-zero-devel): https://github.com/oneapi-src/level-zero/releases
 * For more details see: https://github.com/intel/compute-runtime/blob/master/level_zero/doc/BUILD.md
 *
 * How to check what backends SYCL supports?
 * "C:\Program Files (x86)\Intel\oneAPI\compiler\2025.1\env\vars.bat"
 * "C:\Program Files (x86)\Intel\oneAPI\ocloc\2025.1\env\vars.bat"
 * sycl-ls
 *
 * On Linux:
 * source /opt/intel/oneapi/compiler/latest/env/vars.sh
 * source /opt/intel/oneapi/umf/latest/env/vars.sh
 * source /opt/intel/oneapi/pti/latest/env/vars.sh
 *
 * How to build Level Zero loader library manually?
 * git clone https://github.com/oneapi-src/level-zero.git level-zero-src
 * cd level-zero-src
 * cmake -S . -B build
 * cmake --build build --config Release
 * cmake --install build --config Release --prefix <path>
 */

#include "../libs/level-zero/ze_api.h"

#ifdef SUPPORT_SYCL_INTEROP
namespace sycl { inline namespace _V1 {
class queue;
}}
#endif

namespace sgl { namespace vk {

class Device;

struct LevelZeroFunctionTable {
    ze_result_t ( *zeInit )( ze_init_flags_t flags );
    ze_result_t ( *zeDriverGet )( uint32_t* pCount, ze_driver_handle_t* phDrivers );
    ze_result_t ( *zeInitDrivers )( uint32_t* pCount, ze_driver_handle_t* phDrivers, ze_init_driver_type_desc_t* desc );
    ze_result_t ( *zeDriverGetApiVersion )( ze_driver_handle_t hDriver, ze_api_version_t* version );
    ze_result_t ( *zeDriverGetProperties )( ze_driver_handle_t hDriver, ze_driver_properties_t* pDriverProperties );
    ze_result_t ( *zeDriverGetIpcProperties )( ze_driver_handle_t hDriver, ze_driver_ipc_properties_t* pIpcProperties );
    ze_result_t ( *zeDriverGetExtensionProperties )( ze_driver_handle_t hDriver, uint32_t* pCount, ze_driver_extension_properties_t* pExtensionProperties );
    ze_result_t ( *zeDriverGetExtensionFunctionAddress )( ze_driver_handle_t hDriver, const char* name, void** ppFunctionAddress );
    ze_result_t ( *zeDriverGetLastErrorDescription )( ze_driver_handle_t hDriver, const char** ppString );

    ze_result_t ( *zeDeviceGet )( ze_driver_handle_t hDriver, uint32_t* pCount, ze_device_handle_t* phDevices );
    ze_result_t ( *zeDeviceGetRootDevice )( ze_device_handle_t hDevice, ze_device_handle_t* phRootDevice );
    ze_result_t ( *zeDeviceGetSubDevices )( ze_device_handle_t hDevice, uint32_t* pCount, ze_device_handle_t* phSubdevices );
    ze_result_t ( *zeDeviceGetProperties )( ze_device_handle_t hDevice, ze_device_properties_t* pDeviceProperties );
    ze_result_t ( *zeDeviceGetComputeProperties )( ze_device_handle_t hDevice, ze_device_compute_properties_t* pComputeProperties );
    ze_result_t ( *zeDeviceGetModuleProperties )( ze_device_handle_t hDevice, ze_device_module_properties_t* pModuleProperties );
    ze_result_t ( *zeDeviceGetCommandQueueGroupProperties )( ze_device_handle_t hDevice, uint32_t* pCount, ze_command_queue_group_properties_t* pCommandQueueGroupProperties );
    ze_result_t ( *zeDeviceGetMemoryProperties )( ze_device_handle_t hDevice, uint32_t* pCount, ze_device_memory_properties_t* pMemProperties );
    ze_result_t ( *zeDeviceGetMemoryAccessProperties )( ze_device_handle_t hDevice, ze_device_memory_access_properties_t* pMemAccessProperties );
    ze_result_t ( *zeDeviceGetCacheProperties )( ze_device_handle_t hDevice, uint32_t* pCount, ze_device_cache_properties_t* pCacheProperties );
    ze_result_t ( *zeDeviceGetImageProperties )( ze_device_handle_t hDevice, ze_device_image_properties_t* pImageProperties );
    ze_result_t ( *zeDeviceGetExternalMemoryProperties )( ze_device_handle_t hDevice, ze_device_external_memory_properties_t* pExternalMemoryProperties );
    ze_result_t ( *zeDeviceGetP2PProperties )( ze_device_handle_t hDevice, ze_device_handle_t hPeerDevice, ze_device_p2p_properties_t* pP2PProperties );
    ze_result_t ( *zeDeviceCanAccessPeer )( ze_device_handle_t hDevice, ze_device_handle_t hPeerDevice, ze_bool_t* value );
    ze_result_t ( *zeDeviceGetStatus )( ze_device_handle_t hDevice );
    ze_result_t ( *zeDeviceGetGlobalTimestamps )( ze_device_handle_t hDevice, uint64_t* hostTimestamp, uint64_t* deviceTimestamp );

    ze_result_t ( *zeContextCreate )( ze_driver_handle_t hDriver, const ze_context_desc_t* desc, ze_context_handle_t* phContext );
    ze_result_t ( *zeContextCreateEx )( ze_driver_handle_t hDriver, const ze_context_desc_t* desc, uint32_t numDevices, ze_device_handle_t* phDevices, ze_context_handle_t* phContext );
    ze_result_t ( *zeContextDestroy )( ze_context_handle_t hContext );
    ze_result_t ( *zeContextGetStatus )( ze_context_handle_t hContext );

    ze_result_t ( *zeCommandQueueCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* desc, ze_command_queue_handle_t* phCommandQueue );
    ze_result_t ( *zeCommandQueueDestroy )( ze_command_queue_handle_t hCommandQueue );
    ze_result_t ( *zeCommandQueueExecuteCommandLists )( ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists, ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence );
    ze_result_t ( *zeCommandQueueSynchronize )( ze_command_queue_handle_t hCommandQueue, uint64_t timeout );
    ze_result_t ( *zeCommandQueueGetOrdinal )( ze_command_queue_handle_t hCommandQueue, uint32_t* pOrdinal );
    ze_result_t ( *zeCommandQueueGetIndex )( ze_command_queue_handle_t hCommandQueue, uint32_t* pIndex );
    ze_result_t ( *zeCommandListCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_list_desc_t* desc, ze_command_list_handle_t* phCommandList );
    ze_result_t ( *zeCommandListCreateImmediate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* altdesc, ze_command_list_handle_t* phCommandList );
    ze_result_t ( *zeCommandListDestroy )( ze_command_list_handle_t hCommandList );
    ze_result_t ( *zeCommandListClose )( ze_command_list_handle_t hCommandList );
    ze_result_t ( *zeCommandListReset )( ze_command_list_handle_t hCommandList );
    ze_result_t ( *zeCommandListAppendWriteGlobalTimestamp )( ze_command_list_handle_t hCommandList, uint64_t* dstptr, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListHostSynchronize )( ze_command_list_handle_t hCommandList, uint64_t timeout );
    ze_result_t ( *zeCommandListGetDeviceHandle )( ze_command_list_handle_t hCommandList, ze_device_handle_t* phDevice );
    ze_result_t ( *zeCommandListGetContextHandle )( ze_command_list_handle_t hCommandList, ze_context_handle_t* phContext );
    ze_result_t ( *zeCommandListGetOrdinal )( ze_command_list_handle_t hCommandList, uint32_t* pOrdinal );
    ze_result_t ( *zeCommandListImmediateGetIndex )( ze_command_list_handle_t hCommandListImmediate, uint32_t* pIndex );
    ze_result_t ( *zeCommandListIsImmediate )( ze_command_list_handle_t hCommandList, ze_bool_t* pIsImmediate );
    ze_result_t ( *zeCommandListAppendBarrier )( ze_command_list_handle_t hCommandList, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendMemoryRangesBarrier )( ze_command_list_handle_t hCommandList, uint32_t numRanges, const size_t* pRangeSizes, const void** pRanges, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeContextSystemBarrier )( ze_context_handle_t hContext, ze_device_handle_t hDevice );
    ze_result_t ( *zeCommandListAppendMemoryCopy )( ze_command_list_handle_t hCommandList, void* dstptr, const void* srcptr, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendMemoryFill )( ze_command_list_handle_t hCommandList, void* ptr, const void* pattern, size_t pattern_size, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendMemoryCopyRegion )( ze_command_list_handle_t hCommandList, void* dstptr, const ze_copy_region_t* dstRegion, uint32_t dstPitch, uint32_t dstSlicePitch, const void* srcptr, const ze_copy_region_t* srcRegion, uint32_t srcPitch, uint32_t srcSlicePitch, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendMemoryCopyFromContext )( ze_command_list_handle_t hCommandList, void* dstptr, ze_context_handle_t hContextSrc, const void* srcptr, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendImageCopy )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, ze_image_handle_t hSrcImage, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendImageCopyRegion )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, ze_image_handle_t hSrcImage, const ze_image_region_t* pDstRegion, const ze_image_region_t* pSrcRegion, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendImageCopyToMemory )( ze_command_list_handle_t hCommandList, void* dstptr, ze_image_handle_t hSrcImage, const ze_image_region_t* pSrcRegion, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendImageCopyFromMemory )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, const void* srcptr, const ze_image_region_t* pDstRegion, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendMemoryPrefetch )( ze_command_list_handle_t hCommandList, const void* ptr, size_t size );
    ze_result_t ( *zeCommandListAppendMemAdvise )( ze_command_list_handle_t hCommandList, ze_device_handle_t hDevice, const void* ptr, size_t size, ze_memory_advice_t advice );

    ze_result_t ( *zeEventPoolCreate )( ze_context_handle_t hContext, const ze_event_pool_desc_t* desc, uint32_t numDevices, ze_device_handle_t* phDevices, ze_event_pool_handle_t* phEventPool );
    ze_result_t ( *zeEventPoolDestroy )( ze_event_pool_handle_t hEventPool );
    ze_result_t ( *zeEventCreate )( ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc, ze_event_handle_t* phEvent );
    ze_result_t ( *zeEventDestroy )( ze_event_handle_t hEvent );
    ze_result_t ( *zeEventPoolGetIpcHandle )( ze_event_pool_handle_t hEventPool, ze_ipc_event_pool_handle_t* phIpc );
    ze_result_t ( *zeEventPoolPutIpcHandle )( ze_context_handle_t hContext, ze_ipc_event_pool_handle_t hIpc );
    ze_result_t ( *zeEventPoolOpenIpcHandle )( ze_context_handle_t hContext, ze_ipc_event_pool_handle_t hIpc, ze_event_pool_handle_t* phEventPool );
    ze_result_t ( *zeEventPoolCloseIpcHandle )( ze_event_pool_handle_t hEventPool );
    ze_result_t ( *zeCommandListAppendSignalEvent )( ze_command_list_handle_t hCommandList, ze_event_handle_t hEvent );
    ze_result_t ( *zeCommandListAppendWaitOnEvents )( ze_command_list_handle_t hCommandList, uint32_t numEvents, ze_event_handle_t* phEvents );
    ze_result_t ( *zeEventHostSignal )( ze_event_handle_t hEvent );
    ze_result_t ( *zeEventHostSynchronize )( ze_event_handle_t hEvent, uint64_t timeout );
    ze_result_t ( *zeEventQueryStatus )( ze_event_handle_t hEvent );
    ze_result_t ( *zeCommandListAppendEventReset )( ze_command_list_handle_t hCommandList, ze_event_handle_t hEvent );
    ze_result_t ( *zeEventHostReset )( ze_event_handle_t hEvent );
    ze_result_t ( *zeEventQueryKernelTimestamp )( ze_event_handle_t hEvent, ze_kernel_timestamp_result_t* dstptr );
    ze_result_t ( *zeCommandListAppendQueryKernelTimestamps )( ze_command_list_handle_t hCommandList, uint32_t numEvents, ze_event_handle_t* phEvents, void* dstptr, const size_t* pOffsets, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeEventGetEventPool )( ze_event_handle_t hEvent, ze_event_pool_handle_t* phEventPool );
    ze_result_t ( *zeEventGetSignalScope )( ze_event_handle_t hEvent, ze_event_scope_flags_t* pSignalScope );
    ze_result_t ( *zeEventGetWaitScope )( ze_event_handle_t hEvent, ze_event_scope_flags_t* pWaitScope );
    ze_result_t ( *zeEventPoolGetContextHandle )( ze_event_pool_handle_t hEventPool, ze_context_handle_t* phContext );
    ze_result_t ( *zeEventPoolGetFlags )( ze_event_pool_handle_t hEventPool, ze_event_pool_flags_t* pFlags );

    ze_result_t ( *zeFenceCreate )( ze_command_queue_handle_t hCommandQueue, const ze_fence_desc_t* desc, ze_fence_handle_t* phFence );
    ze_result_t ( *zeFenceDestroy )( ze_fence_handle_t hFence );
    ze_result_t ( *zeFenceHostSynchronize )( ze_fence_handle_t hFence, uint64_t timeout );
    ze_result_t ( *zeFenceQueryStatus )( ze_fence_handle_t hFence );
    ze_result_t ( *zeFenceReset )( ze_fence_handle_t hFence );

    ze_result_t ( *zeImageGetProperties )( ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_properties_t* pImageProperties );
    ze_result_t ( *zeImageCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_handle_t* phImage );
    ze_result_t ( *zeImageDestroy )( ze_image_handle_t hImage );

    ze_result_t ( *zeMemAllocShared )( ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, const ze_host_mem_alloc_desc_t* host_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr );
    ze_result_t ( *zeMemAllocDevice )( ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr );
    ze_result_t ( *zeMemAllocHost )( ze_context_handle_t hContext, const ze_host_mem_alloc_desc_t* host_desc, size_t size, size_t alignment, void** pptr );
    ze_result_t ( *zeMemFree )( ze_context_handle_t hContext, void* ptr );
    ze_result_t ( *zeMemGetAllocProperties )( ze_context_handle_t hContext, const void* ptr, ze_memory_allocation_properties_t* pMemAllocProperties, ze_device_handle_t* phDevice );
    ze_result_t ( *zeMemGetAddressRange )( ze_context_handle_t hContext, const void* ptr, void** pBase, size_t* pSize );
    ze_result_t ( *zeMemGetIpcHandle )( ze_context_handle_t hContext, const void* ptr, ze_ipc_mem_handle_t* pIpcHandle );
    ze_result_t ( *zeMemGetIpcHandleFromFileDescriptorExp )( ze_context_handle_t hContext, uint64_t handle, ze_ipc_mem_handle_t* pIpcHandle );
    ze_result_t ( *zeMemGetFileDescriptorFromIpcHandleExp )( ze_context_handle_t hContext, ze_ipc_mem_handle_t ipcHandle, uint64_t* pHandle );
    ze_result_t ( *zeMemPutIpcHandle )( ze_context_handle_t hContext, ze_ipc_mem_handle_t handle );
    ze_result_t ( *zeMemOpenIpcHandle )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_ipc_mem_handle_t handle, ze_ipc_memory_flags_t flags, void** pptr );
    ze_result_t ( *zeMemCloseIpcHandle )( ze_context_handle_t hContext, const void* ptr );
    ze_result_t ( *zeMemSetAtomicAccessAttributeExp )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const void* ptr, size_t size, ze_memory_atomic_attr_exp_flags_t attr );
    ze_result_t ( *zeMemGetAtomicAccessAttributeExp )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const void* ptr, size_t size, ze_memory_atomic_attr_exp_flags_t* pAttr );

    ze_result_t ( *zeModuleCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_module_desc_t* desc, ze_module_handle_t* phModule, ze_module_build_log_handle_t* phBuildLog );
    ze_result_t ( *zeModuleDestroy )( ze_module_handle_t hModule );
    ze_result_t ( *zeModuleDynamicLink )( uint32_t numModules, ze_module_handle_t* phModules, ze_module_build_log_handle_t* phLinkLog );
    ze_result_t ( *zeModuleBuildLogDestroy )( ze_module_build_log_handle_t hModuleBuildLog );
    ze_result_t ( *zeModuleBuildLogGetString )( ze_module_build_log_handle_t hModuleBuildLog, size_t* pSize, char* pBuildLog );
    ze_result_t ( *zeModuleGetNativeBinary )( ze_module_handle_t hModule, size_t* pSize, uint8_t* pModuleNativeBinary );
    ze_result_t ( *zeModuleGetGlobalPointer )( ze_module_handle_t hModule, const char* pGlobalName, size_t* pSize, void** pptr );
    ze_result_t ( *zeModuleGetKernelNames )( ze_module_handle_t hModule, uint32_t* pCount, const char** pNames );
    ze_result_t ( *zeModuleGetProperties )( ze_module_handle_t hModule, ze_module_properties_t* pModuleProperties );
    ze_result_t ( *zeKernelCreate )( ze_module_handle_t hModule, const ze_kernel_desc_t* desc, ze_kernel_handle_t* phKernel );
    ze_result_t ( *zeKernelDestroy )( ze_kernel_handle_t hKernel );
    ze_result_t ( *zeModuleGetFunctionPointer )( ze_module_handle_t hModule, const char* pFunctionName, void** pfnFunction );
    ze_result_t ( *zeKernelSetGroupSize )( ze_kernel_handle_t hKernel, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ );
    ze_result_t ( *zeKernelSuggestGroupSize )( ze_kernel_handle_t hKernel, uint32_t globalSizeX, uint32_t globalSizeY, uint32_t globalSizeZ, uint32_t* groupSizeX, uint32_t* groupSizeY, uint32_t* groupSizeZ );
    ze_result_t ( *zeKernelSuggestMaxCooperativeGroupCount )( ze_kernel_handle_t hKernel, uint32_t* totalGroupCount );
    ze_result_t ( *zeKernelSetArgumentValue )( ze_kernel_handle_t hKernel, uint32_t argIndex, size_t argSize, const void* pArgValue );
    ze_result_t ( *zeKernelSetIndirectAccess )( ze_kernel_handle_t hKernel, ze_kernel_indirect_access_flags_t flags );
    ze_result_t ( *zeKernelGetIndirectAccess )( ze_kernel_handle_t hKernel, ze_kernel_indirect_access_flags_t* pFlags );
    ze_result_t ( *zeKernelGetSourceAttributes )( ze_kernel_handle_t hKernel, uint32_t* pSize, char** pString );
    ze_result_t ( *zeKernelSetCacheConfig )( ze_kernel_handle_t hKernel, ze_cache_config_flags_t flags );
    ze_result_t ( *zeKernelGetProperties )( ze_kernel_handle_t hKernel, ze_kernel_properties_t* pKernelProperties );
    ze_result_t ( *zeKernelGetName )( ze_kernel_handle_t hKernel, size_t* pSize, char* pName );

    ze_result_t ( *zeCommandListAppendLaunchKernel )( ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendLaunchCooperativeKernel )( ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendLaunchKernelIndirect )( ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchArgumentsBuffer, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendLaunchMultipleKernelsIndirect )( ze_command_list_handle_t hCommandList, uint32_t numKernels, ze_kernel_handle_t* phKernels, const uint32_t* pCountBuffer, const ze_group_count_t* pLaunchArgumentsBuffer, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeContextMakeMemoryResident )( ze_context_handle_t hContext, ze_device_handle_t hDevice, void* ptr, size_t size );
    ze_result_t ( *zeContextEvictMemory )( ze_context_handle_t hContext, ze_device_handle_t hDevice, void* ptr, size_t size );
    ze_result_t ( *zeContextMakeImageResident )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_image_handle_t hImage );
    ze_result_t ( *zeContextEvictImage )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_image_handle_t hImage );

    ze_result_t ( *zeSamplerCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_sampler_desc_t* desc, ze_sampler_handle_t* phSampler );
    ze_result_t ( *zeSamplerDestroy )( ze_sampler_handle_t hSampler );
    ze_result_t ( *zeVirtualMemReserve )( ze_context_handle_t hContext, const void* pStart, size_t size, void** pptr );
    ze_result_t ( *zeVirtualMemFree )( ze_context_handle_t hContext, const void* ptr, size_t size );
    ze_result_t ( *zeVirtualMemQueryPageSize )( ze_context_handle_t hContext, ze_device_handle_t hDevice, size_t size, size_t* pagesize );
    ze_result_t ( *zePhysicalMemCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_physical_mem_desc_t* desc, ze_physical_mem_handle_t* phPhysicalMemory );
    ze_result_t ( *zePhysicalMemDestroy )( ze_context_handle_t hContext, ze_physical_mem_handle_t hPhysicalMemory );
    ze_result_t ( *zeVirtualMemMap )( ze_context_handle_t hContext, const void* ptr, size_t size, ze_physical_mem_handle_t hPhysicalMemory, size_t offset, ze_memory_access_attribute_t access );
    ze_result_t ( *zeVirtualMemUnmap )( ze_context_handle_t hContext, const void* ptr, size_t size );
    ze_result_t ( *zeVirtualMemSetAccessAttribute )( ze_context_handle_t hContext, const void* ptr, size_t size, ze_memory_access_attribute_t access );
    ze_result_t ( *zeVirtualMemGetAccessAttribute )( ze_context_handle_t hContext, const void* ptr, size_t size, ze_memory_access_attribute_t* access, size_t* outSize );
    ze_result_t ( *zeKernelSetGlobalOffsetExp )( ze_kernel_handle_t hKernel, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ );
    ze_result_t ( *zeKernelGetBinaryExp )( ze_kernel_handle_t hKernel, size_t* pSize, uint8_t* pKernelBinary );

    ze_result_t ( *zeDeviceImportExternalSemaphoreExt )( ze_device_handle_t hDevice, const ze_external_semaphore_ext_desc_t* desc, ze_external_semaphore_ext_handle_t* phSemaphore );
    ze_result_t ( *zeDeviceReleaseExternalSemaphoreExt )( ze_external_semaphore_ext_handle_t hSemaphore );
    ze_result_t ( *zeCommandListAppendSignalExternalSemaphoreExt )( ze_command_list_handle_t hCommandList, uint32_t numSemaphores, ze_external_semaphore_ext_handle_t* phSemaphores, ze_external_semaphore_signal_params_ext_t* signalParams, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendWaitExternalSemaphoreExt )( ze_command_list_handle_t hCommandList, uint32_t numSemaphores, ze_external_semaphore_ext_handle_t* phSemaphores, ze_external_semaphore_wait_params_ext_t* waitParams, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );

    ze_result_t ( *zeDeviceReserveCacheExt )( ze_device_handle_t hDevice, size_t cacheLevel, size_t cacheReservationSize );
    ze_result_t ( *zeDeviceSetCacheAdviceExt )( ze_device_handle_t hDevice, void* ptr, size_t regionSize, ze_cache_ext_region_t cacheRegion );
    ze_result_t ( *zeEventQueryTimestampsExp )( ze_event_handle_t hEvent, ze_device_handle_t hDevice, uint32_t* pCount, ze_kernel_timestamp_result_t* pTimestamps );
    ze_result_t ( *zeImageGetMemoryPropertiesExp )( ze_image_handle_t hImage, ze_image_memory_properties_exp_t* pMemoryProperties );
    ze_result_t ( *zeImageViewCreateExt )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_handle_t hImage, ze_image_handle_t* phImageView );
    ze_result_t ( *zeImageViewCreateExp )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_handle_t hImage, ze_image_handle_t* phImageView );
    ze_result_t ( *zeKernelSchedulingHintExp )( ze_kernel_handle_t hKernel, ze_scheduling_hint_exp_desc_t* pHint );
    ze_result_t ( *zeDevicePciGetPropertiesExt )( ze_device_handle_t hDevice, ze_pci_ext_properties_t* pPciProperties );
    ze_result_t ( *zeCommandListAppendImageCopyToMemoryExt )( ze_command_list_handle_t hCommandList, void* dstptr, ze_image_handle_t hSrcImage, const ze_image_region_t* pSrcRegion, uint32_t destRowPitch, uint32_t destSlicePitch, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListAppendImageCopyFromMemoryExt )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, const void* srcptr, const ze_image_region_t* pDstRegion, uint32_t srcRowPitch, uint32_t srcSlicePitch, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeImageGetAllocPropertiesExt )( ze_context_handle_t hContext, ze_image_handle_t hImage, ze_image_allocation_ext_properties_t* pImageAllocProperties );
    ze_result_t ( *zeModuleInspectLinkageExt )( ze_linkage_inspection_ext_desc_t* pInspectDesc, uint32_t numModules, ze_module_handle_t* phModules, ze_module_build_log_handle_t* phLog );
    ze_result_t ( *zeMemFreeExt )( ze_context_handle_t hContext, const ze_memory_free_ext_desc_t* pMemFreeDesc, void* ptr );
    ze_result_t ( *zeFabricVertexGetExp )( ze_driver_handle_t hDriver, uint32_t* pCount, ze_fabric_vertex_handle_t* phVertices );
    ze_result_t ( *zeFabricVertexGetSubVerticesExp )( ze_fabric_vertex_handle_t hVertex, uint32_t* pCount, ze_fabric_vertex_handle_t* phSubvertices );
    ze_result_t ( *zeFabricVertexGetPropertiesExp )( ze_fabric_vertex_handle_t hVertex, ze_fabric_vertex_exp_properties_t* pVertexProperties );
    ze_result_t ( *zeFabricVertexGetDeviceExp )( ze_fabric_vertex_handle_t hVertex, ze_device_handle_t* phDevice );
    ze_result_t ( *zeDeviceGetFabricVertexExp )( ze_device_handle_t hDevice, ze_fabric_vertex_handle_t* phVertex );
    ze_result_t ( *zeFabricEdgeGetExp )( ze_fabric_vertex_handle_t hVertexA, ze_fabric_vertex_handle_t hVertexB, uint32_t* pCount, ze_fabric_edge_handle_t* phEdges );
    ze_result_t ( *zeFabricEdgeGetVerticesExp )( ze_fabric_edge_handle_t hEdge, ze_fabric_vertex_handle_t* phVertexA, ze_fabric_vertex_handle_t* phVertexB );
    ze_result_t ( *zeFabricEdgeGetPropertiesExp )( ze_fabric_edge_handle_t hEdge, ze_fabric_edge_exp_properties_t* pEdgeProperties );
    ze_result_t ( *zeEventQueryKernelTimestampsExt )( ze_event_handle_t hEvent, ze_device_handle_t hDevice, uint32_t* pCount, ze_event_query_kernel_timestamps_results_ext_properties_t* pResults );
    ze_result_t ( *zeRTASBuilderCreateExp )( ze_driver_handle_t hDriver, const ze_rtas_builder_exp_desc_t* pDescriptor, ze_rtas_builder_exp_handle_t* phBuilder );
    ze_result_t ( *zeRTASBuilderGetBuildPropertiesExp )( ze_rtas_builder_exp_handle_t hBuilder, const ze_rtas_builder_build_op_exp_desc_t* pBuildOpDescriptor, ze_rtas_builder_exp_properties_t* pProperties );
    ze_result_t ( *zeDriverRTASFormatCompatibilityCheckExp )( ze_driver_handle_t hDriver, ze_rtas_format_exp_t rtasFormatA, ze_rtas_format_exp_t rtasFormatB );
    ze_result_t ( *zeRTASBuilderBuildExp )( ze_rtas_builder_exp_handle_t hBuilder, const ze_rtas_builder_build_op_exp_desc_t* pBuildOpDescriptor, void* pScratchBuffer, size_t scratchBufferSizeBytes, void* pRtasBuffer, size_t rtasBufferSizeBytes, ze_rtas_parallel_operation_exp_handle_t hParallelOperation, void* pBuildUserPtr, ze_rtas_aabb_exp_t* pBounds, size_t* pRtasBufferSizeBytes );
    ze_result_t ( *zeRTASBuilderDestroyExp )( ze_rtas_builder_exp_handle_t hBuilder );
    ze_result_t ( *zeRTASParallelOperationCreateExp )( ze_driver_handle_t hDriver, ze_rtas_parallel_operation_exp_handle_t* phParallelOperation );
    ze_result_t ( *zeRTASParallelOperationGetPropertiesExp )( ze_rtas_parallel_operation_exp_handle_t hParallelOperation, ze_rtas_parallel_operation_exp_properties_t* pProperties );
    ze_result_t ( *zeRTASParallelOperationJoinExp )( ze_rtas_parallel_operation_exp_handle_t hParallelOperation );
    ze_result_t ( *zeRTASParallelOperationDestroyExp )( ze_rtas_parallel_operation_exp_handle_t hParallelOperation );
    ze_result_t ( *zeMemGetPitchFor2dImage )( ze_context_handle_t hContext, ze_device_handle_t hDevice, size_t imageWidth, size_t imageHeight, unsigned int elementSizeInBytes, size_t * rowPitch );
    ze_result_t ( *zeImageGetDeviceOffsetExp )( ze_image_handle_t hImage, uint64_t* pDeviceOffset );
    ze_result_t ( *zeCommandListCreateCloneExp )( ze_command_list_handle_t hCommandList, ze_command_list_handle_t* phClonedCommandList );
    ze_result_t ( *zeCommandListImmediateAppendCommandListsExp )( ze_command_list_handle_t hCommandListImmediate, uint32_t numCommandLists, ze_command_list_handle_t* phCommandLists, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListGetNextCommandIdExp )( ze_command_list_handle_t hCommandList, const ze_mutable_command_id_exp_desc_t* desc, uint64_t* pCommandId );
    ze_result_t ( *zeCommandListGetNextCommandIdWithKernelsExp )( ze_command_list_handle_t hCommandList, const ze_mutable_command_id_exp_desc_t* desc, uint32_t numKernels, ze_kernel_handle_t* phKernels, uint64_t* pCommandId );
    ze_result_t ( *zeCommandListUpdateMutableCommandsExp )( ze_command_list_handle_t hCommandList, const ze_mutable_commands_exp_desc_t* desc );
    ze_result_t ( *zeCommandListUpdateMutableCommandSignalEventExp )( ze_command_list_handle_t hCommandList, uint64_t commandId, ze_event_handle_t hSignalEvent );
    ze_result_t ( *zeCommandListUpdateMutableCommandWaitEventsExp )( ze_command_list_handle_t hCommandList, uint64_t commandId, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    ze_result_t ( *zeCommandListUpdateMutableCommandKernelsExp )( ze_command_list_handle_t hCommandList, uint32_t numKernels, uint64_t* pCommandId, ze_kernel_handle_t* phKernels );
};

DLL_OBJECT extern LevelZeroFunctionTable g_levelZeroFunctionTable;

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

DLL_OBJECT void _checkZeResult(ze_result_t zeResult, const char* text, const char* locationText);
#define checkZeResult(zeResult, text) _checkZeResult(zeResult, text, __FILE__ ":" TOSTRING(__LINE__))

DLL_OBJECT void _checkZeResultDriver(ze_result_t zeResult, ze_driver_handle_t hDriver, const char* text, const char* locationText);
#define checkZeResultDriver(zeResult, hDriver, text) _checkZeResultDriver(zeResult, hDriver, text, __FILE__ ":" TOSTRING(__LINE__))

DLL_OBJECT bool initializeLevelZeroFunctionTable();
DLL_OBJECT bool getIsLevelZeroFunctionTableInitialized();
DLL_OBJECT void freeLevelZeroFunctionTable();

/**
 * Calls zeInit or zeInitDrivers and selects the closest matching Level Zero device for the passed Vulkan device.
 * @param device The Vulkan device.
 * @param zeDriver The Level Zero driver (if true is returned).
 * @param zeDevice The Level Zero device (if true is returned).
 * @return Whether a matching Level Zero device was found.
 */
DLL_OBJECT bool initializeLevelZeroAndFindMatchingDevice(
        sgl::vk::Device* device, ze_driver_handle_t* zeDriver, ze_device_handle_t* zeDevice);

#ifdef SUPPORT_SYCL_INTEROP
// https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/supported/sycl_ext_oneapi_backend_level_zero.md
DLL_OBJECT bool syclGetQueueManagesCommandList(sycl::queue& syclQueue);
DLL_OBJECT ze_command_list_handle_t syclStreamToZeCommandList(sycl::queue& syclQueue);
DLL_OBJECT ze_device_properties_t retrieveZeDevicePropertiesFromSyclQueue(sycl::queue& syclQueue);
#endif

}}

#endif //SGL_INTEROPLEVELZERO_HPP
