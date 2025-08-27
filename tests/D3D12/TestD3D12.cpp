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

#include <Math/Math.hpp>
#include <Utils/File/Logfile.hpp>

#include <Graphics/D3D12/Utils/DXGIFactory.hpp>
#include <Graphics/D3D12/Utils/Device.hpp>
#include <Graphics/D3D12/Utils/Resource.hpp>
#include <Graphics/D3D12/Shader/Shader.hpp>
#include <Graphics/D3D12/Shader/ShaderManager.hpp>
#include <Graphics/D3D12/Render/CommandList.hpp>
#include <Graphics/D3D12/Render/Data.hpp>
#include <Graphics/D3D12/Render/Renderer.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Graphics/Vulkan/libs/stb/stb_image_write.h>

#include "Graphics/D3D12/Shader/ShaderModuleType.hpp"

#ifdef SUPPORT_SYCL_INTEROP
#include <Graphics/D3D12/Utils/InteropCompute.hpp>
#include <sycl/sycl.hpp>
#endif

class D3D12Test : public ::testing::Test {
protected:
    explicit D3D12Test() {}
    void SetUp() override {
        sgl::Logfile::get()->createLogfile("Logfile.html", "D3D12Test");
    }

    void TearDown() override {
    }
};

TEST_F(D3D12Test, SimpleTest) {
    sgl::d3d12::DXGIFactoryPtr dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
    dxgiFactory->enumerateDevices();
    sgl::d3d12::DevicePtr d3d12Device = dxgiFactory->createDeviceAny(D3D_FEATURE_LEVEL_12_0);
}

TEST_F(D3D12Test, SimpleTestBuffer) {
    sgl::d3d12::DXGIFactoryPtr dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
    sgl::d3d12::DevicePtr d3d12Device = dxgiFactory->createDeviceAny(D3D_FEATURE_LEVEL_12_0);

    float dataToUpload = 42.0f;
    sgl::d3d12::ResourceSettings bufferSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float), flags);
    sgl::d3d12::ResourcePtr bufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);
    bufferD3D12->uploadDataLinear(sizeof(float), &dataToUpload);
}

TEST_F(D3D12Test, SimpleTestTexture) {
    sgl::d3d12::DXGIFactoryPtr dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
    sgl::d3d12::DevicePtr d3d12Device = dxgiFactory->createDeviceAny(D3D_FEATURE_LEVEL_12_0);

    for (uint32_t res = 1; res <= 1024; res *= 2) {
        uint32_t width = res;
        uint32_t height = res;
        size_t numEntries = width * height * 4;
        size_t sizeInBytes = sizeof(float) * numEntries;
        auto* hostPtr = new float[numEntries];
        for (size_t i = 0; i < numEntries; i++) {
            hostPtr[i] = float(i);
        }

        sgl::d3d12::ResourceSettings imageSettings{};
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
        sgl::d3d12::ResourcePtr imageD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
        imageD3D12->uploadDataLinear(sizeInBytes, hostPtr);
        memset(hostPtr, 0, sizeInBytes);
        imageD3D12->readBackDataLinear(sizeInBytes, hostPtr);

        // Check equality.
        for (size_t i = 0; i < numEntries; i++) {
            if (hostPtr[i] != float(i)) {
                size_t channelIdx = i % 4;
                size_t x = (i / 4) % width;
                size_t y = (i / 4) / width;
                std::string errorMessage =
                        "Image content mismatch at res=" + std::to_string(res) + ", x=" + std::to_string(x) + ", y="
                        + std::to_string(y) + ", c=" + std::to_string(channelIdx);
                ASSERT_TRUE(false) << errorMessage;
            }
        }

        delete[] hostPtr;
    }
}

