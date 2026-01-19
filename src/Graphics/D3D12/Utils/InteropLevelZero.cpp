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
#include "InteropLevelZero.hpp"

namespace sgl { namespace d3d12 {

bool initializeLevelZeroAndFindMatchingDevice(
        sgl::d3d12::Device* device, ze_driver_handle_t* zeDriver, ze_device_handle_t* zeDevice) {
    uint64_t deviceLuid = device->getAdapterLuid();

    // zeInit was deprecated, but zeInitDrivers may not be available on all driver versions.
    ze_result_t zeResult;
    uint32_t driverCount = 0;
    ze_driver_handle_t* driverHandles = nullptr;
    if (g_levelZeroFunctionTable.zeInitDrivers) {
        ze_init_driver_type_desc_t initDriverTypeDesc{};
        initDriverTypeDesc.stype = ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC;
        initDriverTypeDesc.flags = ZE_INIT_DRIVER_TYPE_FLAG_GPU;
        zeResult = g_levelZeroFunctionTable.zeInitDrivers(
                &driverCount, nullptr, &initDriverTypeDesc);
        checkZeResult(zeResult, "Error in zeInitDrivers: ");
        driverHandles = new ze_driver_handle_t[driverCount];
        zeResult = g_levelZeroFunctionTable.zeInitDrivers(
                &driverCount, driverHandles, &initDriverTypeDesc);
        checkZeResult(zeResult, "Error in zeInitDrivers: ");
    } else {
        zeResult = g_levelZeroFunctionTable.zeInit(ZE_INIT_FLAG_GPU_ONLY);
        checkZeResult(zeResult, "Error in zeInit: ");
        zeResult = g_levelZeroFunctionTable.zeDriverGet(&driverCount, nullptr);
        checkZeResult(zeResult, "Error in zeDriverGet: ");
        driverHandles = new ze_driver_handle_t[driverCount];
        zeResult = g_levelZeroFunctionTable.zeDriverGet(&driverCount, driverHandles);
        checkZeResult(zeResult, "Error in zeDriverGet: ");
    }

    ze_device_luid_ext_properties_t zeDeviceLuidProperties{};
    zeDeviceLuidProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_LUID_EXT_PROPERTIES;
    ze_device_properties_t zeDeviceProperties{};
    zeDeviceProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    zeDeviceProperties.pNext = &zeDeviceLuidProperties;

    for (uint32_t driverIdx = 0; driverIdx < driverCount; driverIdx++) {
        if (!queryLevelZeroDriverSupportsExtension(driverHandles[driverIdx], ZE_DEVICE_LUID_EXT_NAME)) {
            continue;
        }

        uint32_t deviceCount = 0;
        zeResult = g_levelZeroFunctionTable.zeDeviceGet(
                driverHandles[driverIdx], &deviceCount, nullptr);
        checkZeResult(zeResult, "Error in zeDeviceGet: ");
        auto* deviceHandles = new ze_device_handle_t[deviceCount];
        zeResult = g_levelZeroFunctionTable.zeDeviceGet(
                driverHandles[driverIdx], &deviceCount, deviceHandles);
        checkZeResult(zeResult, "Error in zeDeviceGet: ");

        for (uint32_t deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
            zeResult = g_levelZeroFunctionTable.zeDeviceGetProperties(
                    deviceHandles[deviceIdx], &zeDeviceProperties);
            checkZeResult(zeResult, "Error in zeDeviceGetProperties: ");
            if (deviceLuid == *reinterpret_cast<uint64_t*>(zeDeviceLuidProperties.luid.id)) {
                *zeDriver = driverHandles[driverIdx];
                *zeDevice = deviceHandles[deviceIdx];
                delete[] deviceHandles;
                delete[] driverHandles;
                return true;
            }
        }

        delete[] deviceHandles;
    }

    delete[] driverHandles;
    return false;
}

}}
