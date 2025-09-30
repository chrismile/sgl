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

#include <gtest/gtest.h>
#include <sycl/sycl.hpp>

#include <Utils/File/Logfile.hpp>

#include <Graphics/D3D12/Utils/DXGIFactory.hpp>
#include <Graphics/D3D12/Utils/Device.hpp>
#include <Graphics/D3D12/Utils/Resource.hpp>
#include <Graphics/D3D12/Utils/InteropCompute.hpp>
#include <Graphics/D3D12/Shader/ShaderManager.hpp>
#include <Graphics/D3D12/Render/CommandList.hpp>
#include <Graphics/D3D12/Render/Renderer.hpp>

#include "../SYCL/SyclDeviceCode.hpp"

class InteropTestSyclD3D12 : public ::testing::Test {
protected:
    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileSyclD3D12.html", "TestSyclD3D12");

        sgl::resetComputeApiState();
        sycl::property_list syclQueueProperties = sycl::property_list{sycl::property::queue::in_order(), sycl::ext::intel::property::queue::immediate_command_list()};
        syclQueue = new sycl::queue{sycl::gpu_selector_v, syclQueueProperties};
        std::cout << "Running on " << syclQueue->get_device().get_info<sycl::info::device::name>() << std::endl;
        sgl::setGlobalSyclQueue(*syclQueue);
        sgl::setOpenMessageBoxOnComputeApiError(false);

        uint64_t syclLuid;
        auto syclDevice = syclQueue->get_device();
        sgl::initializeComputeApi(sgl::getSyclDeviceComputeApi(syclDevice));
        if (!sgl::getSyclDeviceLuid(syclDevice, syclLuid)) {
            GTEST_FAIL() << "SYCL device LUID could not be retrieved.";
        }

        dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
        d3d12Device = dxgiFactory->createMatchingDevice(syclLuid, D3D_FEATURE_LEVEL_12_0);
        if (!d3d12Device) {
            GTEST_FAIL() << "No suitable D3D12 device found.";
        }
    }

    void TearDown() override {
        sgl::freeAllComputeApis();
        d3d12Device = {};
        dxgiFactory = {};
        delete syclQueue;
    }

    sgl::d3d12::DXGIFactoryPtr dxgiFactory;
    sgl::d3d12::DevicePtr d3d12Device = nullptr;
    sycl::queue* syclQueue = nullptr;
};

TEST_F(InteropTestSyclD3D12, BufferCopySemaphoreTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "ext_oneapi_external_semaphore_import not supported.";
    }

    auto* renderer = new sgl::d3d12::Renderer(d3d12Device.get());

    const int NUM_ITERATIONS = 100;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        sgl::d3d12::CommandListPtr commandList = std::make_shared<sgl::d3d12::CommandList>(
                d3d12Device.get(), sgl::d3d12::CommandListType::DIRECT);
        uint64_t timelineValue = 0;
        sgl::d3d12::FenceD3D12ComputeApiInteropPtr fence =
                sgl::d3d12::createFenceD3D12ComputeApiInterop(d3d12Device.get(), timelineValue);

        float sharedData = 42.0f;
        sgl::d3d12::ResourceSettings bufferSettings{};
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float), flags);
        bufferSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
        sgl::d3d12::ResourcePtr bufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);
        bufferD3D12->uploadDataLinear(sizeof(float), &sharedData);

        sgl::d3d12::ResourceSettings bufferSettingsIntermediate{};
        bufferSettingsIntermediate.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float), D3D12_RESOURCE_FLAG_NONE);
        bufferSettingsIntermediate.heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        bufferSettingsIntermediate.resourceStates = D3D12_RESOURCE_STATE_COPY_SOURCE;
        sgl::d3d12::ResourcePtr bufferIntermediate = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettingsIntermediate);

        auto bufferSycl = sgl::d3d12::createBufferD3D12ComputeApiExternalMemory(bufferD3D12);
        auto* devicePtr = bufferSycl->getDevicePtr<float>();
        auto* hostPtr = sycl::malloc_host<float>(1, *syclQueue);

        // Upload new data with D3D12.
        ID3D12CommandQueue* d3d12CommandQueue = d3d12Device->getD3D12CommandQueue(commandList->getCommandListType());
        renderer->setCommandList(commandList);
        float newData = 11.0f;
        bufferD3D12->uploadDataLinear(sizeof(float), &newData, bufferIntermediate, commandList);
        commandList->close();
        auto* d3d12CommandList = commandList->getD3D12CommandListPtr();
        d3d12CommandQueue->ExecuteCommandLists(1, &d3d12CommandList);
        timelineValue++;
        d3d12CommandQueue->Signal(fence->getD3D12Fence(), timelineValue);

        // Copy with SYCL and wait on CPU.
        sgl::StreamWrapper stream{};
        stream.syclQueuePtr = syclQueue;
        sycl::event waitSemaphoreEvent{};
        fence->waitFenceComputeApi(stream, timelineValue, &waitSemaphoreEvent);
        auto cpyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeof(float), waitSemaphoreEvent);
        cpyEvent.wait_and_throw();
        fence->waitOnCpu(timelineValue);

        // Test whether a race condition occurred.
        if (*hostPtr != 11) {
            delete renderer;
            FAIL() << "Race condition occurred.";
        }
        sycl::free(hostPtr, *syclQueue);
    }

    delete renderer;
}

