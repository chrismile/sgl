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

#include <Math/Math.hpp>
#include <Utils/Format.hpp>
#include <Utils/File/Logfile.hpp>

#include <Graphics/D3D12/Utils/DXGIFactory.hpp>
#include <Graphics/D3D12/Utils/Device.hpp>
#include <Graphics/D3D12/Utils/Resource.hpp>
#include <Graphics/D3D12/Utils/InteropCompute.hpp>
#include <Graphics/D3D12/Utils/InteropCompute/ImplSycl.hpp>
#include <Graphics/D3D12/Shader/Shader.hpp>
#include <Graphics/D3D12/Shader/ShaderManager.hpp>
#include <Graphics/D3D12/Render/Data.hpp>
#include <Graphics/D3D12/Render/Renderer.hpp>
#include <Graphics/D3D12/Render/CommandList.hpp>
#include <Graphics/D3D12/Render/DescriptorAllocator.hpp>

#include "../Utils/Common.hpp"
#include "../SYCL/CommonSycl.hpp"
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
        hostPtr[i] = static_cast<float>(i);
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


const auto testedImageFormatsD3D12 = testing::Values(
        DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32B32A32_UINT
);

struct PrintToStringFormatD3D12Config {
    std::string operator()(const testing::TestParamInfo<DXGI_FORMAT>& info) const {
        return sgl::d3d12::convertDXGIFormatToString(info.param);
    }
};
class InteropTestSyclD3D12Image
        : public InteropTestSyclD3D12, public testing::WithParamInterface<DXGI_FORMAT> {
public:
    InteropTestSyclD3D12Image() = default;
};

TEST_P(InteropTestSyclD3D12Image, ImageCopyTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "ext_oneapi_bindless_images not supported.";
    }

    DXGI_FORMAT format = GetParam();

    uint32_t width = 1024;
    uint32_t height = 1024;
    auto formatInfo = sgl::d3d12::getDXGIFormatInfo(format);
    size_t numEntries = width * height * formatInfo.numChannels;
    size_t sizeInBytes = width * height * formatInfo.formatSizeInBytes;

    sgl::d3d12::ResourceSettings imageSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            format, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
    imageSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
    sgl::d3d12::ResourcePtr imageD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
    sgl::d3d12::ImageD3D12ComputeApiExternalMemoryPtr imageSycl;
    try {
        imageSycl = sgl::d3d12::createImageD3D12ComputeApiExternalMemory(imageD3D12);
    } catch (sycl::exception const& e) {
        FAIL() << e.what();
    }

    // Upload data to image.
    auto* hostPtr = sycl_malloc_host_typed(formatInfo.channelFormat, numEntries, *syclQueue);
    initializeHostPointerLinearTyped(formatInfo.channelFormat, numEntries, hostPtr);
    imageD3D12->uploadDataLinear(sizeInBytes, hostPtr);

    size_t imgRowPitch = imageD3D12->getRowPitchInBytes();
    size_t imgSizeInBytes = imageD3D12->getCopiableSizeInBytes();
    if (imgRowPitch != width * formatInfo.formatSizeInBytes || imgSizeInBytes != sizeInBytes) {
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
    std::string errorMessage;
    if (!checkIsArrayLinearTyped(formatInfo, width, height, hostPtr, errorMessage)) {
        ASSERT_TRUE(false) << errorMessage;
    }

    // Free data.
    sycl::free(hostPtr, *syclQueue);
    sycl::free(devicePtr, *syclQueue);
}

