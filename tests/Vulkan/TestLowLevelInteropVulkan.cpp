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

#include <iostream>
#include <utility>
#include <gtest/gtest.h>

#include <Utils/File/Logfile.hpp>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/InteropCompute.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/CommandBuffer.hpp>

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
#include <Graphics/Vulkan/Utils/InteropLevelZero.hpp>
#endif
#ifdef SUPPORT_CUDA_INTEROP
#include <Graphics/Vulkan/Utils/InteropCuda.hpp>
#endif
#ifdef SUPPORT_HIP_INTEROP
#include <Graphics/Vulkan/Utils/InteropHIP.hpp>
#endif

#include "ImageFormatsVulkan.hpp"

/*
 * Set of tests using either Level Zero, CUDA or HIP (depending on the GPU).
 */

class InteropTestLowLevelVk : public ::testing::Test {
protected:
    explicit InteropTestLowLevelVk() = default;

    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileLowLevelInteropVulkan.html", "TestLowLevelInteropVulkan");

        sgl::resetComputeApiState();
        sgl::setOpenMessageBoxOnComputeApiError(false);
        instance = new sgl::vk::Instance;
        instance->createInstance({}, false);

        // Prefer testing the dGPU if we have multiple GPUs available.
        bool isDiscreteGpuAvailable = false;
        std::vector<VkPhysicalDevice> physicalDevicesAvailable = sgl::vk::enumeratePhysicalDevices(instance);
        for (VkPhysicalDevice physicalDevice : physicalDevicesAvailable) {
            VkPhysicalDeviceProperties physicalDeviceProperties{};
            sgl::vk::getPhysicalDeviceProperties(physicalDevice, physicalDeviceProperties);
            if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                isDiscreteGpuAvailable = true;
            }
        }

        device = new sgl::vk::Device;
        auto physicalDeviceCheckCallback = [&](
                VkPhysicalDevice physicalDevice,
                VkPhysicalDeviceProperties physicalDeviceProperties,
                std::vector<const char*>& requiredDeviceExtensions,
                std::vector<const char*>& optionalDeviceExtensions,
                sgl::vk::DeviceFeatures& requestedDeviceFeatures) {
            if (physicalDeviceProperties.apiVersion < VK_API_VERSION_1_1) {
                return false;
            }
            if (isDiscreteGpuAvailable && physicalDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                return false;
            }
            return true;
        };
        device->setPhysicalDeviceCheckCallback(physicalDeviceCheckCallback);

        std::vector<const char*> optionalDeviceExtensions = sgl::vk::Device::getCudaInteropDeviceExtensions();
        std::vector<const char*> requiredDeviceExtensions = { VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME };
        sgl::vk::DeviceFeatures requestedDeviceFeatures{};
        device->createDeviceHeadless(
                instance, requiredDeviceExtensions, optionalDeviceExtensions, requestedDeviceFeatures);
        std::cout << "Running on " << device->getDeviceName() << std::endl;

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        if (device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS
                || device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA) {
            levelZeroInteropInitialized = true;
            if (!sgl::initializeLevelZeroFunctionTable()) {
                levelZeroInteropInitialized = false;
                sgl::Logfile::get()->writeError(
                        "Error in main: sgl::initializeLevelZeroFunctionTable() returned false.",
                        false);
            }

            if (levelZeroInteropInitialized) {
                if (!sgl::vk::initializeLevelZeroAndFindMatchingDevice(device, &zeDriver, &zeDevice)) {
                    levelZeroInteropInitialized = false;
                }
            }

            if (levelZeroInteropInitialized) {
                ze_context_desc_t zeContextDesc{};
                zeContextDesc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
                ze_result_t zeResult = sgl::g_levelZeroFunctionTable.zeContextCreateEx(
                        zeDriver, &zeContextDesc, 1, &zeDevice, &zeContext);
                sgl::checkZeResult(zeResult, "Error in zeContextCreateEx: ");
                sgl::setLevelZeroGlobalState(zeDevice, zeContext);

                // Level Zero only supports immediate command lists for external semaphores.
                /*ze_command_queue_desc_t zeCommandQueueDesc{};
                zeCommandQueueDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
                zeCommandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY;
                zeCommandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS; // ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS?
                zeCommandQueueDesc.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
                zeResult = sgl::g_levelZeroFunctionTable.zeCommandQueueCreate(
                        zeContext, zeDevice, &zeCommandQueueDesc, &zeCommandQueue);
                sgl::checkZeResult(zeResult, "Error in zeCommandQueueCreate: ");
                sgl::setLevelZeroGlobalCommandQueue(zeCommandQueue);

                ze_command_list_desc_t zeCommandListDesc{};
                zeCommandListDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
                zeCommandListDesc.flags =
                        ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING | ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY; // TODO: Other flags?
                // ZE_COMMAND_LIST_FLAG_IN_ORDER?
                zeResult = sgl::g_levelZeroFunctionTable.zeCommandListCreate(
                        zeContext, zeDevice, &zeCommandListDesc, &zeCommandList);
                sgl::checkZeResult(zeResult, "Error in zeCommandListCreate: ");*/

                ze_command_queue_desc_t zeCommandQueueDesc{};
                zeCommandQueueDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
                zeCommandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY | ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
                zeCommandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
                zeCommandQueueDesc.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
                zeResult = sgl::g_levelZeroFunctionTable.zeCommandListCreateImmediate(
                        zeContext, zeDevice, &zeCommandQueueDesc, &zeCommandList);
                sgl::checkZeResult(zeResult, "Error in zeCommandListCreateImmediate: ");

                computeApi = sgl::InteropComputeApi::LEVEL_ZERO;
                streamWrapper.zeCommandList = zeCommandList;
            }
        }
