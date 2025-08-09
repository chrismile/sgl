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

#include <Utils/File/Logfile.hpp>

#include "InteropCompute.hpp"

#ifdef SUPPORT_CUDA_INTEROP
#include "InteropCuda.hpp"
#endif

#ifdef SUPPORT_HIP_INTEROP
#include "InteropHIP.hpp"
#if HIP_VERSION_MAJOR < 6
#error Please install HIP SDK >= 6.0 for timeline semaphore support.
#endif
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
#include "InteropLevelZero.hpp"
#endif

#ifdef SUPPORT_SYCL_INTEROP
#include <sycl/sycl.hpp>
#endif

namespace sgl {

bool openMessageBoxOnComputeApiError = true;
void setOpenMessageBoxOnComputeApiError(bool _openMessageBox) {
    openMessageBoxOnComputeApiError = _openMessageBox;
}

#ifdef SUPPORT_SYCL_INTEROP
extern sycl::queue* g_syclQueue;
void setGlobalSyclQueue(sycl::queue& syclQueue) {
    g_syclQueue = &syclQueue;
}
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
ze_device_handle_t g_zeDevice = {};
ze_context_handle_t g_zeContext = {};
ze_command_queue_handle_t g_zeCommandQueue = {};
ze_event_handle_t g_zeSignalEvent = {};
uint32_t g_numWaitEvents = 0;
ze_event_handle_t* g_zeWaitEvents = {};
bool g_useBindlessImagesInterop = {};
void setLevelZeroGlobalState(ze_device_handle_t zeDevice, ze_context_handle_t zeContext) {
    g_zeDevice = zeDevice;
    g_zeContext = zeContext;
}
void setLevelZeroGlobalCommandQueue(ze_command_queue_handle_t zeCommandQueue) {
    g_zeCommandQueue = zeCommandQueue;
}
void setLevelZeroNextCommandEvents(
        ze_event_handle_t zeSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* zeWaitEvents) {
    g_zeSignalEvent = zeSignalEvent;
    g_numWaitEvents = numWaitEvents;
    g_zeWaitEvents = zeWaitEvents;
}
void setLevelZeroUseBindlessImagesInterop(bool useBindlessImages) {
    g_useBindlessImagesInterop = useBindlessImages;
}
#ifdef SUPPORT_SYCL_INTEROP
void setLevelZeroGlobalStateFromSyclQueue(sycl::queue& syclQueue) {
#ifdef SUPPORT_SYCL_INTEROP
    // Reset, as static variables may persist across GoogleTest unit tests.
    g_syclQueue = {};
#endif
    g_zeDevice = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_device());
    g_zeContext = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclQueue.get_context());
}
#endif
#endif

void resetComputeApiState() {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    g_zeDevice = {};
    g_zeContext = {};
    g_zeCommandQueue = {};
    g_zeSignalEvent = {};
    g_numWaitEvents = 0;
    g_zeWaitEvents = {};
    g_useBindlessImagesInterop = false;
#endif
#ifdef SUPPORT_SYCL_INTEROP
    g_syclQueue = {};
#endif
}

void waitForCompletion(InteropCompute interopComputeApi, StreamWrapper stream, void* event) {
#ifdef SUPPORT_CUDA_INTEROP
    if (interopComputeApi == InteropCompute::CUDA) {
        CUresult cuResult = g_cudaDeviceApiFunctionTable.cuStreamSynchronize(stream.cuStream);
        checkCUresult(cuResult, "Error in cuStreamSynchronize: ");
    }
#endif

#ifdef SUPPORT_HIP_INTEROP
    if (interopComputeApi == InteropCompute::HIP) {
        hipError_t hipResult = g_hipDeviceApiFunctionTable.hipStreamSynchronize(stream.hipStream);
        checkHipResult(hipResult, "Error in hipStreamSynchronize: ");
    }
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (interopComputeApi == InteropCompute::LEVEL_ZERO) {
        ze_result_t zeResult = g_levelZeroFunctionTable.zeCommandListClose(stream.zeCommandList);
        checkZeResult(zeResult, "Error in zeFenceCreate: ");

        if (event) {
            zeResult = g_levelZeroFunctionTable.zeCommandQueueExecuteCommandLists(
                    g_zeCommandQueue, 1, &stream.zeCommandList, nullptr);
            checkZeResult(zeResult, "Error in zeCommandQueueExecuteCommandLists: ");

            zeResult = g_levelZeroFunctionTable.zeEventHostSynchronize(
                    reinterpret_cast<ze_event_handle_t>(event), UINT64_MAX);
            checkZeResult(zeResult, "Error in zeEventHostSynchronize: ");
        } else if (g_zeCommandQueue) {
            // We could also use zeCommandQueueSynchronize instead of using a fence.
            ze_fence_desc_t fenceDesc{};
            fenceDesc.stype = ZE_STRUCTURE_TYPE_FENCE_DESC;
            ze_fence_handle_t zeFence{};
            zeResult = g_levelZeroFunctionTable.zeFenceCreate(g_zeCommandQueue, &fenceDesc, &zeFence);
            checkZeResult(zeResult, "Error in zeFenceCreate: ");

            zeResult = g_levelZeroFunctionTable.zeCommandQueueExecuteCommandLists(
                    g_zeCommandQueue, 1, &stream.zeCommandList, zeFence);
            checkZeResult(zeResult, "Error in zeCommandQueueExecuteCommandLists: ");

            zeResult = g_levelZeroFunctionTable.zeFenceHostSynchronize(zeFence, UINT64_MAX);
            checkZeResult(zeResult, "Error in zeFenceHostSynchronize: ");
            zeResult = g_levelZeroFunctionTable.zeFenceReset(zeFence);
            checkZeResult(zeResult, "Error in zeFenceReset: ");

            zeResult = g_levelZeroFunctionTable.zeFenceDestroy(zeFence);
            checkZeResult(zeResult, "Error in zeFenceDestroy: ");

            zeResult = g_levelZeroFunctionTable.zeCommandListReset(stream.zeCommandList);
            checkZeResult(zeResult, "Error in zeFenceCreate: ");
        } else {
            // We assume an immediate command list is used.
            zeResult = g_levelZeroFunctionTable.zeCommandListHostSynchronize(stream.zeCommandList, UINT64_MAX);
            checkZeResult(zeResult, "Error in zeCommandListHostSynchronize: ");
        }
    }
#endif

#ifdef SUPPORT_SYCL_INTEROP
    if (interopComputeApi == InteropCompute::SYCL) {
        if (!event) {
            sgl::Logfile::get()->throwError("sgl::vk::waitForCompletion called with nullptr SYCL event.");
        }
        sycl::event* syclEvent = reinterpret_cast<sycl::event*>(event);
        syclEvent->wait_and_throw();
    }
#endif
}

}
