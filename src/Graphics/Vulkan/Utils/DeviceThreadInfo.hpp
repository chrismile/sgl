/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#ifndef SGL_DEVICETHREADINFO_HPP
#define SGL_DEVICETHREADINFO_HPP

#include <cstdint>

namespace sgl { namespace vk {
class Device;
}}

namespace sgl {

struct DLL_OBJECT DeviceThreadInfo {
    uint32_t numMultiprocessors;
    uint32_t warpSize;
    uint32_t numCoresPerMultiprocessor;
    uint32_t numCoresTotal;
    uint32_t numCudaCoresEquivalent;

    // Contains information about how to schedule threads for the persistent thread/kernel model.
    uint32_t optimalNumWorkgroupsPT;
    uint32_t optimalWorkgroupSizePT;
};

/*
 * Examples:
 * - NVIDIA RTX 3090: 82 SMs, 10496 CUDA Cores, Factor: 128
 * => numMultiprocessors = 82
 * => warpSize = 32
 * => numCoresPerMultiprocessor = 128
 * => numCoresTotal = 10496
 * - AMD Radeon RX 6900XT: 80 CUs, 5120 Stream Processors, Factor: 64
 * => numMultiprocessors = 80
 * => warpSize = 64
 * => numCoresPerMultiprocessor = 64
 * => numCoresTotal = 5120
 * => numCudaCoresEquivalent = 10240
 * - Intel HD Graphics 630: 192 FP32 ALUs, 24 EUs, 3 Subslices
 * (https://en.wikipedia.org/wiki/List_of_Intel_graphics_processing_units)
 */

DLL_OBJECT DeviceThreadInfo getDeviceThreadInfo(sgl::vk::Device* device);

#ifdef SUPPORT_CUDA_INTEROP
typedef int CUdevice;
DLL_OBJECT DeviceThreadInfo getCudaDeviceThreadInfo(CUdevice cuDevice);
#endif

}

#endif //SGL_DEVICETHREADINFO_HPP