#endif

#ifdef SUPPORT_CUDA_INTEROP
        if (device->getDeviceDriverId() == VK_DRIVER_ID_NVIDIA_PROPRIETARY) {
            cudaInteropInitialized = true;
            if (!sgl::initializeCudaDeviceApiFunctionTable()) {
                cudaInteropInitialized = false;
                sgl::Logfile::get()->writeError(
                        "Error in main: sgl::initializeCudaDeviceApiFunctionTable() returned false.",
                        false);
            }

            if (cudaInteropInitialized) {
                CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuInit(0);
                if (cuResult == CUDA_ERROR_NO_DEVICE) {
                    sgl::Logfile::get()->writeInfo("No CUDA-capable device was found. Disabling CUDA interop support.");
                    cudaInteropInitialized = false;
                } else {
                    sgl::checkCUresult(cuResult, "Error in cuInit: ");
                }
            }

            if (cudaInteropInitialized) {
                if (!sgl::vk::getMatchingCudaDevice(device, &cuDevice)) {
                    cudaInteropInitialized = false;
                    sgl::Logfile::get()->writeError(
                            "Error in main: sgl::vk::getMatchingCudaDevice could not find a matching device.",
                            false);
                }
            }

            if (cudaInteropInitialized) {
                CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuCtxCreate(
                        &cuContext, CU_CTX_SCHED_SPIN, cuDevice);
                sgl::checkCUresult(cuResult, "Error in cuCtxCreate: ");
                cuResult = sgl::g_cudaDeviceApiFunctionTable.cuStreamCreate(&cuStream, CU_STREAM_DEFAULT);
                sgl::checkCUresult(cuResult, "Error in cuStreamCreate: ");

                computeApi = sgl::InteropComputeApi::CUDA;
                streamWrapper.cuStream = cuStream;
            }
        }
#endif

