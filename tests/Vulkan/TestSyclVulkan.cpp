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
#include <sycl/sycl.hpp>

#include <Utils/File/Logfile.hpp>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/InteropCompute.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/CommandBuffer.hpp>

#include "ImageFormatsVulkan.hpp"

class InteropTestSyclVk : public ::testing::Test {
protected:
    explicit InteropTestSyclVk(bool useInOrderQueue) : useInOrderQueue(useInOrderQueue) {}
    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileSyclVulkan.html", "TestSyclVulkan");

        sgl::resetComputeApiState();

        // We need immediate command lists for the Level Zero backend to support external semaphores.
        // Should hopefully be ignored by other backends.
        sycl::property_list syclQueueProperties{};
        if (useInOrderQueue) {
            syclQueueProperties = sycl::property_list{sycl::property::queue::in_order(), sycl::ext::intel::property::queue::immediate_command_list()};
        } else {
            syclQueueProperties = sycl::property_list{sycl::ext::intel::property::queue::immediate_command_list()};
        }
        syclQueue = new sycl::queue{sycl::gpu_selector_v, syclQueueProperties};
        std::cout << "Running on " << syclQueue->get_device().get_info<sycl::info::device::name>() << std::endl;

        instance = new sgl::vk::Instance;
        instance->createInstance({}, false);
        device = new sgl::vk::Device;

        uint8_t* deviceUuid = nullptr;
        sycl::device syclDevice = syclQueue->get_device();
        sycl::detail::uuid_type uuid = syclDevice.get_info<sycl::ext::intel::info::device::uuid>();
        deviceUuid = new uint8_t[VK_UUID_SIZE];
        memcpy(deviceUuid, uuid.data(), VK_UUID_SIZE);
        sgl::setGlobalSyclQueue(*syclQueue);
        sgl::setOpenMessageBoxOnComputeApiError(false);
        auto physicalDeviceCheckCallback = [&](
                VkPhysicalDevice physicalDevice,
                VkPhysicalDeviceProperties physicalDeviceProperties,
                std::vector<const char*>& requiredDeviceExtensions,
                std::vector<const char*>& optionalDeviceExtensions,
                sgl::vk::DeviceFeatures& requestedDeviceFeatures) {
            if (physicalDeviceProperties.apiVersion < VK_API_VERSION_1_1) {
                return false;
            }

            VkPhysicalDeviceIDProperties physicalDeviceIdProperties{};
            physicalDeviceIdProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
            VkPhysicalDeviceProperties2 deviceProperties2 = {};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            deviceProperties2.pNext = &physicalDeviceIdProperties;
            sgl::vk::getPhysicalDeviceProperties2(physicalDevice, deviceProperties2);

            if (deviceUuid) {
                bool isSameUuid = true;
                for (int i = 0; i < int(VK_UUID_SIZE); i++) {
                    if (physicalDeviceIdProperties.deviceUUID[i] != deviceUuid[i]) {
                        isSameUuid = false;
                        break;
                    }
                }
                if (!isSameUuid) {
                    return false;
                }
            }

            return true;
        };
        device->setPhysicalDeviceCheckCallback(physicalDeviceCheckCallback);

        std::vector<const char*> optionalDeviceExtensions = sgl::vk::Device::getCudaInteropDeviceExtensions();
        std::vector<const char*> requiredDeviceExtensions = { VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME };
        sgl::vk::DeviceFeatures requestedDeviceFeatures{};
        device->createDeviceHeadless(
                instance, requiredDeviceExtensions, optionalDeviceExtensions, requestedDeviceFeatures);
    }

    void TearDown() override {
        if (device) {
            device->waitIdle();
        }
        delete device;
        delete instance;
        delete syclQueue;
    }

    void runTestsBufferCopySemaphore(bool testRaceCondition);
    void runTestsImageCopy();

    sgl::vk::Instance* instance = nullptr;
    sgl::vk::Device* device = nullptr;
    bool useInOrderQueue = true;
    sycl::queue* syclQueue = nullptr;
};

class InteropTestSyclVkInOrder : public InteropTestSyclVk {
protected:
    InteropTestSyclVkInOrder() : InteropTestSyclVk(true) {}
};

