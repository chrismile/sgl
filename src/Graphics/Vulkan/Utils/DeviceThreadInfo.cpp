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

#include <map>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include "DeviceThreadInfo.hpp"

#ifdef SUPPORT_CUDA_INTEROP
#include <Graphics/Vulkan/Utils/InteropCuda.hpp>
#endif
#ifdef SUPPORT_OPENCL_INTEROP
#include <Graphics/Vulkan/Utils/InteropOpenCL.hpp>
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
#include <Graphics/Vulkan/Utils/InteropLevelZero.hpp>
#endif

namespace sgl {

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
extern ze_device_handle_t g_zeDevice;
#endif

static std::map<uint64_t, DeviceThreadInfo> deviceThreadInfoMap;

DeviceThreadInfo getDeviceThreadInfo(sgl::vk::Device* device) {
    uint64_t deviceId = uint64_t(device->getDeviceId()) + (uint64_t(device->getVendorId()) << uint64_t(32));
    auto it = deviceThreadInfoMap.find(deviceId);
    if (it != deviceThreadInfoMap.end()) {
        return it->second;
    }

    DeviceThreadInfo info{};
    info.warpSize = device->getPhysicalDeviceSubgroupProperties().subgroupSize;
    // For the other values, just guess.
    info.numCoresPerMultiprocessor = info.warpSize;
    info.numMultiprocessors = 64;
    info.numCoresTotal = info.numMultiprocessors * info.numCoresPerMultiprocessor;
    info.numCudaCoresEquivalent = info.numCoresTotal * 2;
    info.optimalWorkgroupSizePT = info.warpSize;
    info.optimalNumWorkgroupsPT = 64;

    if (device->isDeviceExtensionSupported(VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME)
            && device->isDeviceExtensionSupported(VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME)) {
        const auto& shaderCoreProps = device->getDeviceShaderCorePropertiesAMD();
        const auto& shaderCoreProps2 = device->getDeviceShaderCoreProperties2AMD();
        sgl::Logfile::get()->write("VkPhysicalDeviceShaderCorePropertiesAMD:", sgl::BLUE);
        sgl::Logfile::get()->write("- shaderEngineCount: " + std::to_string(shaderCoreProps.shaderEngineCount), sgl::BLUE);
        sgl::Logfile::get()->write("- shaderArraysPerEngineCount: " + std::to_string(shaderCoreProps.shaderArraysPerEngineCount), sgl::BLUE);
        sgl::Logfile::get()->write("- computeUnitsPerShaderArray: " + std::to_string(shaderCoreProps.computeUnitsPerShaderArray), sgl::BLUE);
        sgl::Logfile::get()->write("- simdPerComputeUnit: " + std::to_string(shaderCoreProps.simdPerComputeUnit), sgl::BLUE);
        sgl::Logfile::get()->write("- wavefrontsPerSimd: " + std::to_string(shaderCoreProps.wavefrontsPerSimd), sgl::BLUE);
        sgl::Logfile::get()->write("- wavefrontSize: " + std::to_string(shaderCoreProps.wavefrontSize), sgl::BLUE);
        sgl::Logfile::get()->write("- sgprsPerSimd: " + std::to_string(shaderCoreProps.sgprsPerSimd), sgl::BLUE);
        sgl::Logfile::get()->write("- minSgprAllocation: " + std::to_string(shaderCoreProps.minSgprAllocation), sgl::BLUE);
        sgl::Logfile::get()->write("- maxSgprAllocation: " + std::to_string(shaderCoreProps.maxSgprAllocation), sgl::BLUE);
        sgl::Logfile::get()->write("- sgprAllocationGranularity: " + std::to_string(shaderCoreProps.sgprAllocationGranularity), sgl::BLUE);
        sgl::Logfile::get()->write("- vgprsPerSimd: " + std::to_string(shaderCoreProps.vgprsPerSimd), sgl::BLUE);
        sgl::Logfile::get()->write("- minVgprAllocation: " + std::to_string(shaderCoreProps.minVgprAllocation), sgl::BLUE);
        sgl::Logfile::get()->write("- maxVgprAllocation: " + std::to_string(shaderCoreProps.maxVgprAllocation), sgl::BLUE);
        sgl::Logfile::get()->write("- vgprAllocationGranularity: " + std::to_string(shaderCoreProps.vgprAllocationGranularity), sgl::BLUE);
        sgl::Logfile::get()->write("VkPhysicalDeviceShaderCoreProperties2AMD:", sgl::BLUE);
        sgl::Logfile::get()->write("- shaderCoreFeatures: " + std::to_string(shaderCoreProps2.shaderCoreFeatures), sgl::BLUE);
        sgl::Logfile::get()->write("- activeComputeUnitCount: " + std::to_string(shaderCoreProps2.activeComputeUnitCount), sgl::BLUE);
        info.numMultiprocessors = shaderCoreProps2.activeComputeUnitCount;
        info.numCoresPerMultiprocessor = shaderCoreProps.wavefrontSize; // == subgroupSize
        info.numCoresTotal = info.numMultiprocessors * info.numCoresPerMultiprocessor;
        info.numCudaCoresEquivalent = info.numCoresTotal * 2;
        info.optimalNumWorkgroupsPT = shaderCoreProps2.activeComputeUnitCount;
    }
#ifdef SUPPORT_CUDA_INTEROP
    else if (device->getDeviceDriverId() == VK_DRIVER_ID_NVIDIA_PROPRIETARY
            && sgl::getIsCudaDeviceApiFunctionTableInitialized()) {
        CUdevice cuDevice = 0;
        bool foundDevice = sgl::vk::getMatchingCudaDevice(device, &cuDevice);

        if (foundDevice) {
            getCudaDeviceThreadInfo(cuDevice, info);
        }
    }
#endif
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    else if ((device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS
            || device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA)
            && sgl::getIsLevelZeroFunctionTableInitialized()
            && g_zeDevice) {
        getLevelZeroDeviceThreadInfo(g_zeDevice, info);
    }
#endif
#ifdef SUPPORT_OPENCL_INTEROP
    else if (sgl::getIsOpenCLFunctionTableInitialized()) {
        cl_int res;
        cl_device_id clDevice = getMatchingOpenCLDevice(device);
        if (clDevice != nullptr) {
            cl_uint maxComputeUnits = 0;
            res = sgl::g_openclFunctionTable.clGetDeviceInfo(
                    clDevice, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &maxComputeUnits, nullptr);
            sgl::checkResultCL(res, "Error in clGetDeviceInfo[CL_DEVICE_MAX_COMPUTE_UNITS]: ");
            info.numMultiprocessors = maxComputeUnits;
            // On AMD for example, the core count is the number of CUs times the warp size. We will assume this is true.
            info.numCoresTotal = info.numMultiprocessors * info.numCoresPerMultiprocessor;
            info.numCudaCoresEquivalent = info.numCoresTotal * 2;
            info.optimalNumWorkgroupsPT = maxComputeUnits;
        }
    }
#endif

    sgl::Logfile::get()->write("Device thread info:", sgl::BLUE);
    sgl::Logfile::get()->write("- numMultiprocessors: " + std::to_string(info.numMultiprocessors), sgl::BLUE);
    sgl::Logfile::get()->write("- warpSize: " + std::to_string(info.warpSize), sgl::BLUE);
    sgl::Logfile::get()->write("- numCoresPerMultiprocessor: " + std::to_string(info.numCoresPerMultiprocessor), sgl::BLUE);
    sgl::Logfile::get()->write("- numCoresTotal: " + std::to_string(info.numCoresTotal), sgl::BLUE);
    sgl::Logfile::get()->write("- numCudaCoresEquivalent: " + std::to_string(info.numCudaCoresEquivalent), sgl::BLUE);
    sgl::Logfile::get()->write("- optimalNumWorkgroupsPT: " + std::to_string(info.optimalNumWorkgroupsPT), sgl::BLUE);
    sgl::Logfile::get()->write("- optimalWorkgroupSizePT: " + std::to_string(info.optimalWorkgroupSizePT), sgl::BLUE);
    deviceThreadInfoMap.insert(std::make_pair(deviceId, info));

    return info;
}

#ifdef SUPPORT_CUDA_INTEROP
void getCudaDeviceThreadInfo(CUdevice cuDevice, DeviceThreadInfo& info) {
    /*
     * Only use one thread block per shader multiprocessor (SM) to improve chance of fair scheduling.
     * See, e.g.: https://stackoverflow.com/questions/33150040/doubling-buffering-in-cuda-so-the-cpu-can-operate-on-data-produced-by-a-persiste/33158954#33158954%5B/
     */
    CUresult cuResult;
    int numMultiprocessors = 16;
    cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute(
            &numMultiprocessors, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, cuDevice);
    sgl::checkCUresult(cuResult, "Error in cuDeviceGetAttribute: ");
    info.numMultiprocessors = uint32_t(numMultiprocessors);

    /*
     * Use more threads than warp size. Factor 4 seems to make sense at least for RTX 3090.
     * For more details see: https://stackoverflow.com/questions/32530604/how-can-i-get-number-of-cores-in-cuda-device
     * Or: https://github.com/NVIDIA/cuda-samples/blob/master/Common/helper_cuda.h
     * https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#compute-capabilities
     * https://developer.nvidia.com/blog/inside-pascal/
     */
    int warpSize = 32;
    cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute(
            &warpSize, CU_DEVICE_ATTRIBUTE_WARP_SIZE, cuDevice);
    sgl::checkCUresult(cuResult, "Error in cuDeviceGetAttribute: ");
    info.warpSize = uint32_t(warpSize);

    int major = 0;
    cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute(
            &major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, cuDevice);
    sgl::checkCUresult(cuResult, "Error in cuDeviceGetAttribute: ");
    int minor = 0;
    cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDeviceGetAttribute(
            &minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, cuDevice);
    sgl::checkCUresult(cuResult, "Error in cuDeviceGetAttribute: ");

    // Use warp size * 4 as fallback for unknown architectures.
    int numCoresPerMultiprocessor = warpSize * 4;

    if (major == 2) {
        if (minor == 1) {
            numCoresPerMultiprocessor = 48;
        } else {
            numCoresPerMultiprocessor = 32;
        }
    } else if (major == 3) {
        numCoresPerMultiprocessor = 192;
    } else if (major == 5) {
        numCoresPerMultiprocessor = 128;
    } else if (major == 6) {
        if (minor == 0) {
            numCoresPerMultiprocessor = 64;
        } else {
            numCoresPerMultiprocessor = 128;
        }
    } else if (major == 7) {
        numCoresPerMultiprocessor = 64;
    } else if (major == 8) {
        if (minor == 0) {
            numCoresPerMultiprocessor = 64;
        } else {
            numCoresPerMultiprocessor = 128;
        }
    } else if (major == 9) {
        numCoresPerMultiprocessor = 128;
    }
    info.numCoresPerMultiprocessor = uint32_t(numCoresPerMultiprocessor);
    info.numCoresTotal = info.numMultiprocessors * info.numCoresPerMultiprocessor;
    info.numCudaCoresEquivalent = info.numCoresTotal;
    info.optimalWorkgroupSizePT = numCoresPerMultiprocessor;
    info.optimalNumWorkgroupsPT = numMultiprocessors;
}
DeviceThreadInfo getCudaDeviceThreadInfo(CUdevice cuDevice) {
    DeviceThreadInfo info{};
    getCudaDeviceThreadInfo(cuDevice, info);
    return info;
}
#endif

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
void getLevelZeroDeviceThreadInfo(ze_device_handle_t zeDevice, DeviceThreadInfo& info) {
    ze_device_properties_t zeDeviceProperties{};
    zeDeviceProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    checkZeResult(g_levelZeroFunctionTable.zeDeviceGetProperties(
            zeDevice, &zeDeviceProperties), "Error in zeDeviceGetProperties: ");
    /*
     * Different values (https://www.intel.com/content/www/us/en/docs/oneapi/optimization-guide-gpu/2024-2/intel-xe-gpu-architecture.html):
     * - numSlices: Number of slices on the GPU
     * - numSubslicesPerSlice: Number of subslices per slice
     * - numEUsPerSubslice: Number of EUs per subslice
     * - physicalEUSimdWidth: SIMD width of an EU
     * - numThreadsPerEU: How many threads (aka. warps on CUDA) can simultaneously be scheduled.
     * Example:
     * - numThreadsPerEU: 10
     * - physicalEUSimdWidth: 8
     * - numSubslicesPerSlice: 4
     * - numSlices: 1
     */
    uint32_t numEUs = zeDeviceProperties.numSlices * zeDeviceProperties.numSubslicesPerSlice * zeDeviceProperties.numEUsPerSubslice;

    // Not overwriting warp size. Theoretically, physicalEUSimdWidth should be the minimum supported.
    //info.warpSize = zeDeviceProperties.physicalEUSimdWidth;
    info.optimalNumWorkgroupsPT = numEUs;
    /*
     * Should be between physicalEUSimdWidth and physicalEUSimdWidth * numThreadsPerEU.
     * info.warpSize (set to subgroupSize, which is usually 32 on Intel hardware) might be a good future-proof value.
     */
    info.optimalWorkgroupSizePT = info.warpSize;

    info.numMultiprocessors = numEUs;
    info.numCoresPerMultiprocessor = zeDeviceProperties.physicalEUSimdWidth;
    info.numCoresTotal = info.numMultiprocessors * info.numCoresPerMultiprocessor;

    // Not sure if we want to map SIMD width 1:1 to CUDA cores...
    info.numCudaCoresEquivalent = info.numCoresTotal;
}
DeviceThreadInfo getLevelZeroDeviceThreadInfo(ze_device_handle_t zeDevice) {
    DeviceThreadInfo info{};
    getLevelZeroDeviceThreadInfo(zeDevice, info);
    return info;
}
#endif

}