#ifdef SUPPORT_HIP_INTEROP
        if (device->getDeviceDriverId() == VK_DRIVER_ID_AMD_PROPRIETARY
                || device->getDeviceDriverId() == VK_DRIVER_ID_AMD_OPEN_SOURCE
                || device->getDeviceDriverId() == VK_DRIVER_ID_MESA_RADV) {
            hipInteropInitialized = true;
            if (!sgl::initializeHipDeviceApiFunctionTable()) {
                hipInteropInitialized = false;
                sgl::Logfile::get()->writeError(
                        "Error in main: sgl::initializeHipDeviceApiFunctionTable() returned false.",
                        false);
            }

            if (hipInteropInitialized) {
                hipError_t hipResult = sgl::g_hipDeviceApiFunctionTable.hipInit(0);
                if (hipResult == hipErrorNoDevice) {
                    sgl::Logfile::get()->writeInfo("No HIP-capable device was found. Disabling HIP interop support.");
                    hipInteropInitialized = false;
                } else {
                    sgl::checkHipResult(hipResult, "Error in hipInit: ");
                }
            }

            if (hipInteropInitialized) {
                if (!sgl::vk::getMatchingHipDevice(device, &hipDevice)) {
                    hipInteropInitialized = false;
                    sgl::Logfile::get()->writeError(
                            "Error in main: sgl::vk::getMatchingHipDevice could not find a matching device.",
                            false);
                }
            }

            if (hipInteropInitialized) {
                hipError_t hipResult = sgl::g_hipDeviceApiFunctionTable.hipCtxCreate(
                        &hipContext, hipDeviceScheduleSpin, hipDevice);
                sgl::checkHipResult(hipResult, "Error in hipCtxCreate: ");
                hipResult = sgl::g_hipDeviceApiFunctionTable.hipStreamCreate(&hipStream, hipStreamDefault);
                sgl::checkHipResult(hipResult, "Error in hipCtxCreate: ");

                computeApi = sgl::InteropComputeApi::HIP;
                streamWrapper.hipStream = hipStream;
            }
        }
#endif

        if (computeApi == sgl::InteropComputeApi::NONE) {
            FAIL() << "No compute API could be initialized";
        }
    }

    void TearDown() override {
        if (device) {
            device->waitIdle();
        }
        delete device;
        delete instance;

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        if (sgl::getIsLevelZeroFunctionTableInitialized()) {
            if (zeCommandQueue) {
                ze_result_t zeResult = sgl::g_levelZeroFunctionTable.zeCommandListDestroy(zeCommandList);
                sgl::checkZeResult(zeResult, "Error in zeCommandListDestroy: ");
            }
            if (zeCommandQueue) {
                ze_result_t zeResult = sgl::g_levelZeroFunctionTable.zeCommandQueueDestroy(zeCommandQueue);
                sgl::checkZeResult(zeResult, "Error in zeCommandQueueDestroy: ");
            }
            if (zeContext) {
                ze_result_t zeResult = sgl::g_levelZeroFunctionTable.zeContextDestroy(zeContext);
                sgl::checkZeResult(zeResult, "Error in zeContextDestroy: ");
            }
            sgl::freeLevelZeroFunctionTable();
        }
#endif

#ifdef SUPPORT_CUDA_INTEROP
        if (sgl::getIsCudaDeviceApiFunctionTableInitialized()) {
            if (cuContext) {
                CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuCtxDestroy(cuContext);
                sgl::checkCUresult(cuResult, "Error in cuCtxDestroy: ");
                cuContext = {};
            }
            sgl::freeCudaDeviceApiFunctionTable();
        }
#endif

#ifdef SUPPORT_HIP_INTEROP
        if (sgl::getIsHipDeviceApiFunctionTableInitialized()) {
            if (hipContext) {
                hipError_t hipResult = sgl::g_hipDeviceApiFunctionTable.hipCtxDestroy(hipContext);
                sgl::checkHipResult(hipResult, "Error in hipCtxDestroy: ");
                hipContext = {};
            }
            sgl::freeHipDeviceApiFunctionTable();
        }
#endif
    }

    sgl::StreamWrapper getStreamWrapper() {
        sgl::StreamWrapper stream{};

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
        if (computeApi == sgl::InteropComputeApi::LEVEL_ZERO) {
            stream.zeCommandList = zeCommandList;
        }
#endif
#ifdef SUPPORT_CUDA_INTEROP
        if (computeApi == sgl::InteropComputeApi::CUDA) {
            stream.cuStream = cuStream;
        }
#endif
#ifdef SUPPORT_HIP_INTEROP
        if (computeApi == sgl::InteropComputeApi::HIP) {
            stream.hipStream = hipStream;
        }
#endif

        return stream;
    }

    void checkBindlessImagesSupported(bool& available);
    void checkSemaphoresSupported(bool& available);
    void runTestsBufferCopySemaphore();
    void runTestImageCreation(VkFormat format, bool isFormatRequired);

    sgl::vk::Instance* instance = nullptr;
    sgl::vk::Device* device = nullptr;

    sgl::InteropComputeApi computeApi = sgl::InteropComputeApi::NONE;
    sgl::StreamWrapper streamWrapper{};

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    bool levelZeroInteropInitialized = false;
    ze_driver_handle_t zeDriver{};
    ze_device_handle_t zeDevice{};
    ze_context_handle_t zeContext{};
    ze_command_queue_handle_t zeCommandQueue{};
    ze_command_list_handle_t zeCommandList{};