class InteropTestSyclVkOutOfOrder : public InteropTestSyclVk {
protected:
    InteropTestSyclVkOutOfOrder() : InteropTestSyclVk(false) {}
};

TEST_F(InteropTestSyclVkInOrder, BufferSharingOnlyTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_FAIL() << "External memory import not supported.";
    }
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
    sgl::vk::BufferVkComputeApiExternalMemoryPtr bufferSycl;
    try {
        bufferSycl = sgl::vk::createBufferVkComputeApiExternalMemory(bufferVulkan);
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }
    auto* devicePtr = bufferSycl->getDevicePtr<float>();
    auto* hostPtr = sycl::malloc_host<float>(1, *syclQueue);

    // Copy and wait on CPU.
    auto cpyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeof(float));
    cpyEvent.wait_and_throw();

    // Check data.
    if (*hostPtr != 42) {
        FAIL() << "Race condition occurred.";
    }
    sycl::free(hostPtr, *syclQueue);
    bufferSycl = {};
    bufferVulkan = {};
}

class InteropTestSyclVkImageCreation
        : public InteropTestSyclVkInOrder, public testing::WithParamInterface<std::pair<VkFormat, bool>> {
public:
    InteropTestSyclVkImageCreation() = default;
};
struct PrintToStringFormatConfig {
    std::string operator()(const testing::TestParamInfo<std::pair<VkFormat, bool>>& info) const {
        return sgl::vk::convertVkFormatToString(info.param.first);
    }
};

TEST_P(InteropTestSyclVkImageCreation, Formats) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "External bindless images import not supported.";
    }
    VkFormat format = GetParam().first;
    bool isFormatRequired = GetParam().second;

    sgl::vk::ImageSettings imageSettings{};
    imageSettings.width = 1024;
    imageSettings.height = 1024;
    imageSettings.format = format;
    imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageSettings.exportMemory = true;
    imageSettings.useDedicatedAllocationForExportedMemory = true;
    auto imageViewVulkan = std::make_shared<sgl::vk::ImageView>(
            std::make_shared<sgl::vk::Image>(device, imageSettings));
    sgl::vk::ImageVkComputeApiExternalMemoryPtr imageSycl;
    std::string errorMessage;
    try {
        imageSycl = sgl::vk::createImageVkComputeApiExternalMemory(imageViewVulkan->getImage());
    } catch (sycl::exception const& e) {
        errorMessage = e.what();
    } catch (sgl::UnsupportedComputeApiFeatureException const& e) {
        errorMessage = e.what();
    }
    if (!imageSycl) {
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
INSTANTIATE_TEST_SUITE_P(TestFormats, InteropTestSyclVkImageCreation, testedImageFormats, PrintToStringFormatConfig());

TEST_F(InteropTestSyclVkInOrder, BinarySemaphoreAllocationTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    try {
        auto semaphoreBinaryVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(
                device, 0, VK_SEMAPHORE_TYPE_BINARY);
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }
}

TEST_F(InteropTestSyclVkInOrder, TimelineSemaphoreAllocationTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    try {
        auto semaphoreTimelineVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(
                device, 0, VK_SEMAPHORE_TYPE_TIMELINE, 0);
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }
}

void InteropTestSyclVk::runTestsBufferCopySemaphore(bool testRaceCondition) {
    // Create semaphore.
    uint64_t timelineValue = 0;
    sgl::vk::SemaphoreVkComputeApiInteropPtr semaphoreVulkan;
    try {
        semaphoreVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(
                device, 0, VK_SEMAPHORE_TYPE_TIMELINE, timelineValue);
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }

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
    sgl::vk::BufferVkComputeApiExternalMemoryPtr bufferSycl;
    bufferSycl = sgl::vk::createBufferVkComputeApiExternalMemory(bufferVulkan);
    auto* devicePtr = bufferSycl->getDevicePtr<float>();
    auto* hostPtr = sycl::malloc_host<float>(1, *syclQueue);

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

    // Copy with SYCL and wait on CPU.
    sgl::StreamWrapper stream{};
    stream.syclQueuePtr = syclQueue;
    sycl::event waitSemaphoreEvent{};
    semaphoreVulkan->waitSemaphoreComputeApi(stream, timelineValue, &waitSemaphoreEvent);
    auto cpyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeof(float), waitSemaphoreEvent);
    cpyEvent.wait_and_throw();

    // Other direction:
    //syclQueue->ext_oneapi_submit_barrier();

    // Test whether a race condition occurred.
    float dataFinal = *hostPtr;
    if (testRaceCondition && dataFinal != 11) {
        FAIL() << "Race condition occurred.";
    }
    sycl::free(hostPtr, *syclQueue);
    device->waitIdle(); // Should not be necessary.
    delete renderer;
    semaphoreVulkan = {};
    bufferSycl = {};
    bufferVulkan = {};
}

