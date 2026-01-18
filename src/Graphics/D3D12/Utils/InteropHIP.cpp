/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#include "Device.hpp"
#include "InteropHIP.hpp"

#include <cstring>

namespace sgl { namespace d3d12 {

bool getMatchingHipDevice(sgl::d3d12::Device* device, hipDevice_t* hipDevice) {
    uint64_t deviceLuid = device->getAdapterLuid();
    bool foundDevice = false;

    int numDevices = 0;
    hipError_t hipResult = sgl::g_hipDeviceApiFunctionTable.hipGetDeviceCount(&numDevices);
    checkHipResult(hipResult, "Error in hipGetDeviceCount: ");

    for (int deviceIdx = 0; deviceIdx < numDevices; deviceIdx++) {
        hipDevice_t currDevice = 0;
        hipResult = sgl::g_hipDeviceApiFunctionTable.hipDeviceGet(&currDevice, deviceIdx);
        checkHipResult(hipResult, "Error in hipDeviceGet: ");

        uint64_t currLuid = {};
        hipResult = sgl::g_hipDeviceApiFunctionTable.hipDeviceGetAttribute(
                reinterpret_cast<int*>(&currLuid), hipDeviceAttributeLuid, currDevice);
        checkHipResult(hipResult, "Error in hipDeviceGetUuid: ");

        if (deviceLuid == currLuid) {
            foundDevice = true;
            *hipDevice = currDevice;
            break;
        }
    }

    /*
     * hipDeviceGetUuid is not compatible with VkPhysicalDeviceIDProperties::deviceUUID:
     * https://github.com/ROCm/hipamd/issues/50
     * Unknown if this is also the case for the device LUID.
     * Use some reasonable fallback when no device could be matched.
     */
    if (!foundDevice) {
        if (device->getVendor() != DeviceVendor::AMD) {
            return false;
        }
        if (numDevices == 1) {
            hipDevice_t currDevice = 0;
            hipResult = sgl::g_hipDeviceApiFunctionTable.hipDeviceGet(&currDevice, 0);
            checkHipResult(hipResult, "Error in hipDeviceGet: ");
            foundDevice = true;
            *hipDevice = currDevice;
        } else {
            char deviceName[256];
            for (int deviceIdx = 0; deviceIdx < numDevices; deviceIdx++) {
                hipDevice_t currDevice = 0;
                hipResult = sgl::g_hipDeviceApiFunctionTable.hipDeviceGet(&currDevice, deviceIdx);
                checkHipResult(hipResult, "Error in hipDeviceGet: ");

                memset(deviceName, 0, sizeof(deviceName));
                hipResult = sgl::g_hipDeviceApiFunctionTable.hipDeviceGetName(deviceName, 255, currDevice);
                checkHipResult(hipResult, "Error in hipDeviceGetUuid: ");

                if (device->getAdapterName() == deviceName) {
                    foundDevice = true;
                    *hipDevice = currDevice;
                }
            }
        }
    }

    return foundDevice;
}

}}