#endif

#ifdef SUPPORT_CUDA_INTEROP
    bool cudaInteropInitialized = false;
    CUcontext cuContext = {};
    CUdevice cuDevice = 0;
    CUstream cuStream = {};
#endif

#ifdef SUPPORT_HIP_INTEROP
    bool hipInteropInitialized = false;
    hipCtx_t hipContext = {};
    hipDevice_t hipDevice = 0;
    hipStream_t hipStream = {};
#endif
};

TEST_F(InteropTestLowLevelVk, BufferSharingOnlyTest) {
    // Create buffer data.
    float sharedData = 42.0f;
    sgl::vk::BufferSettings bufferSettings{};
    bufferSettings.sizeInBytes = sizeof(float);
    bufferSettings.usage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferSettings.exportMemory = true;
    bufferSettings.useDedicatedAllocationForExportedMemory = true;
    auto bufferVulkan = std::make_shared<sgl::vk::Buffer>(device, bufferSettings);
    bufferVulkan->uploadData(sizeof(float), &sharedData);
    sgl::vk::BufferVkComputeApiExternalMemoryPtr bufferComputeApi =
            sgl::vk::createBufferVkComputeApiExternalMemory(bufferVulkan);
    float hostData = 0.0f;

    // Copy and wait on CPU.
    sgl::StreamWrapper stream = getStreamWrapper();
    bufferComputeApi->copyToHostPtrAsync(&hostData, stream);
    sgl::waitForCompletion(sgl::vk::decideInteropComputeApi(device), stream);

    // Check data.
    if (hostData != 42) {
        FAIL() << "Race condition occurred.";
    }
    bufferComputeApi = {};
    bufferVulkan = {};
}

#define SKIP_UNSUPPORTED_LEVEL_ZERO_TESTS
void InteropTestLowLevelVk::checkBindlessImagesSupported(bool& available) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (sgl::getIsLevelZeroFunctionTableInitialized() && !sgl::queryLevelZeroDriverSupportsBindlessImages(zeDriver)) {
        available = false;
        const char* errorString = "Level Zero driver does not support bindless images.";
#ifdef SKIP_UNSUPPORTED_LEVEL_ZERO_TESTS
        GTEST_SKIP() << errorString; // Should be handled as a warning.
        sgl::Logfile::get()->writeWarning(errorString);
#else
        FAIL() << errorString;
#endif
    }
#endif
}

class InteropTestLowLevelVkRegularImageCreation
        : public InteropTestLowLevelVk, public testing::WithParamInterface<std::pair<VkFormat, bool>> {
public:
    InteropTestLowLevelVkRegularImageCreation() = default;
};
class InteropTestLowLevelVkBindlessImageCreation
        : public InteropTestLowLevelVk, public testing::WithParamInterface<std::pair<VkFormat, bool>> {
public:
    InteropTestLowLevelVkBindlessImageCreation() = default;
};
struct PrintToStringFormatConfig {
    std::string operator()(const testing::TestParamInfo<std::pair<VkFormat, bool>>& info) const {
        return sgl::vk::convertVkFormatToString(info.param.first);
    }
};

