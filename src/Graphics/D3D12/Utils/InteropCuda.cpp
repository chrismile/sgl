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

#include "InteropCuda.hpp"

namespace sgl { namespace d3d12 {

bool getMatchingCudaDevice(sgl::d3d12::Device* device, CUdevice* cuDevice) {
    DeviceVendor deviceVendor = device->getVendor();
    if (deviceVendor != DeviceVendor::NVIDIA) {
        return false;
    }
    uint64_t adapterLuid = device->getAdapterLuid();
    bool foundDevice = false;

    int numDevices = 0;
    CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGetCount(&numDevices);
    sgl::checkCUresult(cuResult, "Error in cuDeviceGetCount: ");

    for (int deviceIdx = 0; deviceIdx < numDevices; deviceIdx++) {
        CUdevice currDevice = 0;
        cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGet(&currDevice, deviceIdx);
        sgl::checkCUresult(cuResult, "Error in cuDeviceGet: ");

        uint64_t currLuid = {};
        unsigned int deviceNodeMask = 0;
        cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGetLuid(
                reinterpret_cast<char*>(&currLuid), &deviceNodeMask, currDevice);
        sgl::checkCUresult(cuResult, "Error in cuDeviceGetLuid: ");

        if (adapterLuid == currLuid) {
            foundDevice = true;
            *cuDevice = currDevice;
            break;
        }
    }

    return foundDevice;
}

}}
