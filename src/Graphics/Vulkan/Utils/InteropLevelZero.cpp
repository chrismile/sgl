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

#include <map>

#include <Utils/StringUtils.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include "InteropLevelZero.hpp"

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
//#include <vulkan/vulkan_win32.h>
#endif

#ifdef SUPPORT_SYCL_INTEROP
#include <sycl/sycl.hpp>
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#endif

namespace sgl { namespace vk {

LevelZeroFunctionTable g_levelZeroFunctionTable{};

#ifdef _WIN32
HMODULE g_levelZeroLibraryHandle = nullptr;
#define dlsym GetProcAddress
#else
void* g_levelZeroLibraryHandle = nullptr;
#endif

bool initializeLevelZeroFunctionTable() {
    typedef ze_result_t ( *PFN_zeInit )( ze_init_flags_t flags );
    typedef ze_result_t ( *PFN_zeDriverGet )( uint32_t* pCount, ze_driver_handle_t* phDrivers );
    typedef ze_result_t ( *PFN_zeInitDrivers )( uint32_t* pCount, ze_driver_handle_t* phDrivers, ze_init_driver_type_desc_t* desc );
    typedef ze_result_t ( *PFN_zeDriverGetApiVersion )( ze_driver_handle_t hDriver, ze_api_version_t* version );
    typedef ze_result_t ( *PFN_zeDriverGetProperties )( ze_driver_handle_t hDriver, ze_driver_properties_t* pDriverProperties );
    typedef ze_result_t ( *PFN_zeDriverGetIpcProperties )( ze_driver_handle_t hDriver, ze_driver_ipc_properties_t* pIpcProperties );
    typedef ze_result_t ( *PFN_zeDriverGetExtensionProperties )( ze_driver_handle_t hDriver, uint32_t* pCount, ze_driver_extension_properties_t* pExtensionProperties );
    typedef ze_result_t ( *PFN_zeDriverGetExtensionFunctionAddress )( ze_driver_handle_t hDriver, const char* name, void** ppFunctionAddress );
    typedef ze_result_t ( *PFN_zeDriverGetLastErrorDescription )( ze_driver_handle_t hDriver, const char** ppString );
    typedef ze_result_t ( *PFN_zeDeviceGet )( ze_driver_handle_t hDriver, uint32_t* pCount, ze_device_handle_t* phDevices );
    typedef ze_result_t ( *PFN_zeDeviceGetRootDevice )( ze_device_handle_t hDevice, ze_device_handle_t* phRootDevice );
    typedef ze_result_t ( *PFN_zeDeviceGetSubDevices )( ze_device_handle_t hDevice, uint32_t* pCount, ze_device_handle_t* phSubdevices );
    typedef ze_result_t ( *PFN_zeDeviceGetProperties )( ze_device_handle_t hDevice, ze_device_properties_t* pDeviceProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetComputeProperties )( ze_device_handle_t hDevice, ze_device_compute_properties_t* pComputeProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetModuleProperties )( ze_device_handle_t hDevice, ze_device_module_properties_t* pModuleProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetCommandQueueGroupProperties )( ze_device_handle_t hDevice, uint32_t* pCount, ze_command_queue_group_properties_t* pCommandQueueGroupProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetMemoryProperties )( ze_device_handle_t hDevice, uint32_t* pCount, ze_device_memory_properties_t* pMemProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetMemoryAccessProperties )( ze_device_handle_t hDevice, ze_device_memory_access_properties_t* pMemAccessProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetCacheProperties )( ze_device_handle_t hDevice, uint32_t* pCount, ze_device_cache_properties_t* pCacheProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetImageProperties )( ze_device_handle_t hDevice, ze_device_image_properties_t* pImageProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetExternalMemoryProperties )( ze_device_handle_t hDevice, ze_device_external_memory_properties_t* pExternalMemoryProperties );
    typedef ze_result_t ( *PFN_zeDeviceGetP2PProperties )( ze_device_handle_t hDevice, ze_device_handle_t hPeerDevice, ze_device_p2p_properties_t* pP2PProperties );
    typedef ze_result_t ( *PFN_zeDeviceCanAccessPeer )( ze_device_handle_t hDevice, ze_device_handle_t hPeerDevice, ze_bool_t* value );
    typedef ze_result_t ( *PFN_zeDeviceGetStatus )( ze_device_handle_t hDevice );
    typedef ze_result_t ( *PFN_zeDeviceGetGlobalTimestamps )( ze_device_handle_t hDevice, uint64_t* hostTimestamp, uint64_t* deviceTimestamp );
    typedef ze_result_t ( *PFN_zeContextCreate )( ze_driver_handle_t hDriver, const ze_context_desc_t* desc, ze_context_handle_t* phContext );
    typedef ze_result_t ( *PFN_zeContextCreateEx )( ze_driver_handle_t hDriver, const ze_context_desc_t* desc, uint32_t numDevices, ze_device_handle_t* phDevices, ze_context_handle_t* phContext );
    typedef ze_result_t ( *PFN_zeContextDestroy )( ze_context_handle_t hContext );
    typedef ze_result_t ( *PFN_zeContextGetStatus )( ze_context_handle_t hContext );
    typedef ze_result_t ( *PFN_zeCommandQueueCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* desc, ze_command_queue_handle_t* phCommandQueue );
    typedef ze_result_t ( *PFN_zeCommandQueueDestroy )( ze_command_queue_handle_t hCommandQueue );
    typedef ze_result_t ( *PFN_zeCommandQueueExecuteCommandLists )( ze_command_queue_handle_t hCommandQueue, uint32_t numCommandLists, ze_command_list_handle_t* phCommandLists, ze_fence_handle_t hFence );
    typedef ze_result_t ( *PFN_zeCommandQueueSynchronize )( ze_command_queue_handle_t hCommandQueue, uint64_t timeout );
    typedef ze_result_t ( *PFN_zeCommandQueueGetOrdinal )( ze_command_queue_handle_t hCommandQueue, uint32_t* pOrdinal );
    typedef ze_result_t ( *PFN_zeCommandQueueGetIndex )( ze_command_queue_handle_t hCommandQueue, uint32_t* pIndex );
    typedef ze_result_t ( *PFN_zeCommandListCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_list_desc_t* desc, ze_command_list_handle_t* phCommandList );
    typedef ze_result_t ( *PFN_zeCommandListCreateImmediate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_command_queue_desc_t* altdesc, ze_command_list_handle_t* phCommandList );
    typedef ze_result_t ( *PFN_zeCommandListDestroy )( ze_command_list_handle_t hCommandList );
    typedef ze_result_t ( *PFN_zeCommandListClose )( ze_command_list_handle_t hCommandList );
    typedef ze_result_t ( *PFN_zeCommandListReset )( ze_command_list_handle_t hCommandList );
    typedef ze_result_t ( *PFN_zeCommandListAppendWriteGlobalTimestamp )( ze_command_list_handle_t hCommandList, uint64_t* dstptr, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListHostSynchronize )( ze_command_list_handle_t hCommandList, uint64_t timeout );
    typedef ze_result_t ( *PFN_zeCommandListGetDeviceHandle )( ze_command_list_handle_t hCommandList, ze_device_handle_t* phDevice );
    typedef ze_result_t ( *PFN_zeCommandListGetContextHandle )( ze_command_list_handle_t hCommandList, ze_context_handle_t* phContext );
    typedef ze_result_t ( *PFN_zeCommandListGetOrdinal )( ze_command_list_handle_t hCommandList, uint32_t* pOrdinal );
    typedef ze_result_t ( *PFN_zeCommandListImmediateGetIndex )( ze_command_list_handle_t hCommandListImmediate, uint32_t* pIndex );
    typedef ze_result_t ( *PFN_zeCommandListIsImmediate )( ze_command_list_handle_t hCommandList, ze_bool_t* pIsImmediate );
    typedef ze_result_t ( *PFN_zeCommandListAppendBarrier )( ze_command_list_handle_t hCommandList, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemoryRangesBarrier )( ze_command_list_handle_t hCommandList, uint32_t numRanges, const size_t* pRangeSizes, const void** pRanges, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeContextSystemBarrier )( ze_context_handle_t hContext, ze_device_handle_t hDevice );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemoryCopy )( ze_command_list_handle_t hCommandList, void* dstptr, const void* srcptr, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemoryFill )( ze_command_list_handle_t hCommandList, void* ptr, const void* pattern, size_t pattern_size, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemoryCopyRegion )( ze_command_list_handle_t hCommandList, void* dstptr, const ze_copy_region_t* dstRegion, uint32_t dstPitch, uint32_t dstSlicePitch, const void* srcptr, const ze_copy_region_t* srcRegion, uint32_t srcPitch, uint32_t srcSlicePitch, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemoryCopyFromContext )( ze_command_list_handle_t hCommandList, void* dstptr, ze_context_handle_t hContextSrc, const void* srcptr, size_t size, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendImageCopy )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, ze_image_handle_t hSrcImage, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendImageCopyRegion )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, ze_image_handle_t hSrcImage, const ze_image_region_t* pDstRegion, const ze_image_region_t* pSrcRegion, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendImageCopyToMemory )( ze_command_list_handle_t hCommandList, void* dstptr, ze_image_handle_t hSrcImage, const ze_image_region_t* pSrcRegion, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendImageCopyFromMemory )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, const void* srcptr, const ze_image_region_t* pDstRegion, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemoryPrefetch )( ze_command_list_handle_t hCommandList, const void* ptr, size_t size );
    typedef ze_result_t ( *PFN_zeCommandListAppendMemAdvise )( ze_command_list_handle_t hCommandList, ze_device_handle_t hDevice, const void* ptr, size_t size, ze_memory_advice_t advice );
    typedef ze_result_t ( *PFN_zeEventPoolCreate )( ze_context_handle_t hContext, const ze_event_pool_desc_t* desc, uint32_t numDevices, ze_device_handle_t* phDevices, ze_event_pool_handle_t* phEventPool );
    typedef ze_result_t ( *PFN_zeEventPoolDestroy )( ze_event_pool_handle_t hEventPool );
    typedef ze_result_t ( *PFN_zeEventCreate )( ze_event_pool_handle_t hEventPool, const ze_event_desc_t* desc, ze_event_handle_t* phEvent );
    typedef ze_result_t ( *PFN_zeEventDestroy )( ze_event_handle_t hEvent );
    typedef ze_result_t ( *PFN_zeEventPoolGetIpcHandle )( ze_event_pool_handle_t hEventPool, ze_ipc_event_pool_handle_t* phIpc );
    typedef ze_result_t ( *PFN_zeEventPoolPutIpcHandle )( ze_context_handle_t hContext, ze_ipc_event_pool_handle_t hIpc );
    typedef ze_result_t ( *PFN_zeEventPoolOpenIpcHandle )( ze_context_handle_t hContext, ze_ipc_event_pool_handle_t hIpc, ze_event_pool_handle_t* phEventPool );
    typedef ze_result_t ( *PFN_zeEventPoolCloseIpcHandle )( ze_event_pool_handle_t hEventPool );
    typedef ze_result_t ( *PFN_zeCommandListAppendSignalEvent )( ze_command_list_handle_t hCommandList, ze_event_handle_t hEvent );
    typedef ze_result_t ( *PFN_zeCommandListAppendWaitOnEvents )( ze_command_list_handle_t hCommandList, uint32_t numEvents, ze_event_handle_t* phEvents );
    typedef ze_result_t ( *PFN_zeEventHostSignal )( ze_event_handle_t hEvent );
    typedef ze_result_t ( *PFN_zeEventHostSynchronize )( ze_event_handle_t hEvent, uint64_t timeout );
    typedef ze_result_t ( *PFN_zeEventQueryStatus )( ze_event_handle_t hEvent );
    typedef ze_result_t ( *PFN_zeCommandListAppendEventReset )( ze_command_list_handle_t hCommandList, ze_event_handle_t hEvent );
    typedef ze_result_t ( *PFN_zeEventHostReset )( ze_event_handle_t hEvent );
    typedef ze_result_t ( *PFN_zeEventQueryKernelTimestamp )( ze_event_handle_t hEvent, ze_kernel_timestamp_result_t* dstptr );
    typedef ze_result_t ( *PFN_zeCommandListAppendQueryKernelTimestamps )( ze_command_list_handle_t hCommandList, uint32_t numEvents, ze_event_handle_t* phEvents, void* dstptr, const size_t* pOffsets, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeEventGetEventPool )( ze_event_handle_t hEvent, ze_event_pool_handle_t* phEventPool );
    typedef ze_result_t ( *PFN_zeEventGetSignalScope )( ze_event_handle_t hEvent, ze_event_scope_flags_t* pSignalScope );
    typedef ze_result_t ( *PFN_zeEventGetWaitScope )( ze_event_handle_t hEvent, ze_event_scope_flags_t* pWaitScope );
    typedef ze_result_t ( *PFN_zeEventPoolGetContextHandle )( ze_event_pool_handle_t hEventPool, ze_context_handle_t* phContext );
    typedef ze_result_t ( *PFN_zeEventPoolGetFlags )( ze_event_pool_handle_t hEventPool, ze_event_pool_flags_t* pFlags );
    typedef ze_result_t ( *PFN_zeFenceCreate )( ze_command_queue_handle_t hCommandQueue, const ze_fence_desc_t* desc, ze_fence_handle_t* phFence );
    typedef ze_result_t ( *PFN_zeFenceDestroy )( ze_fence_handle_t hFence );
    typedef ze_result_t ( *PFN_zeFenceHostSynchronize )( ze_fence_handle_t hFence, uint64_t timeout );
    typedef ze_result_t ( *PFN_zeFenceQueryStatus )( ze_fence_handle_t hFence );
    typedef ze_result_t ( *PFN_zeFenceReset )( ze_fence_handle_t hFence );
    typedef ze_result_t ( *PFN_zeImageGetProperties )( ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_properties_t* pImageProperties );
    typedef ze_result_t ( *PFN_zeImageCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_handle_t* phImage );
    typedef ze_result_t ( *PFN_zeImageDestroy )( ze_image_handle_t hImage );
    typedef ze_result_t ( *PFN_zeMemAllocShared )( ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, const ze_host_mem_alloc_desc_t* host_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr );
    typedef ze_result_t ( *PFN_zeMemAllocDevice )( ze_context_handle_t hContext, const ze_device_mem_alloc_desc_t* device_desc, size_t size, size_t alignment, ze_device_handle_t hDevice, void** pptr );
    typedef ze_result_t ( *PFN_zeMemAllocHost )( ze_context_handle_t hContext, const ze_host_mem_alloc_desc_t* host_desc, size_t size, size_t alignment, void** pptr );
    typedef ze_result_t ( *PFN_zeMemFree )( ze_context_handle_t hContext, void* ptr );
    typedef ze_result_t ( *PFN_zeMemGetAllocProperties )( ze_context_handle_t hContext, const void* ptr, ze_memory_allocation_properties_t* pMemAllocProperties, ze_device_handle_t* phDevice );
    typedef ze_result_t ( *PFN_zeMemGetAddressRange )( ze_context_handle_t hContext, const void* ptr, void** pBase, size_t* pSize );
    typedef ze_result_t ( *PFN_zeMemGetIpcHandle )( ze_context_handle_t hContext, const void* ptr, ze_ipc_mem_handle_t* pIpcHandle );
    typedef ze_result_t ( *PFN_zeMemGetIpcHandleFromFileDescriptorExp )( ze_context_handle_t hContext, uint64_t handle, ze_ipc_mem_handle_t* pIpcHandle );
    typedef ze_result_t ( *PFN_zeMemGetFileDescriptorFromIpcHandleExp )( ze_context_handle_t hContext, ze_ipc_mem_handle_t ipcHandle, uint64_t* pHandle );
    typedef ze_result_t ( *PFN_zeMemPutIpcHandle )( ze_context_handle_t hContext, ze_ipc_mem_handle_t handle );
    typedef ze_result_t ( *PFN_zeMemOpenIpcHandle )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_ipc_mem_handle_t handle, ze_ipc_memory_flags_t flags, void** pptr );
    typedef ze_result_t ( *PFN_zeMemCloseIpcHandle )( ze_context_handle_t hContext, const void* ptr );
    typedef ze_result_t ( *PFN_zeMemSetAtomicAccessAttributeExp )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const void* ptr, size_t size, ze_memory_atomic_attr_exp_flags_t attr );
    typedef ze_result_t ( *PFN_zeMemGetAtomicAccessAttributeExp )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const void* ptr, size_t size, ze_memory_atomic_attr_exp_flags_t* pAttr );
    typedef ze_result_t ( *PFN_zeModuleCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_module_desc_t* desc, ze_module_handle_t* phModule, ze_module_build_log_handle_t* phBuildLog );
    typedef ze_result_t ( *PFN_zeModuleDestroy )( ze_module_handle_t hModule );
    typedef ze_result_t ( *PFN_zeModuleDynamicLink )( uint32_t numModules, ze_module_handle_t* phModules, ze_module_build_log_handle_t* phLinkLog );
    typedef ze_result_t ( *PFN_zeModuleBuildLogDestroy )( ze_module_build_log_handle_t hModuleBuildLog );
    typedef ze_result_t ( *PFN_zeModuleBuildLogGetString )( ze_module_build_log_handle_t hModuleBuildLog, size_t* pSize, char* pBuildLog );
    typedef ze_result_t ( *PFN_zeModuleGetNativeBinary )( ze_module_handle_t hModule, size_t* pSize, uint8_t* pModuleNativeBinary );
    typedef ze_result_t ( *PFN_zeModuleGetGlobalPointer )( ze_module_handle_t hModule, const char* pGlobalName, size_t* pSize, void** pptr );
    typedef ze_result_t ( *PFN_zeModuleGetKernelNames )( ze_module_handle_t hModule, uint32_t* pCount, const char** pNames );
    typedef ze_result_t ( *PFN_zeModuleGetProperties )( ze_module_handle_t hModule, ze_module_properties_t* pModuleProperties );
    typedef ze_result_t ( *PFN_zeKernelCreate )( ze_module_handle_t hModule, const ze_kernel_desc_t* desc, ze_kernel_handle_t* phKernel );
    typedef ze_result_t ( *PFN_zeKernelDestroy )( ze_kernel_handle_t hKernel );
    typedef ze_result_t ( *PFN_zeModuleGetFunctionPointer )( ze_module_handle_t hModule, const char* pFunctionName, void** pfnFunction );
    typedef ze_result_t ( *PFN_zeKernelSetGroupSize )( ze_kernel_handle_t hKernel, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ );
    typedef ze_result_t ( *PFN_zeKernelSuggestGroupSize )( ze_kernel_handle_t hKernel, uint32_t globalSizeX, uint32_t globalSizeY, uint32_t globalSizeZ, uint32_t* groupSizeX, uint32_t* groupSizeY, uint32_t* groupSizeZ );
    typedef ze_result_t ( *PFN_zeKernelSuggestMaxCooperativeGroupCount )( ze_kernel_handle_t hKernel, uint32_t* totalGroupCount );
    typedef ze_result_t ( *PFN_zeKernelSetArgumentValue )( ze_kernel_handle_t hKernel, uint32_t argIndex, size_t argSize, const void* pArgValue );
    typedef ze_result_t ( *PFN_zeKernelSetIndirectAccess )( ze_kernel_handle_t hKernel, ze_kernel_indirect_access_flags_t flags );
    typedef ze_result_t ( *PFN_zeKernelGetIndirectAccess )( ze_kernel_handle_t hKernel, ze_kernel_indirect_access_flags_t* pFlags );
    typedef ze_result_t ( *PFN_zeKernelGetSourceAttributes )( ze_kernel_handle_t hKernel, uint32_t* pSize, char** pString );
    typedef ze_result_t ( *PFN_zeKernelSetCacheConfig )( ze_kernel_handle_t hKernel, ze_cache_config_flags_t flags );
    typedef ze_result_t ( *PFN_zeKernelGetProperties )( ze_kernel_handle_t hKernel, ze_kernel_properties_t* pKernelProperties );
    typedef ze_result_t ( *PFN_zeKernelGetName )( ze_kernel_handle_t hKernel, size_t* pSize, char* pName );
    typedef ze_result_t ( *PFN_zeCommandListAppendLaunchKernel )( ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendLaunchCooperativeKernel )( ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchFuncArgs, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendLaunchKernelIndirect )( ze_command_list_handle_t hCommandList, ze_kernel_handle_t hKernel, const ze_group_count_t* pLaunchArgumentsBuffer, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendLaunchMultipleKernelsIndirect )( ze_command_list_handle_t hCommandList, uint32_t numKernels, ze_kernel_handle_t* phKernels, const uint32_t* pCountBuffer, const ze_group_count_t* pLaunchArgumentsBuffer, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeContextMakeMemoryResident )( ze_context_handle_t hContext, ze_device_handle_t hDevice, void* ptr, size_t size );
    typedef ze_result_t ( *PFN_zeContextEvictMemory )( ze_context_handle_t hContext, ze_device_handle_t hDevice, void* ptr, size_t size );
    typedef ze_result_t ( *PFN_zeContextMakeImageResident )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_image_handle_t hImage );
    typedef ze_result_t ( *PFN_zeContextEvictImage )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_image_handle_t hImage );
    typedef ze_result_t ( *PFN_zeSamplerCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_sampler_desc_t* desc, ze_sampler_handle_t* phSampler );
    typedef ze_result_t ( *PFN_zeSamplerDestroy )( ze_sampler_handle_t hSampler );
    typedef ze_result_t ( *PFN_zeVirtualMemReserve )( ze_context_handle_t hContext, const void* pStart, size_t size, void** pptr );
    typedef ze_result_t ( *PFN_zeVirtualMemFree )( ze_context_handle_t hContext, const void* ptr, size_t size );
    typedef ze_result_t ( *PFN_zeVirtualMemQueryPageSize )( ze_context_handle_t hContext, ze_device_handle_t hDevice, size_t size, size_t* pagesize );
    typedef ze_result_t ( *PFN_zePhysicalMemCreate )( ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_physical_mem_desc_t* desc, ze_physical_mem_handle_t* phPhysicalMemory );
    typedef ze_result_t ( *PFN_zePhysicalMemDestroy )( ze_context_handle_t hContext, ze_physical_mem_handle_t hPhysicalMemory );
    typedef ze_result_t ( *PFN_zeVirtualMemMap )( ze_context_handle_t hContext, const void* ptr, size_t size, ze_physical_mem_handle_t hPhysicalMemory, size_t offset, ze_memory_access_attribute_t access );
    typedef ze_result_t ( *PFN_zeVirtualMemUnmap )( ze_context_handle_t hContext, const void* ptr, size_t size );
    typedef ze_result_t ( *PFN_zeVirtualMemSetAccessAttribute )( ze_context_handle_t hContext, const void* ptr, size_t size, ze_memory_access_attribute_t access );
    typedef ze_result_t ( *PFN_zeVirtualMemGetAccessAttribute )( ze_context_handle_t hContext, const void* ptr, size_t size, ze_memory_access_attribute_t* access, size_t* outSize );
    typedef ze_result_t ( *PFN_zeKernelSetGlobalOffsetExp )( ze_kernel_handle_t hKernel, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ );
    typedef ze_result_t ( *PFN_zeKernelGetBinaryExp )( ze_kernel_handle_t hKernel, size_t* pSize, uint8_t* pKernelBinary );
    typedef ze_result_t ( *PFN_zeDeviceImportExternalSemaphoreExt )( ze_device_handle_t hDevice, const ze_external_semaphore_ext_desc_t* desc, ze_external_semaphore_ext_handle_t* phSemaphore );
    typedef ze_result_t ( *PFN_zeDeviceReleaseExternalSemaphoreExt )( ze_external_semaphore_ext_handle_t hSemaphore );
    typedef ze_result_t ( *PFN_zeCommandListAppendSignalExternalSemaphoreExt )( ze_command_list_handle_t hCommandList, uint32_t numSemaphores, ze_external_semaphore_ext_handle_t* phSemaphores, ze_external_semaphore_signal_params_ext_t* signalParams, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendWaitExternalSemaphoreExt )( ze_command_list_handle_t hCommandList, uint32_t numSemaphores, ze_external_semaphore_ext_handle_t* phSemaphores, ze_external_semaphore_wait_params_ext_t* waitParams, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeDeviceReserveCacheExt )( ze_device_handle_t hDevice, size_t cacheLevel, size_t cacheReservationSize );
    typedef ze_result_t ( *PFN_zeDeviceSetCacheAdviceExt )( ze_device_handle_t hDevice, void* ptr, size_t regionSize, ze_cache_ext_region_t cacheRegion );
    typedef ze_result_t ( *PFN_zeEventQueryTimestampsExp )( ze_event_handle_t hEvent, ze_device_handle_t hDevice, uint32_t* pCount, ze_kernel_timestamp_result_t* pTimestamps );
    typedef ze_result_t ( *PFN_zeImageGetMemoryPropertiesExp )( ze_image_handle_t hImage, ze_image_memory_properties_exp_t* pMemoryProperties );
    typedef ze_result_t ( *PFN_zeImageViewCreateExt )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_handle_t hImage, ze_image_handle_t* phImageView );
    typedef ze_result_t ( *PFN_zeImageViewCreateExp )( ze_context_handle_t hContext, ze_device_handle_t hDevice, const ze_image_desc_t* desc, ze_image_handle_t hImage, ze_image_handle_t* phImageView );
    typedef ze_result_t ( *PFN_zeKernelSchedulingHintExp )( ze_kernel_handle_t hKernel, ze_scheduling_hint_exp_desc_t* pHint );
    typedef ze_result_t ( *PFN_zeDevicePciGetPropertiesExt )( ze_device_handle_t hDevice, ze_pci_ext_properties_t* pPciProperties );
    typedef ze_result_t ( *PFN_zeCommandListAppendImageCopyToMemoryExt )( ze_command_list_handle_t hCommandList, void* dstptr, ze_image_handle_t hSrcImage, const ze_image_region_t* pSrcRegion, uint32_t destRowPitch, uint32_t destSlicePitch, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListAppendImageCopyFromMemoryExt )( ze_command_list_handle_t hCommandList, ze_image_handle_t hDstImage, const void* srcptr, const ze_image_region_t* pDstRegion, uint32_t srcRowPitch, uint32_t srcSlicePitch, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeImageGetAllocPropertiesExt )( ze_context_handle_t hContext, ze_image_handle_t hImage, ze_image_allocation_ext_properties_t* pImageAllocProperties );
    typedef ze_result_t ( *PFN_zeModuleInspectLinkageExt )( ze_linkage_inspection_ext_desc_t* pInspectDesc, uint32_t numModules, ze_module_handle_t* phModules, ze_module_build_log_handle_t* phLog );
    typedef ze_result_t ( *PFN_zeMemFreeExt )( ze_context_handle_t hContext, const ze_memory_free_ext_desc_t* pMemFreeDesc, void* ptr );
    typedef ze_result_t ( *PFN_zeFabricVertexGetExp )( ze_driver_handle_t hDriver, uint32_t* pCount, ze_fabric_vertex_handle_t* phVertices );
    typedef ze_result_t ( *PFN_zeFabricVertexGetSubVerticesExp )( ze_fabric_vertex_handle_t hVertex, uint32_t* pCount, ze_fabric_vertex_handle_t* phSubvertices );
    typedef ze_result_t ( *PFN_zeFabricVertexGetPropertiesExp )( ze_fabric_vertex_handle_t hVertex, ze_fabric_vertex_exp_properties_t* pVertexProperties );
    typedef ze_result_t ( *PFN_zeFabricVertexGetDeviceExp )( ze_fabric_vertex_handle_t hVertex, ze_device_handle_t* phDevice );
    typedef ze_result_t ( *PFN_zeDeviceGetFabricVertexExp )( ze_device_handle_t hDevice, ze_fabric_vertex_handle_t* phVertex );
    typedef ze_result_t ( *PFN_zeFabricEdgeGetExp )( ze_fabric_vertex_handle_t hVertexA, ze_fabric_vertex_handle_t hVertexB, uint32_t* pCount, ze_fabric_edge_handle_t* phEdges );
    typedef ze_result_t ( *PFN_zeFabricEdgeGetVerticesExp )( ze_fabric_edge_handle_t hEdge, ze_fabric_vertex_handle_t* phVertexA, ze_fabric_vertex_handle_t* phVertexB );
    typedef ze_result_t ( *PFN_zeFabricEdgeGetPropertiesExp )( ze_fabric_edge_handle_t hEdge, ze_fabric_edge_exp_properties_t* pEdgeProperties );
    typedef ze_result_t ( *PFN_zeEventQueryKernelTimestampsExt )( ze_event_handle_t hEvent, ze_device_handle_t hDevice, uint32_t* pCount, ze_event_query_kernel_timestamps_results_ext_properties_t* pResults );
    typedef ze_result_t ( *PFN_zeRTASBuilderCreateExp )( ze_driver_handle_t hDriver, const ze_rtas_builder_exp_desc_t* pDescriptor, ze_rtas_builder_exp_handle_t* phBuilder );
    typedef ze_result_t ( *PFN_zeRTASBuilderGetBuildPropertiesExp )( ze_rtas_builder_exp_handle_t hBuilder, const ze_rtas_builder_build_op_exp_desc_t* pBuildOpDescriptor, ze_rtas_builder_exp_properties_t* pProperties );
    typedef ze_result_t ( *PFN_zeDriverRTASFormatCompatibilityCheckExp )( ze_driver_handle_t hDriver, ze_rtas_format_exp_t rtasFormatA, ze_rtas_format_exp_t rtasFormatB );
    typedef ze_result_t ( *PFN_zeRTASBuilderBuildExp )( ze_rtas_builder_exp_handle_t hBuilder, const ze_rtas_builder_build_op_exp_desc_t* pBuildOpDescriptor, void* pScratchBuffer, size_t scratchBufferSizeBytes, void* pRtasBuffer, size_t rtasBufferSizeBytes, ze_rtas_parallel_operation_exp_handle_t hParallelOperation, void* pBuildUserPtr, ze_rtas_aabb_exp_t* pBounds, size_t* pRtasBufferSizeBytes );
    typedef ze_result_t ( *PFN_zeRTASBuilderDestroyExp )( ze_rtas_builder_exp_handle_t hBuilder );
    typedef ze_result_t ( *PFN_zeRTASParallelOperationCreateExp )( ze_driver_handle_t hDriver, ze_rtas_parallel_operation_exp_handle_t* phParallelOperation );
    typedef ze_result_t ( *PFN_zeRTASParallelOperationGetPropertiesExp )( ze_rtas_parallel_operation_exp_handle_t hParallelOperation, ze_rtas_parallel_operation_exp_properties_t* pProperties );
    typedef ze_result_t ( *PFN_zeRTASParallelOperationJoinExp )( ze_rtas_parallel_operation_exp_handle_t hParallelOperation );
    typedef ze_result_t ( *PFN_zeRTASParallelOperationDestroyExp )( ze_rtas_parallel_operation_exp_handle_t hParallelOperation );
    typedef ze_result_t ( *PFN_zeMemGetPitchFor2dImage )( ze_context_handle_t hContext, ze_device_handle_t hDevice, size_t imageWidth, size_t imageHeight, unsigned int elementSizeInBytes, size_t * rowPitch );
    typedef ze_result_t ( *PFN_zeImageGetDeviceOffsetExp )( ze_image_handle_t hImage, uint64_t* pDeviceOffset );
    typedef ze_result_t ( *PFN_zeCommandListCreateCloneExp )( ze_command_list_handle_t hCommandList, ze_command_list_handle_t* phClonedCommandList );
    typedef ze_result_t ( *PFN_zeCommandListImmediateAppendCommandListsExp )( ze_command_list_handle_t hCommandListImmediate, uint32_t numCommandLists, ze_command_list_handle_t* phCommandLists, ze_event_handle_t hSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListGetNextCommandIdExp )( ze_command_list_handle_t hCommandList, const ze_mutable_command_id_exp_desc_t* desc, uint64_t* pCommandId );
    typedef ze_result_t ( *PFN_zeCommandListGetNextCommandIdWithKernelsExp )( ze_command_list_handle_t hCommandList, const ze_mutable_command_id_exp_desc_t* desc, uint32_t numKernels, ze_kernel_handle_t* phKernels, uint64_t* pCommandId );
    typedef ze_result_t ( *PFN_zeCommandListUpdateMutableCommandsExp )( ze_command_list_handle_t hCommandList, const ze_mutable_commands_exp_desc_t* desc );
    typedef ze_result_t ( *PFN_zeCommandListUpdateMutableCommandSignalEventExp )( ze_command_list_handle_t hCommandList, uint64_t commandId, ze_event_handle_t hSignalEvent );
    typedef ze_result_t ( *PFN_zeCommandListUpdateMutableCommandWaitEventsExp )( ze_command_list_handle_t hCommandList, uint64_t commandId, uint32_t numWaitEvents, ze_event_handle_t* phWaitEvents );
    typedef ze_result_t ( *PFN_zeCommandListUpdateMutableCommandKernelsExp )( ze_command_list_handle_t hCommandList, uint32_t numKernels, uint64_t* pCommandId, ze_kernel_handle_t* phKernels );