TEST_P(InteropTestSyclD3D12Image, ImageD3D12WriteSyclReadTests) {
#ifndef SUPPORT_D3D_COMPILER
    GTEST_SKIP() << "D3D12 shader compiler is not enabled.";
#endif
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "ext_oneapi_external_semaphore_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "ext_oneapi_bindless_images not supported.";
    }

    auto* shaderManager = new sgl::d3d12::ShaderManagerD3D12();
    auto* renderer = new sgl::d3d12::Renderer(d3d12Device.get());

    DXGI_FORMAT format = GetParam();

    uint32_t width = 1024;
    uint32_t height = 1024;
    auto formatInfo = sgl::d3d12::getDXGIFormatInfo(format);
    size_t numEntries = width * height * formatInfo.numChannels;
    size_t sizeInBytes = width * height * formatInfo.formatSizeInBytes;

    const char* SHADER_STRING_WRITE_IMAGE_COMPUTE_FMT = R"(
    RWTexture2D<$0> destImage : register(u0);
    #define tvec $0
    #define NUM_CHANNELS $1
    [numthreads(16, 16, 1)]
    void CSMain(
            uint3 groupID : SV_GroupID, uint3 dispatchThreadID : SV_DispatchThreadID,
            uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex) {
        uint width, height;
        destImage.GetDimensions(width, height);
        const uint2 idx = dispatchThreadID.xy;
        if (idx.x >= width || idx.y >= height) {
            return;
        }
    #if NUM_CHANNELS == 1
        tvec outputValue = tvec(idx.x + idx.y * width);
    #elif NUM_CHANNELS == 2
        uint value = (idx.x + idx.y * width) * 2;
        tvec outputValue = tvec(value, value + 1);
    #elif NUM_CHANNELS == 4
        uint value = (idx.x + idx.y * width) * 4;
        tvec outputValue = tvec(value, value + 1, value + 2, value + 3);
    #else
    #error Unsupported number of image channels.
    #endif
        destImage[idx] = outputValue;
    }
    )";
    auto shaderStringWriteImageCompute = sgl::formatStringPositional(
            SHADER_STRING_WRITE_IMAGE_COMPUTE_FMT,
            sgl::d3d12::getDXGIFormatHLSLStructuredTypeString(format), formatInfo.numChannels);
    auto computeShader = shaderManager->loadShaderFromHlslString(
            shaderStringWriteImageCompute, "WriteImageShader.hlsl",
            sgl::d3d12::ShaderModuleType::COMPUTE, "CSMain", {});
    auto rootParameters = std::make_shared<sgl::d3d12::RootParameters>(computeShader);
    D3D12_DESCRIPTOR_RANGE1 descriptorRange{};
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRange.NumDescriptors = 1;
    auto rpiDescriptorTable = rootParameters->pushDescriptorTable(1, &descriptorRange);
    sgl::d3d12::DescriptorAllocator* descriptorAllocatorUAV =
            renderer->getDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto descriptorAllocationUAV = descriptorAllocatorUAV->allocate(1);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    const int NUM_ITERATIONS = 1000;
    for (int it = 0; it < NUM_ITERATIONS; it++) {
        sgl::d3d12::CommandListPtr commandList = std::make_shared<sgl::d3d12::CommandList>(
                d3d12Device.get(), sgl::d3d12::CommandListType::DIRECT);
        uint64_t timelineValue = 0;
        sgl::d3d12::FenceD3D12ComputeApiInteropPtr fence =
                sgl::d3d12::createFenceD3D12ComputeApiInterop(d3d12Device.get(), timelineValue);

        sgl::d3d12::ResourceSettings imageSettings{};
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                format, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
        imageSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
        sgl::d3d12::ResourcePtr imageD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
        sgl::d3d12::UnsampledImageD3D12ComputeApiExternalMemoryPtr imageInterop;
        try {
            imageInterop = sgl::d3d12::createUnsampledImageD3D12ComputeApiExternalMemory(imageD3D12);
        } catch (sycl::exception const& e) {
            FAIL() << e.what();
        }
        auto imageInteropSycl = std::static_pointer_cast<sgl::d3d12::UnsampledImageD3D12SyclInterop>(imageInterop);

        d3d12Device->getD3D12Device2()->CreateUnorderedAccessView(
                imageD3D12->getD3D12ResourcePtr(), nullptr, &uavDesc,
                descriptorAllocationUAV->getCPUDescriptorHandle());

        auto computeData = std::make_shared<sgl::d3d12::ComputeData>(d3d12Device.get(), rootParameters);
        computeData->setDescriptorTable(rpiDescriptorTable, descriptorAllocationUAV.get());

        // Upload data to image.
        auto* hostPtr = sycl_malloc_host_typed(formatInfo.channelFormat, numEntries, *syclQueue);
        auto* devicePtr = sycl_malloc_device_typed(formatInfo.channelFormat, numEntries, *syclQueue);
        initializeHostPointerTyped(formatInfo.channelFormat, numEntries, 42, hostPtr);
        imageD3D12->uploadDataLinear(sizeInBytes, hostPtr);

        // Write new data with D3D12.
        ID3D12CommandQueue* d3d12CommandQueue = d3d12Device->getD3D12CommandQueue(commandList->getCommandListType());
        renderer->setCommandList(commandList);
        auto* descriptorHeap = descriptorAllocatorUAV->getD3D12DescriptorHeapPtr();
        commandList->getD3D12GraphicsCommandListPtr()->SetDescriptorHeaps(1, &descriptorHeap);
        renderer->dispatch(computeData, sgl::uiceil(width, 16u), sgl::uiceil(height, 16u), 1);
        commandList->close();
        auto* d3d12CommandList = commandList->getD3D12CommandListPtr();
        d3d12CommandQueue->ExecuteCommandLists(1, &d3d12CommandList);
        timelineValue++;
        d3d12CommandQueue->Signal(fence->getD3D12Fence(), timelineValue);

        // Copy and wait on CPU.
        sgl::StreamWrapper stream{};
        stream.syclQueuePtr = syclQueue;
        sycl::event waitSemaphoreEvent{};
        fence->waitFenceComputeApi(stream, timelineValue, &waitSemaphoreEvent);
        syclexp::unsampled_image_handle imageSyclHandle{};
        imageSyclHandle.raw_handle = imageInteropSycl->getRawHandle();
        sycl::event copyEventImg = copySyclBindlessImageToBuffer(
                *syclQueue, imageSyclHandle, formatInfo, width, height, devicePtr, waitSemaphoreEvent);
        auto copyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeInBytes, copyEventImg);
        copyEvent.wait_and_throw();

        // Check equality.
        std::string errorMessage;
        if (!checkIsArrayLinearTyped(formatInfo, width, height, hostPtr, errorMessage)) {
            descriptorAllocationUAV = {};
            delete shaderManager;
            delete renderer;
            ASSERT_TRUE(false) << errorMessage;
        }

        // Free data.
        sycl::free(hostPtr, *syclQueue);
        sycl::free(devicePtr, *syclQueue);
    }

    descriptorAllocationUAV = {};
    delete shaderManager;
    delete renderer;
}

