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

#ifndef SGL_INTEROPCOMPUTEAPI_HPP
#define SGL_INTEROPCOMPUTEAPI_HPP

#include <string>
#include <stdexcept>
#include <cstdint>

// Forward declarations for CUDA, HIP and Level Zero objects.

extern "C" {
#ifdef SUPPORT_CUDA_INTEROP
#if defined(_WIN64) || defined(__LP64__)
    typedef unsigned long long CUdeviceptr_v2;
#else
    typedef unsigned int CUdeviceptr_v2;
#endif
    typedef CUdeviceptr_v2 CUdeviceptr;
    typedef struct CUmipmappedArray_st *CUmipmappedArray;
    typedef struct CUarray_st *CUarray;
    typedef struct CUstream_st *CUstream;
#endif

#ifdef SUPPORT_HIP_INTEROP
    typedef void* hipDeviceptr_t;
    typedef struct hipMipmappedArray* hipMipmappedArray_t;
    typedef struct hipArray* hipArray_t;
    typedef struct ihipStream_t* hipStream_t;
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    typedef struct _ze_device_handle_t* ze_device_handle_t;
    typedef struct _ze_context_handle_t* ze_context_handle_t;
    typedef struct _ze_command_queue_handle_t* ze_command_queue_handle_t;
    typedef struct _ze_command_list_handle_t* ze_command_list_handle_t;
    typedef struct _ze_event_handle_t *ze_event_handle_t;
#endif
}

#ifdef SUPPORT_SYCL_INTEROP
namespace sycl { inline namespace _V1 {
class queue;
}}
#endif

namespace sgl {

enum class InteropComputeApi {
    NONE, CUDA, HIP, LEVEL_ZERO, SYCL
};

union DLL_OBJECT StreamWrapper {
    void* stream;
#ifdef SUPPORT_CUDA_INTEROP
    CUstream cuStream;
#endif
#ifdef SUPPORT_HIP_INTEROP
    hipStream_t hipStream;
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_command_list_handle_t zeCommandList;
#endif
#ifdef SUPPORT_SYCL_INTEROP
    sycl::queue* syclQueuePtr;
#endif
};

/**
 * An exception that can be thrown when the compute API does not support the used feature.
 */
class UnsupportedComputeApiFeatureException : public std::exception {
public:
    explicit UnsupportedComputeApiFeatureException(std::string msg) : message(std::move(msg)) {}
    [[nodiscard]] char const* what() const noexcept override { return message.c_str(); }

private:
    std::string message;
};

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
/*
 * Internally, Level Zero interop needs more information (device, context, ...) than CUDA or HIP interop.
 * The functions below can be used for setting the state globally.
 */
DLL_OBJECT void setLevelZeroGlobalState(ze_device_handle_t zeDevice, ze_context_handle_t zeContext);
DLL_OBJECT void setLevelZeroGlobalCommandQueue(ze_command_queue_handle_t zeCommandQueue);
DLL_OBJECT void setLevelZeroNextCommandEvents(
        ze_event_handle_t zeSignalEvent, uint32_t numWaitEvents, ze_event_handle_t* zeWaitEvents);
DLL_OBJECT void setLevelZeroUseBindlessImagesInterop(bool useBindlessImages);
#ifdef SUPPORT_SYCL_INTEROP
DLL_OBJECT void setLevelZeroGlobalStateFromSyclQueue(sycl::queue& syclQueue);
#endif
#endif

#ifdef SUPPORT_SYCL_INTEROP
// For more information on SYCL interop:
// https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/experimental/sycl_ext_oneapi_bindless_images.asciidoc
DLL_OBJECT void setGlobalSyclQueue(sycl::queue& syclQueue);
#endif

/// Whether a message box should be shown when a compute API error is generated that is not fatal.
DLL_OBJECT void setOpenMessageBoxOnComputeApiError(bool _openMessageBox);


/// Reset function for unit tests, as static variables may persist across GoogleTest test cases.
DLL_OBJECT void resetComputeApiState();

/**
 * Waits for completion of the stream (CUDA, HIP, Level Zero) or event (SYCL, and optionally Level Zero if not nullptr).
 * If using Level Zero, @see setLevelZeroGlobalCommandQueue must have been called.
 */
DLL_OBJECT void waitForCompletion(InteropComputeApi interopComputeApi, StreamWrapper stream, void* event = nullptr);

}

#endif //SGL_INTEROPCOMPUTEAPI_HPP