TEST_F(InteropTestSyclD3D12, BufferCopyTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }

    uint32_t width = 1024;
    uint32_t height = 1024;
    size_t numEntries = width * height;
    size_t sizeInBytes = sizeof(float) * numEntries;

    sgl::d3d12::ResourceSettings bufferSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, flags);
    bufferSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
    sgl::d3d12::ResourcePtr bufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);

    auto bufferSycl = sgl::d3d12::createBufferD3D12ComputeApiExternalMemory(bufferD3D12);
    auto* hostPtr = sycl::malloc_host<float>(numEntries, *syclQueue);
    for (size_t i = 0; i < numEntries; i++) {
        hostPtr[i] = float(i);
    }
    bufferD3D12->uploadDataLinear(sizeInBytes, hostPtr);

    // Copy and wait on CPU.
    memset(hostPtr, 0, sizeInBytes);
    sgl::StreamWrapper stream{};
    stream.syclQueuePtr = syclQueue;
    sycl::event copyEvent{};
    bufferSycl->copyToHostPtrAsync(hostPtr, stream, &copyEvent);
    copyEvent.wait_and_throw();

    // Check equality.
    for (size_t i = 0; i < numEntries; i++) {
        if (hostPtr[i] != float(i)) {
            size_t x = i % width;
            size_t y = i / width;
            std::string errorMessage =
                    "Buffer content mismatch at x=" + std::to_string(x) + ", y=" + std::to_string(y);
            ASSERT_TRUE(false) << errorMessage;
        }
    }

    // Free data.
    sycl::free(hostPtr, *syclQueue);
}

TEST_F(InteropTestSyclD3D12, BufferSyclWriteTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }

    uint32_t width = 1024;
    uint32_t height = 1024;
    size_t numEntries = width * height;
    size_t sizeInBytes = sizeof(float) * numEntries;

    sgl::d3d12::ResourceSettings bufferSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, flags);
    bufferSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
    sgl::d3d12::ResourcePtr bufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);

    auto bufferSycl = sgl::d3d12::createBufferD3D12ComputeApiExternalMemory(bufferD3D12);
    auto* devicePtr = bufferSycl->getDevicePtr<float>();
    auto* hostPtr = sycl::malloc_host<float>(numEntries, *syclQueue);
    memset(hostPtr, 0, sizeInBytes);
    bufferD3D12->uploadDataLinear(sizeInBytes, hostPtr);

    // Copy and wait on CPU.
    sgl::StreamWrapper stream{};
    stream.syclQueuePtr = syclQueue;
    auto kernelWriteEvent = writeSyclBufferData(*syclQueue, numEntries, devicePtr);
    auto copyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeInBytes, kernelWriteEvent);
    copyEvent.wait_and_throw();

    // Check equality.
    for (size_t i = 0; i < numEntries; i++) {
        if (hostPtr[i] != float(i)) {
            size_t x = i % width;
            size_t y = i / width;
            std::string errorMessage =
                    "Buffer content mismatch at x=" + std::to_string(x) + ", y=" + std::to_string(y);
            ASSERT_TRUE(false) << errorMessage;
        }
    }

    // Free data.
    sycl::free(hostPtr, *syclQueue);
}

TEST_F(InteropTestSyclD3D12, ImageCopyTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "ext_oneapi_bindless_images not supported.";
    }

    uint32_t width = 1024;
    uint32_t height = 1024;

    sgl::d3d12::ResourceSettings imageSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
    imageSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
    sgl::d3d12::ResourcePtr imageD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
    sgl::d3d12::ImageD3D12ComputeApiExternalMemoryPtr imageSycl;
    try {
        imageSycl = sgl::d3d12::createImageD3D12ComputeApiExternalMemory(imageD3D12);
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }

    // Upload data to image.
    size_t numEntries = width * height * 4;
    size_t sizeInBytes = numEntries * sizeof(float);
    auto* hostPtr = sycl::malloc_host<float>(numEntries, *syclQueue);
    for (size_t i = 0; i < numEntries; i++) {
        hostPtr[i] = float(i);
    }
    imageD3D12->uploadDataLinear(sizeof(float) * numEntries, hostPtr);

    size_t imgRowPitch = imageD3D12->getRowPitchInBytes();
    size_t imgSizeInBytes = imageD3D12->getCopiableSizeInBytes();
    if (imgRowPitch != width * 4 * sizeof(float) || imgSizeInBytes != sizeInBytes) {
        FAIL() << "Expected row pitch equal to row size.";
    }

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
            size_t x = (i / 4) % width;
            size_t y = (i / 4) / width;
            std::string errorMessage =
                    "Image content mismatch at x=" + std::to_string(x) + ", y=" + std::to_string(y)
                    + ", c=" + std::to_string(channelIdx);
            ASSERT_TRUE(false) << errorMessage;
        }
    }

    // Free data.
    sycl::free(hostPtr, *syclQueue);
    sycl::free(devicePtr, *syclQueue);
}