TEST_P(InteropTestSyclD3D12Image, ImageSyclWriteD3D12ReadTests) {
#ifndef SUPPORT_D3D_COMPILER
    GTEST_SKIP() << "D3D12 shader compiler is not enabled.";
#endif
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "ext_oneapi_external_semaphore_import not supported.";
    }
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "ext_oneapi_bindless_images not supported.";
    }

    DXGI_FORMAT format = GetParam();

    uint32_t width = 1024;
    uint32_t height = 1024;
    auto formatInfo = sgl::d3d12::getDXGIFormatInfo(format);
    size_t numEntries = width * height * formatInfo.numChannels;
    size_t sizeInBytes = width * height * formatInfo.formatSizeInBytes;

    auto* shaderManager = new sgl::d3d12::ShaderManagerD3D12();
    auto* renderer = new sgl::d3d12::Renderer(d3d12Device.get());

    const char* SHADER_STRING_COPY_IMAGE_FROM_BUFFER_COMPUTE_FMT = R"(
    RWTexture2D<$0> srcImage : register(u0);
    RWStructuredBuffer<$0> destBuffer : register(u1);
    [numthreads(16, 16, 1)]
    void CSMain(
            uint3 groupID : SV_GroupID, uint3 dispatchThreadID : SV_DispatchThreadID,
            uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex) {
        uint width, height;
        srcImage.GetDimensions(width, height);
        const uint2 idx = dispatchThreadID.xy;
        if (idx.x >= width || idx.y >= height) {
            return;
        }
        destBuffer[idx.x + idx.y * width] = srcImage[idx];
    }
    )";
    auto shaderStringWriteImageCompute = sgl::formatStringPositional(
            SHADER_STRING_COPY_IMAGE_FROM_BUFFER_COMPUTE_FMT,
            sgl::d3d12::getDXGIFormatHLSLStructuredTypeString(format));
    auto computeShader = shaderManager->loadShaderFromHlslString(
            shaderStringWriteImageCompute, "CopyImageToBufferShader.hlsl",
            sgl::d3d12::ShaderModuleType::COMPUTE, "CSMain", {});
    auto rootParameters = std::make_shared<sgl::d3d12::RootParameters>(computeShader);
    D3D12_DESCRIPTOR_RANGE1 descriptorRange{};
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRange.NumDescriptors = 2;
    auto rpiDescriptorTable = rootParameters->pushDescriptorTable(1, &descriptorRange);
    sgl::d3d12::DescriptorAllocator* descriptorAllocator =
            renderer->getDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto descriptorAllocation = descriptorAllocator->allocate(2);
    D3D12_UNORDERED_ACCESS_VIEW_DESC sourceImgUavDesc{};
    sourceImgUavDesc.Format = format;
    sourceImgUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    D3D12_UNORDERED_ACCESS_VIEW_DESC destBufferUavDesc{};
    destBufferUavDesc.Format = DXGI_FORMAT_UNKNOWN;
    destBufferUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    destBufferUavDesc.Buffer.NumElements = UINT(numEntries);
    destBufferUavDesc.Buffer.StructureByteStride = sizeof(float);

    const int NUM_ITERATIONS = 1000;
    for (int it = 0; it < NUM_ITERATIONS; it++) {
        sgl::d3d12::CommandListPtr commandList = std::make_shared<sgl::d3d12::CommandList>(
                d3d12Device.get(), sgl::d3d12::CommandListType::DIRECT);
        uint64_t timelineValue = 0;
        sgl::d3d12::FenceD3D12ComputeApiInteropPtr fence =
                sgl::d3d12::createFenceD3D12ComputeApiInterop(d3d12Device.get(), timelineValue);

        sgl::d3d12::ResourceSettings imageSettings{};
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                format, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
        imageSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
        sgl::d3d12::ResourcePtr imageD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
        sgl::d3d12::UnsampledImageD3D12ComputeApiExternalMemoryPtr imageInterop;
        try {
            imageInterop = sgl::d3d12::createUnsampledImageD3D12ComputeApiExternalMemory(imageD3D12);
        } catch (sycl::exception const& e) {
            FAIL() << e.what();
        }
        auto imageInteropSycl = std::static_pointer_cast<sgl::d3d12::UnsampledImageD3D12SyclInterop>(imageInterop);

        sgl::d3d12::ResourceSettings bufferSettings{};
        bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, flags);
        sgl::d3d12::ResourcePtr bufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);
        /*bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, D3D12_RESOURCE_FLAG_NONE);
        bufferSettings.heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
        bufferSettings.resourceStates = D3D12_RESOURCE_STATE_COPY_DEST;
        sgl::d3d12::ResourcePtr stagingBufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);*/

        auto* hostPtr = sycl_malloc_host_typed(formatInfo.channelFormat, numEntries, *syclQueue);

        d3d12Device->getD3D12Device2()->CreateUnorderedAccessView(
                imageD3D12->getD3D12ResourcePtr(), nullptr, &sourceImgUavDesc,
                descriptorAllocation->getCPUDescriptorHandle(0));
        d3d12Device->getD3D12Device2()->CreateUnorderedAccessView(
                bufferD3D12->getD3D12ResourcePtr(), nullptr, &destBufferUavDesc,
                descriptorAllocation->getCPUDescriptorHandle(1));

        auto computeData = std::make_shared<sgl::d3d12::ComputeData>(d3d12Device.get(), rootParameters);
        computeData->setDescriptorTable(rpiDescriptorTable, descriptorAllocation.get());

        // Write data with SYCL.
        sgl::StreamWrapper stream{};
        stream.syclQueuePtr = syclQueue;
        syclexp::unsampled_image_handle imageSyclHandle{};
        imageSyclHandle.raw_handle = imageInteropSycl->getRawHandle();
        sycl::event writeImgEvent = writeSyclBindlessImageIncreasingIndices(
                *syclQueue, imageSyclHandle, formatInfo, width, height);
        //auto barrierEvent = syclQueue->ext_oneapi_submit_barrier({ writeImgEvent }); // broken
        sycl::event signalSemaphoreEvent{};
        timelineValue++;
        fence->signalFenceComputeApi(stream, timelineValue, &writeImgEvent, &signalSemaphoreEvent);

        // Copy image data to buffer with D3D12.
        ID3D12CommandQueue* d3d12CommandQueue = d3d12Device->getD3D12CommandQueue(commandList->getCommandListType());
        d3d12CommandQueue->Wait(fence->getD3D12Fence(), timelineValue);
        renderer->setCommandList(commandList);
        auto* descriptorHeap = descriptorAllocator->getD3D12DescriptorHeapPtr();
        commandList->getD3D12GraphicsCommandListPtr()->SetDescriptorHeaps(1, &descriptorHeap);
        renderer->dispatch(computeData, sgl::uiceil(width, 16u), sgl::uiceil(height, 16u), 1);
        commandList->close();
        auto* d3d12CommandList = commandList->getD3D12CommandListPtr();
        d3d12CommandQueue->ExecuteCommandLists(1, &d3d12CommandList);
        timelineValue++;
        d3d12CommandQueue->Signal(fence->getD3D12Fence(), timelineValue);

        // Wait on CPU.
        fence->waitOnCpu(timelineValue);

        // Check equality.
        //const auto* hostPtr = static_cast<float*>(stagingBufferD3D12->map());
        bufferD3D12->readBackDataLinear(sizeInBytes, hostPtr);
        std::string errorMessage;
        if (!checkIsArrayLinearTyped(formatInfo, width, height, hostPtr, errorMessage)) {
            descriptorAllocation = {};
            delete shaderManager;
            delete renderer;
            ASSERT_TRUE(false) << errorMessage;
        }
        //stagingBufferD3D12->unmap();

        // Free data.
        sycl::free(hostPtr, *syclQueue);
    }

    descriptorAllocation = {};
    delete shaderManager;
    delete renderer;
}
INSTANTIATE_TEST_SUITE_P(, InteropTestSyclD3D12Image, testedImageFormatsD3D12, PrintToStringFormatD3D12Config());