const int NUM_BUFFER_COPY_RUNS = 100;

TEST_F(InteropTestSyclVkInOrder, BufferCopySemaphoreTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    for (int i = 0; i < NUM_BUFFER_COPY_RUNS; i++) {
        runTestsBufferCopySemaphore(true);
    }
}

TEST_F(InteropTestSyclVkInOrder, BufferCopySemaphoreNoRaceConditionCheckTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    for (int i = 0; i < NUM_BUFFER_COPY_RUNS; i++) {
        runTestsBufferCopySemaphore(false);
    }
}

TEST_F(InteropTestSyclVkOutOfOrder, BufferCopySemaphoreTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    for (int i = 0; i < NUM_BUFFER_COPY_RUNS; i++) {
        runTestsBufferCopySemaphore(true);
    }
}

TEST_F(InteropTestSyclVkOutOfOrder, BufferCopySemaphoreNoRaceConditionCheckTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    for (int i = 0; i < NUM_BUFFER_COPY_RUNS; i++) {
        runTestsBufferCopySemaphore(false);
    }
}


void InteropTestSyclVk::runTestsImageCopy() {
    // Image.
    sgl::vk::ImageSettings imageSettings{};
    imageSettings.width = 1024;
    imageSettings.height = 1024;
    imageSettings.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageSettings.usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageSettings.exportMemory = true;
    imageSettings.useDedicatedAllocationForExportedMemory = true;
    auto imageViewVulkan = std::make_shared<sgl::vk::ImageView>(
            std::make_shared<sgl::vk::Image>(device, imageSettings));
    sgl::vk::ImageVkComputeApiExternalMemoryPtr imageSycl;
    try {
        imageSycl = sgl::vk::createImageVkComputeApiExternalMemory(imageViewVulkan->getImage());
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }

    // Upload data to image.
    size_t numEntries = imageSettings.width * imageSettings.height * 4;
    size_t sizeInBytes = numEntries * sizeof(float);
    auto* hostPtr = sycl::malloc_host<float>(numEntries, *syclQueue);
    for (size_t i = 0; i < numEntries; i++) {
        hostPtr[i] = float(i);
    }
    imageViewVulkan->getImage()->uploadData(numEntries * sizeof(float), hostPtr);

    // Copy and wait on CPU.
    memset(hostPtr, 0, sizeInBytes);
    auto* devicePtr = sycl::malloc_device<float>(numEntries, *syclQueue);
    sgl::StreamWrapper stream{};
    stream.syclQueuePtr = syclQueue;
    sycl::event copyEventImg{};
    imageSycl->copyToDevicePtrAsync(devicePtr, stream, &copyEventImg);
    auto copyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeInBytes, copyEventImg);
    copyEvent.wait_and_throw();

    // Check equality.
    for (size_t i = 0; i < numEntries; i++) {
        if (hostPtr[i] != float(i)) {
            size_t channelIdx = i % 4;
            size_t x = (i / 4) % imageSettings.width;
            size_t y = (i / 4) / imageSettings.width;
            std::string errorMessage =
                    "Image content mismatch at x=" + std::to_string(x) + ", y=" + std::to_string(y)
                    + ", c=" + std::to_string(channelIdx);
            ASSERT_TRUE(false) << errorMessage;
        }
    }

    // Free data.
    sycl::free(hostPtr, *syclQueue);
    sycl::free(devicePtr, *syclQueue);
    imageSycl = {};
    imageViewVulkan = {};
}

TEST_F(InteropTestSyclVkInOrder, ImageCopyTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "External semaphore import not supported.";
    }
    for (int i = 0; i < 10; i++) {
        runTestsImageCopy();
    }
}