void InteropTestLowLevelVk::runTestImageCreation(VkFormat format, bool isFormatRequired) {
    sgl::vk::ImageSettings imageSettings{};
    imageSettings.width = 1024;
    imageSettings.height = 1024;
    imageSettings.format = format;
    imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageSettings.exportMemory = true;
    imageSettings.useDedicatedAllocationForExportedMemory = true;
    auto imageViewVulkan = std::make_shared<sgl::vk::ImageView>(
            std::make_shared<sgl::vk::Image>(device, imageSettings));
    sgl::vk::ImageVkComputeApiExternalMemoryPtr imageComputeApi;
    std::string errorMessage;
    try {
        imageComputeApi = sgl::vk::createImageVkComputeApiExternalMemory(imageViewVulkan->getImage());
    } catch (sgl::UnsupportedComputeApiFeatureException const& e) {
        errorMessage = e.what();
    }
    if (!imageComputeApi) {
        std::string errorString;
        if (isFormatRequired) {
            errorString = "Required";
        } else {
            errorString = "Optional";
        }
        errorString +=
                " format " + sgl::vk::convertVkFormatToString(format) + " not supported. "
                + "Error message: " + errorMessage;
        if (isFormatRequired) {
            FAIL() << errorString;
        } else {
            sgl::Logfile::get()->writeWarning(errorString);
            GTEST_SKIP() << errorString; // Should be handled as a warning.
        }
    }
}

TEST_P(InteropTestLowLevelVkRegularImageCreation, Formats) {
    VkFormat format = GetParam().first;
    bool isFormatRequired = GetParam().second;
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (sgl::getIsLevelZeroFunctionTableInitialized()) {
        sgl::setLevelZeroUseBindlessImagesInterop(false);
    }
#endif
    runTestImageCreation(format, isFormatRequired);
}

TEST_P(InteropTestLowLevelVkBindlessImageCreation, Formats) {
    bool bindlessImagesSupported = true;
    checkBindlessImagesSupported(bindlessImagesSupported);
    if (!bindlessImagesSupported) {
        return;
    }

    VkFormat format = GetParam().first;
    bool isFormatRequired = GetParam().second;
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (sgl::getIsLevelZeroFunctionTableInitialized()) {
        sgl::setLevelZeroUseBindlessImagesInterop(true);
    }
#endif
    runTestImageCreation(format, isFormatRequired);
}

INSTANTIATE_TEST_SUITE_P(TestFormats, InteropTestLowLevelVkRegularImageCreation, testedImageFormats, PrintToStringFormatConfig());
INSTANTIATE_TEST_SUITE_P(TestFormats, InteropTestLowLevelVkBindlessImageCreation, testedImageFormats, PrintToStringFormatConfig());


void InteropTestLowLevelVk::checkSemaphoresSupported(bool& available) {
#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (sgl::getIsLevelZeroFunctionTableInitialized() && !sgl::queryLevelZeroDriverSupportsExternalSemaphores(zeDriver)) {
        available = false;
        const char* errorString = "Level Zero driver does not support external semaphores.";
#ifdef SKIP_UNSUPPORTED_LEVEL_ZERO_TESTS
        GTEST_SKIP() << errorString; // Should be handled as a warning.
        sgl::Logfile::get()->writeWarning(errorString);
#else
        FAIL() << errorString;
#endif
    }
#endif
#ifdef SUPPORT_HIP_INTEROP
    if (sgl::getIsHipDeviceApiFunctionTableInitialized() && !sgl::getHipInteropSupportsSemaphores()) {
        available = false;
        const char* errorString = "HIP does not support external semaphores.";
        FAIL() << errorString;
    }
#endif
}

TEST_F(InteropTestLowLevelVk, BinarySemaphoreAllocationTest) {
    bool semaphoresSupported = true;
    checkSemaphoresSupported(semaphoresSupported);
    if (!semaphoresSupported) {
        return;
    }
    auto semaphoreBinaryVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(device, 0, VK_SEMAPHORE_TYPE_BINARY);
}

TEST_F(InteropTestLowLevelVk, TimelineSemaphoreAllocationTest) {
    bool semaphoresSupported = true;
    checkSemaphoresSupported(semaphoresSupported);
    if (!semaphoresSupported) {
        return;
    }
    auto semaphoreTimelineVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(device, 0, VK_SEMAPHORE_TYPE_TIMELINE, 0);
}