#if defined(__linux__)
    g_levelZeroLibraryHandle = dlopen("libze_loader.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!g_levelZeroLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeLevelZeroFunctionTable: Could not load libze_loader.so.1.");
        return false;
    }
#elif defined(_WIN32)
    g_levelZeroLibraryHandle = LoadLibraryA("ze_loader.dll");
    if (!g_levelZeroLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeLevelZeroFunctionTable: Could not load ze_loader.dll.");
        return false;
    }
#endif
    g_levelZeroFunctionTable.zeInit = PFN_zeInit(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeInit)));
    g_levelZeroFunctionTable.zeDriverGet = PFN_zeDriverGet(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGet)));
    g_levelZeroFunctionTable.zeInitDrivers = PFN_zeInitDrivers(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeInitDrivers)));
    g_levelZeroFunctionTable.zeDriverGetApiVersion = PFN_zeDriverGetApiVersion(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGetApiVersion)));
    g_levelZeroFunctionTable.zeDriverGetProperties = PFN_zeDriverGetProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGetProperties)));
    g_levelZeroFunctionTable.zeDriverGetIpcProperties = PFN_zeDriverGetIpcProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGetIpcProperties)));
    g_levelZeroFunctionTable.zeDriverGetExtensionProperties = PFN_zeDriverGetExtensionProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGetExtensionProperties)));
    g_levelZeroFunctionTable.zeDriverGetExtensionFunctionAddress = PFN_zeDriverGetExtensionFunctionAddress(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGetExtensionFunctionAddress)));
    g_levelZeroFunctionTable.zeDriverGetLastErrorDescription = PFN_zeDriverGetLastErrorDescription(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverGetLastErrorDescription)));
    g_levelZeroFunctionTable.zeDeviceGet = PFN_zeDeviceGet(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGet)));
    g_levelZeroFunctionTable.zeDeviceGetRootDevice = PFN_zeDeviceGetRootDevice(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetRootDevice)));
    g_levelZeroFunctionTable.zeDeviceGetSubDevices = PFN_zeDeviceGetSubDevices(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetSubDevices)));
    g_levelZeroFunctionTable.zeDeviceGetProperties = PFN_zeDeviceGetProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetProperties)));
    g_levelZeroFunctionTable.zeDeviceGetComputeProperties = PFN_zeDeviceGetComputeProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetComputeProperties)));
    g_levelZeroFunctionTable.zeDeviceGetModuleProperties = PFN_zeDeviceGetModuleProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetModuleProperties)));
    g_levelZeroFunctionTable.zeDeviceGetCommandQueueGroupProperties = PFN_zeDeviceGetCommandQueueGroupProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetCommandQueueGroupProperties)));
    g_levelZeroFunctionTable.zeDeviceGetMemoryProperties = PFN_zeDeviceGetMemoryProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetMemoryProperties)));
    g_levelZeroFunctionTable.zeDeviceGetMemoryAccessProperties = PFN_zeDeviceGetMemoryAccessProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetMemoryAccessProperties)));
    g_levelZeroFunctionTable.zeDeviceGetCacheProperties = PFN_zeDeviceGetCacheProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetCacheProperties)));
    g_levelZeroFunctionTable.zeDeviceGetImageProperties = PFN_zeDeviceGetImageProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetImageProperties)));
    g_levelZeroFunctionTable.zeDeviceGetExternalMemoryProperties = PFN_zeDeviceGetExternalMemoryProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetExternalMemoryProperties)));
    g_levelZeroFunctionTable.zeDeviceGetP2PProperties = PFN_zeDeviceGetP2PProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetP2PProperties)));
    g_levelZeroFunctionTable.zeDeviceCanAccessPeer = PFN_zeDeviceCanAccessPeer(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceCanAccessPeer)));
    g_levelZeroFunctionTable.zeDeviceGetStatus = PFN_zeDeviceGetStatus(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetStatus)));
    g_levelZeroFunctionTable.zeDeviceGetGlobalTimestamps = PFN_zeDeviceGetGlobalTimestamps(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetGlobalTimestamps)));
    g_levelZeroFunctionTable.zeContextCreate = PFN_zeContextCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextCreate)));
    g_levelZeroFunctionTable.zeContextCreateEx = PFN_zeContextCreateEx(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextCreateEx)));
    g_levelZeroFunctionTable.zeContextDestroy = PFN_zeContextDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextDestroy)));
    g_levelZeroFunctionTable.zeContextGetStatus = PFN_zeContextGetStatus(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextGetStatus)));
    g_levelZeroFunctionTable.zeCommandQueueCreate = PFN_zeCommandQueueCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandQueueCreate)));
    g_levelZeroFunctionTable.zeCommandQueueDestroy = PFN_zeCommandQueueDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandQueueDestroy)));
    g_levelZeroFunctionTable.zeCommandQueueExecuteCommandLists = PFN_zeCommandQueueExecuteCommandLists(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandQueueExecuteCommandLists)));
    g_levelZeroFunctionTable.zeCommandQueueSynchronize = PFN_zeCommandQueueSynchronize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandQueueSynchronize)));
    g_levelZeroFunctionTable.zeCommandQueueGetOrdinal = PFN_zeCommandQueueGetOrdinal(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandQueueGetOrdinal)));
    g_levelZeroFunctionTable.zeCommandQueueGetIndex = PFN_zeCommandQueueGetIndex(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandQueueGetIndex)));
    g_levelZeroFunctionTable.zeCommandListCreate = PFN_zeCommandListCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListCreate)));
    g_levelZeroFunctionTable.zeCommandListCreateImmediate = PFN_zeCommandListCreateImmediate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListCreateImmediate)));
    g_levelZeroFunctionTable.zeCommandListDestroy = PFN_zeCommandListDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListDestroy)));
    g_levelZeroFunctionTable.zeCommandListClose = PFN_zeCommandListClose(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListClose)));
    g_levelZeroFunctionTable.zeCommandListReset = PFN_zeCommandListReset(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListReset)));
    g_levelZeroFunctionTable.zeCommandListAppendWriteGlobalTimestamp = PFN_zeCommandListAppendWriteGlobalTimestamp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendWriteGlobalTimestamp)));
    g_levelZeroFunctionTable.zeCommandListHostSynchronize = PFN_zeCommandListHostSynchronize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListHostSynchronize)));
    g_levelZeroFunctionTable.zeCommandListGetDeviceHandle = PFN_zeCommandListGetDeviceHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListGetDeviceHandle)));
    g_levelZeroFunctionTable.zeCommandListGetContextHandle = PFN_zeCommandListGetContextHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListGetContextHandle)));
    g_levelZeroFunctionTable.zeCommandListGetOrdinal = PFN_zeCommandListGetOrdinal(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListGetOrdinal)));
    g_levelZeroFunctionTable.zeCommandListImmediateGetIndex = PFN_zeCommandListImmediateGetIndex(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListImmediateGetIndex)));
    g_levelZeroFunctionTable.zeCommandListIsImmediate = PFN_zeCommandListIsImmediate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListIsImmediate)));
    g_levelZeroFunctionTable.zeCommandListAppendBarrier = PFN_zeCommandListAppendBarrier(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendBarrier)));
    g_levelZeroFunctionTable.zeCommandListAppendMemoryRangesBarrier = PFN_zeCommandListAppendMemoryRangesBarrier(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemoryRangesBarrier)));
    g_levelZeroFunctionTable.zeContextSystemBarrier = PFN_zeContextSystemBarrier(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextSystemBarrier)));
    g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy = PFN_zeCommandListAppendMemoryCopy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemoryCopy)));
    g_levelZeroFunctionTable.zeCommandListAppendMemoryFill = PFN_zeCommandListAppendMemoryFill(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemoryFill)));
    g_levelZeroFunctionTable.zeCommandListAppendMemoryCopyRegion = PFN_zeCommandListAppendMemoryCopyRegion(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemoryCopyRegion)));
    g_levelZeroFunctionTable.zeCommandListAppendMemoryCopyFromContext = PFN_zeCommandListAppendMemoryCopyFromContext(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemoryCopyFromContext)));
    g_levelZeroFunctionTable.zeCommandListAppendImageCopy = PFN_zeCommandListAppendImageCopy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendImageCopy)));
    g_levelZeroFunctionTable.zeCommandListAppendImageCopyRegion = PFN_zeCommandListAppendImageCopyRegion(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendImageCopyRegion)));
    g_levelZeroFunctionTable.zeCommandListAppendImageCopyToMemory = PFN_zeCommandListAppendImageCopyToMemory(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendImageCopyToMemory)));
    g_levelZeroFunctionTable.zeCommandListAppendImageCopyFromMemory = PFN_zeCommandListAppendImageCopyFromMemory(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendImageCopyFromMemory)));
    g_levelZeroFunctionTable.zeCommandListAppendMemoryPrefetch = PFN_zeCommandListAppendMemoryPrefetch(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemoryPrefetch)));
    g_levelZeroFunctionTable.zeCommandListAppendMemAdvise = PFN_zeCommandListAppendMemAdvise(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendMemAdvise)));
    g_levelZeroFunctionTable.zeEventPoolCreate = PFN_zeEventPoolCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolCreate)));
    g_levelZeroFunctionTable.zeEventPoolDestroy = PFN_zeEventPoolDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolDestroy)));
    g_levelZeroFunctionTable.zeEventCreate = PFN_zeEventCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventCreate)));
    g_levelZeroFunctionTable.zeEventDestroy = PFN_zeEventDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventDestroy)));
    g_levelZeroFunctionTable.zeEventPoolGetIpcHandle = PFN_zeEventPoolGetIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolGetIpcHandle)));
    g_levelZeroFunctionTable.zeEventPoolPutIpcHandle = PFN_zeEventPoolPutIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolPutIpcHandle)));
    g_levelZeroFunctionTable.zeEventPoolOpenIpcHandle = PFN_zeEventPoolOpenIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolOpenIpcHandle)));
    g_levelZeroFunctionTable.zeEventPoolCloseIpcHandle = PFN_zeEventPoolCloseIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolCloseIpcHandle)));
    g_levelZeroFunctionTable.zeCommandListAppendSignalEvent = PFN_zeCommandListAppendSignalEvent(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendSignalEvent)));
    g_levelZeroFunctionTable.zeCommandListAppendWaitOnEvents = PFN_zeCommandListAppendWaitOnEvents(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendWaitOnEvents)));
    g_levelZeroFunctionTable.zeEventHostSignal = PFN_zeEventHostSignal(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventHostSignal)));
    g_levelZeroFunctionTable.zeEventHostSynchronize = PFN_zeEventHostSynchronize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventHostSynchronize)));
    g_levelZeroFunctionTable.zeEventQueryStatus = PFN_zeEventQueryStatus(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventQueryStatus)));
    g_levelZeroFunctionTable.zeCommandListAppendEventReset = PFN_zeCommandListAppendEventReset(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendEventReset)));
    g_levelZeroFunctionTable.zeEventHostReset = PFN_zeEventHostReset(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventHostReset)));
    g_levelZeroFunctionTable.zeEventQueryKernelTimestamp = PFN_zeEventQueryKernelTimestamp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventQueryKernelTimestamp)));
    g_levelZeroFunctionTable.zeCommandListAppendQueryKernelTimestamps = PFN_zeCommandListAppendQueryKernelTimestamps(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendQueryKernelTimestamps)));
    g_levelZeroFunctionTable.zeEventGetEventPool = PFN_zeEventGetEventPool(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventGetEventPool)));
    g_levelZeroFunctionTable.zeEventGetSignalScope = PFN_zeEventGetSignalScope(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventGetSignalScope)));
    g_levelZeroFunctionTable.zeEventGetWaitScope = PFN_zeEventGetWaitScope(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventGetWaitScope)));
    g_levelZeroFunctionTable.zeEventPoolGetContextHandle = PFN_zeEventPoolGetContextHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolGetContextHandle)));
    g_levelZeroFunctionTable.zeEventPoolGetFlags = PFN_zeEventPoolGetFlags(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventPoolGetFlags)));
    g_levelZeroFunctionTable.zeFenceCreate = PFN_zeFenceCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFenceCreate)));
    g_levelZeroFunctionTable.zeFenceDestroy = PFN_zeFenceDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFenceDestroy)));
    g_levelZeroFunctionTable.zeFenceHostSynchronize = PFN_zeFenceHostSynchronize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFenceHostSynchronize)));
    g_levelZeroFunctionTable.zeFenceQueryStatus = PFN_zeFenceQueryStatus(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFenceQueryStatus)));
    g_levelZeroFunctionTable.zeFenceReset = PFN_zeFenceReset(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFenceReset)));
    g_levelZeroFunctionTable.zeImageGetProperties = PFN_zeImageGetProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageGetProperties)));
    g_levelZeroFunctionTable.zeImageCreate = PFN_zeImageCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageCreate)));
    g_levelZeroFunctionTable.zeImageDestroy = PFN_zeImageDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageDestroy)));
    g_levelZeroFunctionTable.zeMemAllocShared = PFN_zeMemAllocShared(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemAllocShared)));
    g_levelZeroFunctionTable.zeMemAllocDevice = PFN_zeMemAllocDevice(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemAllocDevice)));
    g_levelZeroFunctionTable.zeMemAllocHost = PFN_zeMemAllocHost(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemAllocHost)));
    g_levelZeroFunctionTable.zeMemFree = PFN_zeMemFree(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemFree)));
    g_levelZeroFunctionTable.zeMemGetAllocProperties = PFN_zeMemGetAllocProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetAllocProperties)));
    g_levelZeroFunctionTable.zeMemGetAddressRange = PFN_zeMemGetAddressRange(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetAddressRange)));
    g_levelZeroFunctionTable.zeMemGetIpcHandle = PFN_zeMemGetIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetIpcHandle)));
    g_levelZeroFunctionTable.zeMemGetIpcHandleFromFileDescriptorExp = PFN_zeMemGetIpcHandleFromFileDescriptorExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetIpcHandleFromFileDescriptorExp)));
    g_levelZeroFunctionTable.zeMemGetFileDescriptorFromIpcHandleExp = PFN_zeMemGetFileDescriptorFromIpcHandleExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetFileDescriptorFromIpcHandleExp)));
    g_levelZeroFunctionTable.zeMemPutIpcHandle = PFN_zeMemPutIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemPutIpcHandle)));
    g_levelZeroFunctionTable.zeMemOpenIpcHandle = PFN_zeMemOpenIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemOpenIpcHandle)));
    g_levelZeroFunctionTable.zeMemCloseIpcHandle = PFN_zeMemCloseIpcHandle(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemCloseIpcHandle)));
    g_levelZeroFunctionTable.zeMemSetAtomicAccessAttributeExp = PFN_zeMemSetAtomicAccessAttributeExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemSetAtomicAccessAttributeExp)));
    g_levelZeroFunctionTable.zeMemGetAtomicAccessAttributeExp = PFN_zeMemGetAtomicAccessAttributeExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetAtomicAccessAttributeExp)));
    g_levelZeroFunctionTable.zeModuleCreate = PFN_zeModuleCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleCreate)));
    g_levelZeroFunctionTable.zeModuleDestroy = PFN_zeModuleDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleDestroy)));
    g_levelZeroFunctionTable.zeModuleDynamicLink = PFN_zeModuleDynamicLink(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleDynamicLink)));
    g_levelZeroFunctionTable.zeModuleBuildLogDestroy = PFN_zeModuleBuildLogDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleBuildLogDestroy)));
    g_levelZeroFunctionTable.zeModuleBuildLogGetString = PFN_zeModuleBuildLogGetString(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleBuildLogGetString)));
    g_levelZeroFunctionTable.zeModuleGetNativeBinary = PFN_zeModuleGetNativeBinary(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleGetNativeBinary)));
    g_levelZeroFunctionTable.zeModuleGetGlobalPointer = PFN_zeModuleGetGlobalPointer(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleGetGlobalPointer)));
    g_levelZeroFunctionTable.zeModuleGetKernelNames = PFN_zeModuleGetKernelNames(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleGetKernelNames)));
    g_levelZeroFunctionTable.zeModuleGetProperties = PFN_zeModuleGetProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleGetProperties)));
    g_levelZeroFunctionTable.zeKernelCreate = PFN_zeKernelCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelCreate)));
    g_levelZeroFunctionTable.zeKernelDestroy = PFN_zeKernelDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelDestroy)));
    g_levelZeroFunctionTable.zeModuleGetFunctionPointer = PFN_zeModuleGetFunctionPointer(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleGetFunctionPointer)));
    g_levelZeroFunctionTable.zeKernelSetGroupSize = PFN_zeKernelSetGroupSize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSetGroupSize)));
    g_levelZeroFunctionTable.zeKernelSuggestGroupSize = PFN_zeKernelSuggestGroupSize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSuggestGroupSize)));
    g_levelZeroFunctionTable.zeKernelSuggestMaxCooperativeGroupCount = PFN_zeKernelSuggestMaxCooperativeGroupCount(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSuggestMaxCooperativeGroupCount)));
    g_levelZeroFunctionTable.zeKernelSetArgumentValue = PFN_zeKernelSetArgumentValue(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSetArgumentValue)));
    g_levelZeroFunctionTable.zeKernelSetIndirectAccess = PFN_zeKernelSetIndirectAccess(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSetIndirectAccess)));
    g_levelZeroFunctionTable.zeKernelGetIndirectAccess = PFN_zeKernelGetIndirectAccess(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelGetIndirectAccess)));
    g_levelZeroFunctionTable.zeKernelGetSourceAttributes = PFN_zeKernelGetSourceAttributes(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelGetSourceAttributes)));
    g_levelZeroFunctionTable.zeKernelSetCacheConfig = PFN_zeKernelSetCacheConfig(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSetCacheConfig)));
    g_levelZeroFunctionTable.zeKernelGetProperties = PFN_zeKernelGetProperties(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelGetProperties)));
    g_levelZeroFunctionTable.zeKernelGetName = PFN_zeKernelGetName(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelGetName)));
    g_levelZeroFunctionTable.zeCommandListAppendLaunchKernel = PFN_zeCommandListAppendLaunchKernel(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendLaunchKernel)));
    g_levelZeroFunctionTable.zeCommandListAppendLaunchCooperativeKernel = PFN_zeCommandListAppendLaunchCooperativeKernel(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendLaunchCooperativeKernel)));
    g_levelZeroFunctionTable.zeCommandListAppendLaunchKernelIndirect = PFN_zeCommandListAppendLaunchKernelIndirect(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendLaunchKernelIndirect)));
    g_levelZeroFunctionTable.zeCommandListAppendLaunchMultipleKernelsIndirect = PFN_zeCommandListAppendLaunchMultipleKernelsIndirect(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendLaunchMultipleKernelsIndirect)));
    g_levelZeroFunctionTable.zeContextMakeMemoryResident = PFN_zeContextMakeMemoryResident(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextMakeMemoryResident)));
    g_levelZeroFunctionTable.zeContextEvictMemory = PFN_zeContextEvictMemory(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextEvictMemory)));
    g_levelZeroFunctionTable.zeContextMakeImageResident = PFN_zeContextMakeImageResident(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextMakeImageResident)));
    g_levelZeroFunctionTable.zeContextEvictImage = PFN_zeContextEvictImage(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeContextEvictImage)));
    g_levelZeroFunctionTable.zeSamplerCreate = PFN_zeSamplerCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeSamplerCreate)));
    g_levelZeroFunctionTable.zeSamplerDestroy = PFN_zeSamplerDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeSamplerDestroy)));
    g_levelZeroFunctionTable.zeVirtualMemReserve = PFN_zeVirtualMemReserve(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemReserve)));
    g_levelZeroFunctionTable.zeVirtualMemFree = PFN_zeVirtualMemFree(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemFree)));
    g_levelZeroFunctionTable.zeVirtualMemQueryPageSize = PFN_zeVirtualMemQueryPageSize(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemQueryPageSize)));
    g_levelZeroFunctionTable.zePhysicalMemCreate = PFN_zePhysicalMemCreate(dlsym(g_levelZeroLibraryHandle, TOSTRING(zePhysicalMemCreate)));
    g_levelZeroFunctionTable.zePhysicalMemDestroy = PFN_zePhysicalMemDestroy(dlsym(g_levelZeroLibraryHandle, TOSTRING(zePhysicalMemDestroy)));
    g_levelZeroFunctionTable.zeVirtualMemMap = PFN_zeVirtualMemMap(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemMap)));
    g_levelZeroFunctionTable.zeVirtualMemUnmap = PFN_zeVirtualMemUnmap(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemUnmap)));
    g_levelZeroFunctionTable.zeVirtualMemSetAccessAttribute = PFN_zeVirtualMemSetAccessAttribute(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemSetAccessAttribute)));
    g_levelZeroFunctionTable.zeVirtualMemGetAccessAttribute = PFN_zeVirtualMemGetAccessAttribute(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeVirtualMemGetAccessAttribute)));
    g_levelZeroFunctionTable.zeKernelSetGlobalOffsetExp = PFN_zeKernelSetGlobalOffsetExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSetGlobalOffsetExp)));
    g_levelZeroFunctionTable.zeKernelGetBinaryExp = PFN_zeKernelGetBinaryExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelGetBinaryExp)));
    g_levelZeroFunctionTable.zeDeviceImportExternalSemaphoreExt = PFN_zeDeviceImportExternalSemaphoreExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceImportExternalSemaphoreExt)));
    g_levelZeroFunctionTable.zeDeviceReleaseExternalSemaphoreExt = PFN_zeDeviceReleaseExternalSemaphoreExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceReleaseExternalSemaphoreExt)));
    g_levelZeroFunctionTable.zeCommandListAppendSignalExternalSemaphoreExt = PFN_zeCommandListAppendSignalExternalSemaphoreExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendSignalExternalSemaphoreExt)));
    g_levelZeroFunctionTable.zeCommandListAppendWaitExternalSemaphoreExt = PFN_zeCommandListAppendWaitExternalSemaphoreExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendWaitExternalSemaphoreExt)));
    g_levelZeroFunctionTable.zeDeviceReserveCacheExt = PFN_zeDeviceReserveCacheExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceReserveCacheExt)));
    g_levelZeroFunctionTable.zeDeviceSetCacheAdviceExt = PFN_zeDeviceSetCacheAdviceExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceSetCacheAdviceExt)));
    g_levelZeroFunctionTable.zeEventQueryTimestampsExp = PFN_zeEventQueryTimestampsExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventQueryTimestampsExp)));
    g_levelZeroFunctionTable.zeImageGetMemoryPropertiesExp = PFN_zeImageGetMemoryPropertiesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageGetMemoryPropertiesExp)));
    g_levelZeroFunctionTable.zeImageViewCreateExt = PFN_zeImageViewCreateExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageViewCreateExt)));
    g_levelZeroFunctionTable.zeImageViewCreateExp = PFN_zeImageViewCreateExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageViewCreateExp)));
    g_levelZeroFunctionTable.zeKernelSchedulingHintExp = PFN_zeKernelSchedulingHintExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeKernelSchedulingHintExp)));
    g_levelZeroFunctionTable.zeDevicePciGetPropertiesExt = PFN_zeDevicePciGetPropertiesExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDevicePciGetPropertiesExt)));
    g_levelZeroFunctionTable.zeCommandListAppendImageCopyToMemoryExt = PFN_zeCommandListAppendImageCopyToMemoryExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendImageCopyToMemoryExt)));
    g_levelZeroFunctionTable.zeCommandListAppendImageCopyFromMemoryExt = PFN_zeCommandListAppendImageCopyFromMemoryExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListAppendImageCopyFromMemoryExt)));
    g_levelZeroFunctionTable.zeImageGetAllocPropertiesExt = PFN_zeImageGetAllocPropertiesExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageGetAllocPropertiesExt)));
    g_levelZeroFunctionTable.zeModuleInspectLinkageExt = PFN_zeModuleInspectLinkageExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeModuleInspectLinkageExt)));
    g_levelZeroFunctionTable.zeMemFreeExt = PFN_zeMemFreeExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemFreeExt)));
    g_levelZeroFunctionTable.zeFabricVertexGetExp = PFN_zeFabricVertexGetExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricVertexGetExp)));
    g_levelZeroFunctionTable.zeFabricVertexGetSubVerticesExp = PFN_zeFabricVertexGetSubVerticesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricVertexGetSubVerticesExp)));
    g_levelZeroFunctionTable.zeFabricVertexGetPropertiesExp = PFN_zeFabricVertexGetPropertiesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricVertexGetPropertiesExp)));
    g_levelZeroFunctionTable.zeFabricVertexGetDeviceExp = PFN_zeFabricVertexGetDeviceExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricVertexGetDeviceExp)));
    g_levelZeroFunctionTable.zeDeviceGetFabricVertexExp = PFN_zeDeviceGetFabricVertexExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDeviceGetFabricVertexExp)));
    g_levelZeroFunctionTable.zeFabricEdgeGetExp = PFN_zeFabricEdgeGetExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricEdgeGetExp)));
    g_levelZeroFunctionTable.zeFabricEdgeGetVerticesExp = PFN_zeFabricEdgeGetVerticesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricEdgeGetVerticesExp)));
    g_levelZeroFunctionTable.zeFabricEdgeGetPropertiesExp = PFN_zeFabricEdgeGetPropertiesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeFabricEdgeGetPropertiesExp)));
    g_levelZeroFunctionTable.zeEventQueryKernelTimestampsExt = PFN_zeEventQueryKernelTimestampsExt(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeEventQueryKernelTimestampsExt)));
    g_levelZeroFunctionTable.zeRTASBuilderCreateExp = PFN_zeRTASBuilderCreateExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASBuilderCreateExp)));
    g_levelZeroFunctionTable.zeRTASBuilderGetBuildPropertiesExp = PFN_zeRTASBuilderGetBuildPropertiesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASBuilderGetBuildPropertiesExp)));
    g_levelZeroFunctionTable.zeDriverRTASFormatCompatibilityCheckExp = PFN_zeDriverRTASFormatCompatibilityCheckExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeDriverRTASFormatCompatibilityCheckExp)));
    g_levelZeroFunctionTable.zeRTASBuilderBuildExp = PFN_zeRTASBuilderBuildExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASBuilderBuildExp)));
    g_levelZeroFunctionTable.zeRTASBuilderDestroyExp = PFN_zeRTASBuilderDestroyExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASBuilderDestroyExp)));
    g_levelZeroFunctionTable.zeRTASParallelOperationCreateExp = PFN_zeRTASParallelOperationCreateExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASParallelOperationCreateExp)));
    g_levelZeroFunctionTable.zeRTASParallelOperationGetPropertiesExp = PFN_zeRTASParallelOperationGetPropertiesExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASParallelOperationGetPropertiesExp)));
    g_levelZeroFunctionTable.zeRTASParallelOperationJoinExp = PFN_zeRTASParallelOperationJoinExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASParallelOperationJoinExp)));
    g_levelZeroFunctionTable.zeRTASParallelOperationDestroyExp = PFN_zeRTASParallelOperationDestroyExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeRTASParallelOperationDestroyExp)));
    g_levelZeroFunctionTable.zeMemGetPitchFor2dImage = PFN_zeMemGetPitchFor2dImage(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeMemGetPitchFor2dImage)));
    g_levelZeroFunctionTable.zeImageGetDeviceOffsetExp = PFN_zeImageGetDeviceOffsetExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeImageGetDeviceOffsetExp)));
    g_levelZeroFunctionTable.zeCommandListCreateCloneExp = PFN_zeCommandListCreateCloneExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListCreateCloneExp)));
    g_levelZeroFunctionTable.zeCommandListImmediateAppendCommandListsExp = PFN_zeCommandListImmediateAppendCommandListsExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListImmediateAppendCommandListsExp)));
    g_levelZeroFunctionTable.zeCommandListGetNextCommandIdExp = PFN_zeCommandListGetNextCommandIdExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListGetNextCommandIdExp)));
    g_levelZeroFunctionTable.zeCommandListGetNextCommandIdWithKernelsExp = PFN_zeCommandListGetNextCommandIdWithKernelsExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListGetNextCommandIdWithKernelsExp)));
    g_levelZeroFunctionTable.zeCommandListUpdateMutableCommandsExp = PFN_zeCommandListUpdateMutableCommandsExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListUpdateMutableCommandsExp)));
    g_levelZeroFunctionTable.zeCommandListUpdateMutableCommandSignalEventExp = PFN_zeCommandListUpdateMutableCommandSignalEventExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListUpdateMutableCommandSignalEventExp)));
    g_levelZeroFunctionTable.zeCommandListUpdateMutableCommandWaitEventsExp = PFN_zeCommandListUpdateMutableCommandWaitEventsExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListUpdateMutableCommandWaitEventsExp)));
    g_levelZeroFunctionTable.zeCommandListUpdateMutableCommandKernelsExp = PFN_zeCommandListUpdateMutableCommandKernelsExp(dlsym(g_levelZeroLibraryHandle, TOSTRING(zeCommandListUpdateMutableCommandKernelsExp)));

    if (!g_levelZeroFunctionTable.zeInit
            || !g_levelZeroFunctionTable.zeDriverGet
            || !g_levelZeroFunctionTable.zeInitDrivers
            || !g_levelZeroFunctionTable.zeDriverGetApiVersion
            || !g_levelZeroFunctionTable.zeDriverGetProperties
            || !g_levelZeroFunctionTable.zeDriverGetLastErrorDescription

            || !g_levelZeroFunctionTable.zeDeviceGet
            || !g_levelZeroFunctionTable.zeDeviceGetProperties
            || !g_levelZeroFunctionTable.zeDeviceGetMemoryProperties
            || !g_levelZeroFunctionTable.zeDeviceGetImageProperties
            || !g_levelZeroFunctionTable.zeDeviceGetStatus

            || !g_levelZeroFunctionTable.zeContextCreate
            || !g_levelZeroFunctionTable.zeContextCreateEx
            || !g_levelZeroFunctionTable.zeContextDestroy
            || !g_levelZeroFunctionTable.zeContextGetStatus

            || !g_levelZeroFunctionTable.zeCommandQueueCreate
            || !g_levelZeroFunctionTable.zeCommandQueueDestroy
            || !g_levelZeroFunctionTable.zeCommandQueueExecuteCommandLists
            || !g_levelZeroFunctionTable.zeCommandQueueSynchronize
            || !g_levelZeroFunctionTable.zeCommandListCreate
            || !g_levelZeroFunctionTable.zeCommandListCreateImmediate
            || !g_levelZeroFunctionTable.zeCommandListDestroy
            || !g_levelZeroFunctionTable.zeCommandListClose
            || !g_levelZeroFunctionTable.zeCommandListReset
            || !g_levelZeroFunctionTable.zeCommandListHostSynchronize
            || !g_levelZeroFunctionTable.zeCommandListGetDeviceHandle
            || !g_levelZeroFunctionTable.zeCommandListGetContextHandle
            || !g_levelZeroFunctionTable.zeCommandListIsImmediate
            || !g_levelZeroFunctionTable.zeCommandListAppendBarrier
            || !g_levelZeroFunctionTable.zeCommandListAppendMemoryRangesBarrier
            || !g_levelZeroFunctionTable.zeCommandListAppendMemoryCopy
            || !g_levelZeroFunctionTable.zeCommandListAppendMemoryFill
            || !g_levelZeroFunctionTable.zeCommandListAppendMemoryCopyRegion
            || !g_levelZeroFunctionTable.zeCommandListAppendImageCopy
            || !g_levelZeroFunctionTable.zeCommandListAppendImageCopyRegion
            || !g_levelZeroFunctionTable.zeCommandListAppendImageCopyToMemory
            || !g_levelZeroFunctionTable.zeCommandListAppendImageCopyFromMemory

            || !g_levelZeroFunctionTable.zeEventPoolCreate
            || !g_levelZeroFunctionTable.zeEventPoolDestroy
            || !g_levelZeroFunctionTable.zeEventCreate
            || !g_levelZeroFunctionTable.zeEventDestroy
            || !g_levelZeroFunctionTable.zeCommandListAppendSignalEvent
            || !g_levelZeroFunctionTable.zeCommandListAppendWaitOnEvents
            || !g_levelZeroFunctionTable.zeEventHostSignal
            || !g_levelZeroFunctionTable.zeEventHostSynchronize
            || !g_levelZeroFunctionTable.zeEventQueryStatus
            || !g_levelZeroFunctionTable.zeEventHostReset

            || !g_levelZeroFunctionTable.zeFenceCreate
            || !g_levelZeroFunctionTable.zeFenceDestroy
            || !g_levelZeroFunctionTable.zeFenceHostSynchronize
            || !g_levelZeroFunctionTable.zeFenceQueryStatus
            || !g_levelZeroFunctionTable.zeFenceReset

            || !g_levelZeroFunctionTable.zeImageGetProperties
            || !g_levelZeroFunctionTable.zeImageCreate
            || !g_levelZeroFunctionTable.zeImageDestroy

            || !g_levelZeroFunctionTable.zeMemAllocShared
            || !g_levelZeroFunctionTable.zeMemAllocDevice
            || !g_levelZeroFunctionTable.zeMemAllocHost
            || !g_levelZeroFunctionTable.zeMemFree
            || !g_levelZeroFunctionTable.zeMemGetAllocProperties
            || !g_levelZeroFunctionTable.zeMemGetAddressRange

            || !g_levelZeroFunctionTable.zeModuleCreate
            || !g_levelZeroFunctionTable.zeModuleDestroy
            || !g_levelZeroFunctionTable.zeKernelCreate
            || !g_levelZeroFunctionTable.zeKernelDestroy
            || !g_levelZeroFunctionTable.zeKernelSetGroupSize
            || !g_levelZeroFunctionTable.zeKernelSuggestGroupSize
            || !g_levelZeroFunctionTable.zeKernelGetName

            || !g_levelZeroFunctionTable.zeCommandListAppendLaunchKernel

            || !g_levelZeroFunctionTable.zeSamplerCreate
            || !g_levelZeroFunctionTable.zeSamplerDestroy
            || !g_levelZeroFunctionTable.zeVirtualMemReserve
            || !g_levelZeroFunctionTable.zeVirtualMemFree
            || !g_levelZeroFunctionTable.zeVirtualMemQueryPageSize
            || !g_levelZeroFunctionTable.zePhysicalMemCreate
            || !g_levelZeroFunctionTable.zePhysicalMemDestroy
            || !g_levelZeroFunctionTable.zeVirtualMemMap
            || !g_levelZeroFunctionTable.zeVirtualMemUnmap

            || !g_levelZeroFunctionTable.zeDeviceImportExternalSemaphoreExt
            || !g_levelZeroFunctionTable.zeDeviceReleaseExternalSemaphoreExt
            || !g_levelZeroFunctionTable.zeCommandListAppendSignalExternalSemaphoreExt
            || !g_levelZeroFunctionTable.zeCommandListAppendWaitExternalSemaphoreExt

            || !g_levelZeroFunctionTable.zeMemFreeExt) {
        sgl::Logfile::get()->throwError(
                "Error in initializeLevelZeroFunctionTable: "
                "At least one function pointer could not be loaded.");
    }

    return true;
}