TEST_F(D3D12Test, ComputeShader) {
#ifndef SUPPORT_D3D_COMPILER
    GTEST_SKIP() << "D3D12 shader compiler is not enabled.";
#endif

    sgl::d3d12::DXGIFactoryPtr dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
    sgl::d3d12::DevicePtr d3d12Device = dxgiFactory->createDeviceAny(D3D_FEATURE_LEVEL_12_0);

    size_t bufferNumEntries = 2000;
    size_t bufferSizeInBytes = sizeof(float) * bufferNumEntries;
    sgl::d3d12::ResourceSettings bufferSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSizeInBytes, flags);
    sgl::d3d12::ResourcePtr bufferD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);

    auto* shaderManager = new sgl::d3d12::ShaderManagerD3D12();
    auto* renderer = new sgl::d3d12::Renderer(d3d12Device.get());

    auto computeShader = shaderManager->loadShaderFromHlslString(R"(
    cbuffer globalSettingsCB : register(b0) {
        uint numEntries;
    }
    RWStructuredBuffer<float> dstBuffer : register(u0);
    [numthreads(256, 1, 1)]
    void CSMain(
            uint3 groupID : SV_GroupID, uint3 dispatchThreadID : SV_DispatchThreadID,
            uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex) {
        const uint idx = dispatchThreadID.x;
        if (idx <= numEntries) {
            dstBuffer[idx] = float(idx);
        }
    }
    )", "WriteBufferShader.hlsl", sgl::d3d12::ShaderModuleType::COMPUTE, "CSMain", {});

    auto rootParameters = std::make_shared<sgl::d3d12::RootParameters>(computeShader);
    //rootParameters->pushConstants(1, 0);
    auto rpiGlobalSettingsCB = rootParameters->pushConstants("globalSettingsCB");
    //rootParameters->pushUnorderedAccessView(0);
    auto rpiDstBuffer = rootParameters->pushUnorderedAccessView("dstBuffer");

    auto computeData = std::make_shared<sgl::d3d12::ComputeData>(d3d12Device.get(), rootParameters);
    computeData->setRootConstantValue(rpiGlobalSettingsCB, uint32_t(bufferNumEntries));
    computeData->setUnorderedAccessView(rpiDstBuffer, bufferD3D12.get());

    auto commandList = std::make_shared<sgl::d3d12::CommandList>(d3d12Device.get(), sgl::d3d12::CommandListType::COMPUTE);
    renderer->setCommandList(commandList);
    auto threadGroupCount = sgl::uiceil(uint32_t(bufferNumEntries), computeShader->getThreadGroupSizeX());
    renderer->dispatch(computeData, threadGroupCount);
    renderer->submitAndWait();

    auto* hostPtr = new float[bufferNumEntries];
    bufferD3D12->readBackDataLinear(bufferSizeInBytes, hostPtr);

    // Check equality.
    for (size_t i = 0; i < bufferNumEntries; i++) {
        if (hostPtr[i] != float(i)) {
            std::string errorMessage = "Buffer content mismatch at i=" + std::to_string(i);
            ASSERT_TRUE(false) << errorMessage;
        }
    }

    delete[] hostPtr;
    delete shaderManager;
    delete renderer;
}