void InteropTestLowLevelVk::runTestsBufferCopySemaphore() {
    bool semaphoresSupported = true;
    checkSemaphoresSupported(semaphoresSupported);
    if (!semaphoresSupported) {
        return;
    }

    // Create semaphore.
    uint64_t timelineValue = 0;
    sgl::vk::SemaphoreVkComputeApiInteropPtr semaphoreVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(
            device, 0, VK_SEMAPHORE_TYPE_TIMELINE, timelineValue);

    // Create buffer data.
    float sharedData = 42.0f;
    sgl::vk::BufferSettings bufferSettings{};
    bufferSettings.sizeInBytes = sizeof(float);
    bufferSettings.usage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferSettings.exportMemory = true;
    bufferSettings.useDedicatedAllocationForExportedMemory = true;
    auto bufferVulkan = std::make_shared<sgl::vk::Buffer>(device, bufferSettings);
    bufferVulkan->uploadData(sizeof(float), &sharedData);
    sgl::vk::BufferVkComputeApiExternalMemoryPtr bufferComputeApi;
    bufferComputeApi = sgl::vk::createBufferVkComputeApiExternalMemory(bufferVulkan);
    auto* devicePtr = bufferComputeApi->getDevicePtr<float>();
    float hostData = 0.0f;

    // Create renderer and command buffer.
    auto renderer = new sgl::vk::Renderer(device);
    sgl::vk::CommandPoolType commandPoolType;
    commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    auto commandBuffer = std::make_shared<sgl::vk::CommandBuffer>(device, commandPoolType);

    // Upload new data with Vulkan.
    renderer->pushCommandBuffer(commandBuffer);
    renderer->beginCommandBuffer();
    float newData = 11.0f;
    bufferVulkan->updateData(0, sizeof(float), &newData, commandBuffer->getVkCommandBuffer());
    timelineValue++;
    semaphoreVulkan->setSignalSemaphoreValue(timelineValue);
    commandBuffer->pushSignalSemaphore(semaphoreVulkan);
    renderer->endCommandBuffer();
    renderer->submitToQueue();

    // Copy with compute API and wait on CPU.

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    ze_event_pool_handle_t zeEventPool{};
    ze_event_handle_t waitSemaphoreEvent{};
    if (sgl::getIsLevelZeroFunctionTableInitialized()) {
        ze_event_pool_desc_t zeEventPoolDesc{};
        zeEventPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
        zeEventPoolDesc.count = 10;
        ze_result_t zeResult = sgl::g_levelZeroFunctionTable.zeEventPoolCreate(
                zeContext, &zeEventPoolDesc, 1, &zeDevice, &zeEventPool);
        sgl::checkZeResult(zeResult, "Error in zeEventPoolCreate: ");

        ze_event_desc_t zeEventDesc{};
        zeEventDesc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
        zeEventDesc.index = 0;
        zeEventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
        zeEventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        zeResult = sgl::g_levelZeroFunctionTable.zeEventCreate(zeEventPool, &zeEventDesc, &waitSemaphoreEvent);
        sgl::checkZeResult(zeResult, "Error in zeEventCreate: ");
    }
#endif

    sgl::StreamWrapper stream = getStreamWrapper();
    sgl::setLevelZeroNextCommandEvents(waitSemaphoreEvent, 0, nullptr);
    semaphoreVulkan->waitSemaphoreComputeApi(stream, timelineValue, &waitSemaphoreEvent);
    sgl::setLevelZeroNextCommandEvents(nullptr, 1, &waitSemaphoreEvent);
    bufferComputeApi->copyToHostPtrAsync(&hostData, stream);
    sgl::waitForCompletion(sgl::vk::decideInteropComputeApi(device), stream);

    // Test whether a race condition occurred.
    if (hostData != 11) {
        FAIL() << "Race condition occurred.";
    }
    device->waitIdle(); // Should not be necessary.
    delete renderer;
    semaphoreVulkan = {};
    bufferComputeApi = {};
    bufferVulkan = {};

#ifdef SUPPORT_LEVEL_ZERO_INTEROP
    if (sgl::getIsLevelZeroFunctionTableInitialized()) {
        ze_result_t zeResult = sgl::g_levelZeroFunctionTable.zeEventPoolDestroy(zeEventPool);
        sgl::checkZeResult(zeResult, "Error in zeEventPoolDestroy: ");
    }
#endif
}

const int NUM_BUFFER_COPY_RUNS = 1000;

TEST_F(InteropTestLowLevelVk, BufferCopySemaphoreTest) {
    for (int i = 0; i < NUM_BUFFER_COPY_RUNS; i++) {
        runTestsBufferCopySemaphore();
    }
}