#ifdef _WIN32
#undef dlsym
#endif

bool getIsLevelZeroFunctionTableInitialized() {
    return g_levelZeroLibraryHandle != nullptr;
}

void freeLevelZeroFunctionTable() {
    if (g_levelZeroLibraryHandle) {
#if defined(__linux__)
        dlclose(g_levelZeroLibraryHandle);
#elif defined(_WIN32)
        FreeLibrary(g_levelZeroLibraryHandle);
#endif
        g_levelZeroLibraryHandle = {};
    }
}

static std::map<ze_result_t, std::string> zeErrorMap = {
        { ZE_RESULT_SUCCESS, "SUCCESS" },
        { ZE_RESULT_NOT_READY, "NOT_READY" },
        { ZE_RESULT_ERROR_DEVICE_LOST, "ERROR_DEVICE_LOST" },
        { ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY, "ERROR_OUT_OF_HOST_MEMORY" },
        { ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY, "ERROR_OUT_OF_DEVICE_MEMORY" },
        { ZE_RESULT_ERROR_MODULE_BUILD_FAILURE, "ERROR_MODULE_BUILD_FAILURE" },
        { ZE_RESULT_ERROR_MODULE_LINK_FAILURE, "ERROR_MODULE_LINK_FAILURE" },
        { ZE_RESULT_ERROR_DEVICE_REQUIRES_RESET, "ERROR_DEVICE_REQUIRES_RESET" },
        { ZE_RESULT_ERROR_DEVICE_IN_LOW_POWER_STATE, "ERROR_DEVICE_IN_LOW_POWER_STATE" },
        { ZE_RESULT_EXP_ERROR_DEVICE_IS_NOT_VERTEX, "EXP_ERROR_DEVICE_IS_NOT_VERTEX" },
        { ZE_RESULT_EXP_ERROR_VERTEX_IS_NOT_DEVICE, "EXP_ERROR_VERTEX_IS_NOT_DEVICE" },
        { ZE_RESULT_EXP_ERROR_REMOTE_DEVICE, "EXP_ERROR_REMOTE_DEVICE" },
        { ZE_RESULT_EXP_ERROR_OPERANDS_INCOMPATIBLE, "EXP_ERROR_OPERANDS_INCOMPATIBLE" },
        { ZE_RESULT_EXP_RTAS_BUILD_RETRY, "EXP_RTAS_BUILD_RETRY" },
        { ZE_RESULT_EXP_RTAS_BUILD_DEFERRED, "EXP_RTAS_BUILD_DEFERRED" },
        { ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, "ERROR_INSUFFICIENT_PERMISSIONS" },
        { ZE_RESULT_ERROR_NOT_AVAILABLE, "ERROR_NOT_AVAILABLE" },
        { ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE, "ERROR_DEPENDENCY_UNAVAILABLE" },
        { ZE_RESULT_WARNING_DROPPED_DATA, "WARNING_DROPPED_DATA" },
        { ZE_RESULT_ERROR_UNINITIALIZED, "ERROR_UNINITIALIZED" },
        { ZE_RESULT_ERROR_UNSUPPORTED_VERSION, "ERROR_UNSUPPORTED_VERSION" },
        { ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, "ERROR_UNSUPPORTED_FEATURE" },
        { ZE_RESULT_ERROR_INVALID_ARGUMENT, "ERROR_INVALID_ARGUMENT" },
        { ZE_RESULT_ERROR_INVALID_NULL_HANDLE, "ERROR_INVALID_NULL_HANDLE" },
        { ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE, "ERROR_HANDLE_OBJECT_IN_USE" },
        { ZE_RESULT_ERROR_INVALID_NULL_POINTER, "ERROR_INVALID_NULL_POINTER" },
        { ZE_RESULT_ERROR_INVALID_SIZE, "ERROR_INVALID_SIZE" },
        { ZE_RESULT_ERROR_UNSUPPORTED_SIZE, "ERROR_UNSUPPORTED_SIZE" },
        { ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT, "ERROR_UNSUPPORTED_ALIGNMENT" },
        { ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT, "ERROR_INVALID_SYNCHRONIZATION_OBJECT" },
        { ZE_RESULT_ERROR_INVALID_ENUMERATION, "ERROR_INVALID_ENUMERATION" },
        { ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION, "ERROR_UNSUPPORTED_ENUMERATION" },
        { ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT, "ERROR_UNSUPPORTED_IMAGE_FORMAT" },
        { ZE_RESULT_ERROR_INVALID_NATIVE_BINARY, "ERROR_INVALID_NATIVE_BINARY" },
        { ZE_RESULT_ERROR_INVALID_GLOBAL_NAME, "ERROR_INVALID_GLOBAL_NAME" },
        { ZE_RESULT_ERROR_INVALID_KERNEL_NAME, "ERROR_INVALID_KERNEL_NAME" },
        { ZE_RESULT_ERROR_INVALID_FUNCTION_NAME, "ERROR_INVALID_FUNCTION_NAME" },
        { ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION, "ERROR_INVALID_GROUP_SIZE_DIMENSION" },
        { ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION, "ERROR_INVALID_GLOBAL_WIDTH_DIMENSION" },
        { ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX, "ERROR_INVALID_KERNEL_ARGUMENT_INDEX" },
        { ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE, "ERROR_INVALID_KERNEL_ARGUMENT_SIZE" },
        { ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE, "ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE" },
        { ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED, "ERROR_INVALID_MODULE_UNLINKED" },
        { ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE, "ERROR_INVALID_COMMAND_LIST_TYPE" },
        { ZE_RESULT_ERROR_OVERLAPPING_REGIONS, "ERROR_OVERLAPPING_REGIONS" },
        { ZE_RESULT_WARNING_ACTION_REQUIRED, "WARNING_ACTION_REQUIRED" },
        { ZE_RESULT_ERROR_INVALID_KERNEL_HANDLE, "ERROR_INVALID_KERNEL_HANDLE" },
        { ZE_RESULT_ERROR_UNKNOWN, "ERROR_UNKNOWN" },
};

