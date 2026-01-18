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
#include <Graphics/D3D12/Utils/InteropCompute.hpp>

#ifdef SUPPORT_CUDA_INTEROP
#include <Graphics/D3D12/Utils/InteropCuda.hpp>
#endif

class InteropTestLowLevelInteropD3D12 : public ::testing::Test {
protected:
    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileLowLevelInteropD3D12.html", "TestLowLevelInteropD3D12");

        sgl::resetComputeApiState();
        sgl::setOpenMessageBoxOnComputeApiError(false);

        dxgiFactory = std::make_shared<sgl::d3d12::DXGIFactory>(true);
        d3d12Device = dxgiFactory->createDeviceAny(D3D_FEATURE_LEVEL_12_0);
        if (!d3d12Device) {
            GTEST_FAIL() << "No suitable D3D12 device found.";
        }
        std::cout << "Running on " << d3d12Device->getAdapterName() << std::endl;

#ifdef SUPPORT_CUDA_INTEROP
        if (d3d12Device->getVendor() == sgl::d3d12::DeviceVendor::NVIDIA) {
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
                if (!sgl::d3d12::getMatchingCudaDevice(d3d12Device.get(), &cuDevice)) {
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

        //if (computeApi == sgl::InteropComputeApi::NONE) {
        //    FAIL() << "No compute API could be initialized";
        //}
    }

    void TearDown() override {
        d3d12Device = {};
        dxgiFactory = {};

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
    }

    sgl::StreamWrapper getStreamWrapper() {
        sgl::StreamWrapper stream{};

#ifdef SUPPORT_CUDA_INTEROP
        if (computeApi == sgl::InteropComputeApi::CUDA) {
            stream.cuStream = cuStream;
        }
#endif

        return stream;
    }

    sgl::d3d12::DXGIFactoryPtr dxgiFactory;
    sgl::d3d12::DevicePtr d3d12Device = nullptr;

    sgl::InteropComputeApi computeApi = sgl::InteropComputeApi::NONE;
    sgl::StreamWrapper streamWrapper{};

#ifdef SUPPORT_CUDA_INTEROP
    bool cudaInteropInitialized = false;
    CUcontext cuContext = {};
    CUdevice cuDevice = 0;
    CUstream cuStream = {};
#endif
};

TEST_F(InteropTestLowLevelInteropD3D12, FenceAllocationTest) {
    if (computeApi == sgl::InteropComputeApi::NONE) {
        GTEST_SKIP() << "Compute API not initialized.";
    }

    uint64_t timelineValue = 0;
    sgl::d3d12::FenceD3D12ComputeApiInteropPtr fence =
            sgl::d3d12::createFenceD3D12ComputeApiInterop(d3d12Device.get(), timelineValue);
}

TEST_F(InteropTestLowLevelInteropD3D12, BufferAllocationTest) {
    if (computeApi == sgl::InteropComputeApi::NONE) {
        GTEST_SKIP() << "Compute API not initialized.";
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
    auto bufferComputeApi = sgl::d3d12::createBufferD3D12ComputeApiExternalMemory(bufferD3D12);
}

TEST_F(InteropTestLowLevelInteropD3D12, ImageAllocationTest) {
    if (computeApi == sgl::InteropComputeApi::NONE) {
        GTEST_SKIP() << "Compute API not initialized.";
    }

    uint32_t width = 1024;
    uint32_t height = 1024;

    sgl::d3d12::ResourceSettings imageSettings{};
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    imageSettings.resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, 1, 0, 1, 0, flags, D3D12_TEXTURE_LAYOUT_UNKNOWN);
    imageSettings.heapFlags = D3D12_HEAP_FLAG_SHARED;
    sgl::d3d12::ResourcePtr imageD3D12 = std::make_shared<sgl::d3d12::Resource>(d3d12Device.get(), imageSettings);
    sgl::d3d12::ImageD3D12ComputeApiExternalMemoryPtr imageComputeApi =
            sgl::d3d12::createImageD3D12ComputeApiExternalMemory(imageD3D12);
}