TEST_F(D3D12Test, RasterPass) {
#ifndef SUPPORT_D3D_COMPILER
    GTEST_SKIP() << "D3D12 shader compiler is not enabled.";
#endif

    sgl::d3d12::DXGIFactoryPtr dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
    sgl::d3d12::DevicePtr d3d12Device = dxgiFactory->createDeviceAny(D3D_FEATURE_LEVEL_12_0);

    uint32_t width = 128;
    uint32_t height = 96;
    const glm::vec4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);
    sgl::d3d12::ResourceSettings imageSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
    imageSettings.optimizedClearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, clearColor };
    sgl::d3d12::ResourcePtr colorImage = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
    flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
    sgl::d3d12::ClearValue depthStencilClearValue{};
    depthStencilClearValue.depthStencilValue = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0 };
    imageSettings.optimizedClearValue = depthStencilClearValue;
    //imageSettings.optimizedClearValue = { .format = DXGI_FORMAT_D32_FLOAT, .depthStencilValue = D3D12_DEPTH_STENCIL_VALUE{ 1.0f, 0 } };
    sgl::d3d12::ResourcePtr depthImage = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);

    struct VertexPosAndColor {
        glm::vec2 position;
        glm::vec3 color;
    };
    std::vector<VertexPosAndColor> vertexData = {
        { {  0.0f, -0.5f }, { 1.0f, 0.5f, 0.0f } },
        { {  0.5f,  0.5f }, { 1.0f, 0.5f, 0.0f } },
        { { -0.5f,  0.5f }, { 1.0f, 0.5f, 0.0f } },
    };
    size_t vertexBufferSize = sizeof(VertexPosAndColor) * vertexData.size();
    std::vector<uint32_t> triangleIndexData = { 0, 1, 2 };
    size_t indexBufferSize = sizeof(uint32_t) * triangleIndexData.size();

    sgl::d3d12::ResourceSettings bufferSettings{};
    bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize, D3D12_RESOURCE_FLAG_NONE);
    sgl::d3d12::ResourcePtr vertexBuffer = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);
    bufferSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize, D3D12_RESOURCE_FLAG_NONE);
    sgl::d3d12::ResourcePtr indexBuffer = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), bufferSettings);
    vertexBuffer->uploadDataLinear(vertexBufferSize, vertexData.data());
    indexBuffer->uploadDataLinear(indexBufferSize, triangleIndexData.data());

    auto* shaderManager = new sgl::d3d12::ShaderManagerD3D12();
    auto* renderer = new sgl::d3d12::Renderer(d3d12Device.get());

    auto vertexShader = shaderManager->loadShaderFromHlslString(R"(
    struct VertexPosAndColor {
        float2 Position : POSITION;
        float4 Color    : COLOR;
    };
    struct VertexShaderOutput {
        float4 Color    : COLOR;
        float4 Position : SV_Position;
    };
    VertexShaderOutput VSMain(VertexPosAndColor IN) {
        VertexShaderOutput OUT;
        OUT.Position = float4(IN.Position, 0.0f, 1.0f);
        OUT.Color = IN.Color;
        return OUT;
    }
    )", "RasterVertexShader.hlsl", sgl::d3d12::ShaderModuleType::VERTEX, "VSMain", {});
    auto pixelShader = shaderManager->loadShaderFromHlslString(R"(
    struct PixelShaderInput {
        float4 Color : COLOR;
    };
    float4 PSMain(PixelShaderInput IN) : SV_Target {
        return IN.Color;
    }
    )", "RasterPixelShader.hlsl", sgl::d3d12::ShaderModuleType::PIXEL, "PSMain", {});
    auto shaderStages = std::make_shared<sgl::d3d12::ShaderStages>(std::vector{ vertexShader, pixelShader });

    auto rootParameters = std::make_shared<sgl::d3d12::RootParameters>(shaderStages);
    auto rasterPipelineState = std::make_shared<sgl::d3d12::RasterPipelineState>(rootParameters, shaderStages);
    rasterPipelineState->pushInputElementDesc("POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0);
    rasterPipelineState->pushInputElementDesc("COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
    rasterPipelineState->setRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
    rasterPipelineState->setDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

    auto rasterData = std::make_shared<sgl::d3d12::RasterData>(renderer, rasterPipelineState);
    rasterData->setVertexBuffer(vertexBuffer, 0, sizeof(VertexPosAndColor));
    rasterData->setIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT);
    rasterData->setRenderTargetView(colorImage, 0);
    rasterData->setClearColor(clearColor);
    rasterData->setDepthStencilView(depthImage);
    rasterData->setClearDepthStencil(1.0f);

    auto commandList = std::make_shared<sgl::d3d12::CommandList>(d3d12Device.get(), sgl::d3d12::CommandListType::DIRECT);
    renderer->setCommandList(commandList);
    vertexBuffer->transition(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, commandList);
    indexBuffer->transition(D3D12_RESOURCE_STATE_INDEX_BUFFER, commandList);
    colorImage->transition(D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
    depthImage->transition(D3D12_RESOURCE_STATE_DEPTH_WRITE, commandList);
    renderer->render(rasterData);
    renderer->submitAndWait();

    auto* hostImagePtr = new uint8_t[width * height * 4];
    colorImage->readBackDataLinear(width * height * 4, hostImagePtr);

    stbi_write_png("triangle.png", width, height, 4, hostImagePtr, width * 4);

    delete[] hostImagePtr;
    rasterData = {}; //< references DescriptorAllocation from renderer
    delete shaderManager;
    delete renderer;
}

#ifdef SUPPORT_SYCL_INTEROP
TEST_F(D3D12Test, SyclInterop) {
    sgl::Logfile::get()->createLogfile("Logfile.html", "D3D12TestSyclInterop");

    sycl::queue syclQueue{sycl::gpu_selector_v};
    std::cout << "Running on " << syclQueue.get_device().get_info<sycl::info::device::name>() << "\n";
    sgl::setGlobalSyclQueue(syclQueue);
    sgl::setOpenMessageBoxOnComputeApiError(false);
    //sycl::detail::uuid_type uuid = syclQueue.get_device().get_info<sycl::ext::intel::info::device::uuid>();
    uint64_t syclLuid;
    auto syclDevice = syclQueue.get_device();
    sgl::initializeComputeApi(sgl::getSyclDeviceComputeApi(syclDevice));
    if (!sgl::getSyclDeviceLuid(syclDevice, syclLuid)) {
        GTEST_SKIP() << "SYCL device LUID could not be retrieved.";
    }
    if (!syclQueue.get_device().has(sycl::aspect::ext_oneapi_external_memory_import)) {
        GTEST_SKIP() << "ext_oneapi_external_memory_import not supported.";
    }
    if (!syclQueue.get_device().has(sycl::aspect::ext_oneapi_external_semaphore_import)) {
        GTEST_SKIP() << "ext_oneapi_external_semaphore_import not supported.";
    }

    sgl::d3d12::DXGIFactoryPtr dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
    sgl::d3d12::DevicePtr d3d12Device = dxgiFactory->createMatchingDevice(syclLuid, D3D_FEATURE_LEVEL_12_0);
    if (!d3d12Device) {
        GTEST_SKIP() << "No suitable D3D12 device found.";
    }
    sgl::d3d12::Renderer* renderer = new sgl::d3d12::Renderer(d3d12Device.get());

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
        auto* hostPtr = sycl::malloc_host<float>(1, syclQueue);

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
        stream.syclQueuePtr = &syclQueue;
        sycl::event waitSemaphoreEvent{};
        fence->waitFenceComputeApi(stream, timelineValue, &waitSemaphoreEvent);
        auto cpyEvent = syclQueue.memcpy(hostPtr, devicePtr, sizeof(float), waitSemaphoreEvent);
        cpyEvent.wait_and_throw();
        fence->waitOnCpu(timelineValue);

        // Test whether a race condition occurred.
        if (*hostPtr != 11) {
            delete renderer;
            FAIL() << "Race condition occurred.";
        }
        sycl::free(hostPtr, syclQueue);
    }

    delete renderer;
    sgl::freeAllComputeApis();
}
#endif