void _checkZeResult(ze_result_t zeResult, const char* text, const char* locationText) {
    if (zeResult != ZE_RESULT_SUCCESS) {
        auto it = zeErrorMap.find(zeResult);
        if (it != zeErrorMap.end()) {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + it->second);
        } else {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + std::to_string(int(zeResult)));
        }
    }
}

void _checkZeResultDriver(ze_result_t zeResult, ze_driver_handle_t hDriver, const char* text, const char* locationText) {
    if (zeResult != ZE_RESULT_SUCCESS) {
        const char* errorString = nullptr;
        zeResult = g_levelZeroFunctionTable.zeDriverGetLastErrorDescription(hDriver, &errorString);
        if (zeResult == ZE_RESULT_SUCCESS) {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + errorString);
        } else {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + "Error in zeDriverGetLastErrorDescription.");
        }
    }
}


#ifdef SUPPORT_SYCL_INTEROP
// https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/supported/sycl_ext_oneapi_backend_level_zero.md
bool syclGetQueueManagesCommandList(sycl::queue& syclQueue) {
    std::variant<ze_command_queue_handle_t, ze_command_list_handle_t> nativeQueue =
        sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue);
    ze_command_queue_handle_t* zeCommandQueuePtr = std::get_if<ze_command_queue_handle_t>(&nativeQueue);
    ze_command_list_handle_t* zeCommandListPtr = std::get_if<ze_command_list_handle_t>(&nativeQueue);
    if (!zeCommandQueuePtr && !zeCommandListPtr) {
        Logfile::get()->throwError(
                "Error in syclGetQueueManagesCommandList: "
                "SYCL queue stores neither a command queue nor command list handle.");
    }
    return zeCommandListPtr != nullptr;

}

