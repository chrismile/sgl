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

enum class InteropCompute {
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

}

#endif //SGL_INTEROPCOMPUTEAPI_HPP