ze_command_list_handle_t syclStreamToZeCommandList(sycl::queue& syclQueue) {
    std::variant<ze_command_queue_handle_t, ze_command_list_handle_t> nativeQueue =
            sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue);
    ze_command_queue_handle_t* zeCommandQueuePtr = std::get_if<ze_command_queue_handle_t>(&nativeQueue);
    ze_command_list_handle_t* zeCommandListPtr = std::get_if<ze_command_list_handle_t>(&nativeQueue);
    if (!zeCommandQueuePtr && !zeCommandListPtr) {
        Logfile::get()->throwError(
                "Error in syclStreamToZeCommandList: "
                "SYCL queue stores neither a command queue nor command list handle.");
    }
    if (!zeCommandListPtr) {
        Logfile::get()->throwError(
                "Error in syclStreamToZeCommandList: "
                "SYCL queue stores a command queue handle, not a command list handle.");
    }
    return *zeCommandListPtr;
}

ze_device_properties_t retrieveZeDevicePropertiesFromSyclQueue(sycl::queue& syclQueue) {
    ze_device_handle_t zeDevice = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_device());
    ze_device_properties_t zeDeviceProperties{};
    zeDeviceProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    checkZeResult(g_levelZeroFunctionTable.zeDeviceGetProperties(
            zeDevice, &zeDeviceProperties), "Error in zeDeviceGetProperties: ");
    return zeDeviceProperties;
}
#endif

}}
