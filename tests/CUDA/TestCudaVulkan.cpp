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

#include <iostream>
#include <utility>
#include <gtest/gtest.h>

#include <Math/Math.hpp>
#include <Utils/Format.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/InteropCompute.hpp>
#include <Graphics/Vulkan/Utils/InteropCuda.hpp>
#include <Graphics/Vulkan/Utils/InteropCompute/ImplCuda.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/CommandBuffer.hpp>
#include <Graphics/Vulkan/Render/ComputePipeline.hpp>
#include <Graphics/Vulkan/Render/Data.hpp>

#include "../Utils/Common.hpp"
#include "../Vulkan/ImageFormatsVulkan.hpp"
#include "../CUDA/CudaDeviceCode.hpp"

/*
 * Set of tests using either Level Zero, CUDA or HIP (depending on the GPU).
 */

class InteropTestCudaVulkan : public ::testing::Test {
protected:
    explicit InteropTestCudaVulkan() = default;

    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileCudaVulkan.html", "TestCudaVulkan");

        sgl::resetComputeApiState();
        sgl::setOpenMessageBoxOnComputeApiError(false);
        instance = new sgl::vk::Instance;
        instance->createInstance({}, false);

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
            VkPhysicalDeviceDriverProperties physicalDeviceDriverProperties{};
            physicalDeviceDriverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
            VkPhysicalDeviceProperties2 deviceProperties2 = {};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            deviceProperties2.pNext = &physicalDeviceDriverProperties;
            sgl::vk::getPhysicalDeviceProperties2(physicalDevice, deviceProperties2);
            return physicalDeviceDriverProperties.driverID == VK_DRIVER_ID_NVIDIA_PROPRIETARY;
        };
        device->setPhysicalDeviceCheckCallback(physicalDeviceCheckCallback);

        std::vector<const char*> optionalDeviceExtensions = sgl::vk::Device::getCudaInteropDeviceExtensions();
        std::vector<const char*> requiredDeviceExtensions = { VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME };
        sgl::vk::DeviceFeatures requestedDeviceFeatures{};
        device->createDeviceHeadless(
                instance, requiredDeviceExtensions, optionalDeviceExtensions, requestedDeviceFeatures);
        std::cout << "Running on " << device->getDeviceName() << std::endl;

        if (device->getDeviceDriverId() == VK_DRIVER_ID_NVIDIA_PROPRIETARY) {
            cudaInteropInitialized = true;
            if (!sgl::initializeCudaDeviceApiFunctionTable()) {
                cudaInteropInitialized = false;
                sgl::Logfile::get()->writeError(
                        "Error in main: sgl::initializeCudaDeviceApiFunctionTable() returned false.",
                        false);
            }

            if (cudaInteropInitialized && !getIsCudaRuntimeApiInitialized()) {
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
                setCudaDevice(cuDevice);
                //CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuCtxCreate(
                //        &cuContext, CU_CTX_SCHED_SPIN, cuDevice);
                //sgl::checkCUresult(cuResult, "Error in cuCtxCreate: ");
                //cuResult = sgl::g_cudaDeviceApiFunctionTable.cuCtxPushCurrent(cuContext);
                //sgl::checkCUresult(cuResult, "Error in cuCtxPushCurrent: ");
                //cuCtxGetCurrent
                CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuStreamCreate(&cuStream, CU_STREAM_DEFAULT);
                sgl::checkCUresult(cuResult, "Error in cuStreamCreate: ");

                computeApi = sgl::InteropComputeApi::CUDA;
                streamWrapper.cuStream = cuStream;
            }
            if (computeApi == sgl::InteropComputeApi::NONE) {
                FAIL() << "No compute API could be initialized";
            }
        } else {
            GTEST_SKIP() << "No NVIDIA GPU found";
        }
    }

    void TearDown() override {
        if (device) {
            device->waitIdle();
        }
        delete device;
        delete instance;

        if (sgl::getIsCudaDeviceApiFunctionTableInitialized()) {
            if (cuContext) {
                //CUcontext oldContext{};
                //CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuCtxPopCurrent(&oldContext);
                //sgl::checkCUresult(cuResult, "Error in cuCtxSetCurrent: ");
                //cuResult = sgl::g_cudaDeviceApiFunctionTable.cuCtxDestroy(cuContext);
                //sgl::checkCUresult(cuResult, "Error in cuCtxDestroy: ");
                //CUresult cuResult = sgl::g_cudaDeviceApiFunctionTable.cuDevicePrimaryCtxReset(cuDevice);
                //sgl::checkCUresult(cuResult, "Error in cuCtxDestroy: ");
                cuContext = {};
            }
            sgl::freeCudaDeviceApiFunctionTable();
        }
    }

    sgl::StreamWrapper getStreamWrapper() {
        sgl::StreamWrapper stream{};

        if (computeApi == sgl::InteropComputeApi::CUDA) {
            stream.cuStream = cuStream;
        }

        return stream;
    }

    sgl::vk::Instance* instance = nullptr;
    sgl::vk::Device* device = nullptr;

    sgl::InteropComputeApi computeApi = sgl::InteropComputeApi::NONE;
    sgl::StreamWrapper streamWrapper{};

    bool cudaInteropInitialized = false;
    CUcontext cuContext = {};
    CUdevice cuDevice = 0;
    CUstream cuStream = {};
};


template<class T>
std::string getVkFormatStringCuda(const T& info) {
    VkFormat format = std::get<0>(info.param);
    uint32_t width = std::get<1>(info.param);
    uint32_t height = std::get<2>(info.param);
    auto formatString = sgl::vk::convertVkFormatToString(format);
    if (sgl::vk::getImageFormatChannelByteSize(format) == 4 && (width != 1024 || height != 1024)) {
        formatString += "_" + std::to_string(width) + "x" + std::to_string(height);
    }
    return formatString;
}

class InteropTestCudaVulkanImageCudaWriteVulkanRead
        : public InteropTestCudaVulkan, public testing::WithParamInterface<std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>> {
public:
    InteropTestCudaVulkanImageCudaWriteVulkanRead() = default;
};
struct PrintToStringFormatSemaphoreConfig {
    std::string operator()(const testing::TestParamInfo<std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>>& info) const {
        return getVkFormatStringCuda(info);
    }
};
TEST_P(InteropTestCudaVulkanImageCudaWriteVulkanRead, Formats) {
    const auto [format, width, height, useSemaphore, isFormatRequired] = GetParam();

    auto* shaderManager = new sgl::vk::ShaderManagerVk(device);
    auto renderer = new sgl::vk::Renderer(device);

    sgl::vk::ImageSettings imageSettings{};
    imageSettings.width = width;
    imageSettings.height = height;
    imageSettings.format = format;
    imageSettings.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    imageSettings.exportMemory = true;
    imageSettings.useDedicatedAllocationForExportedMemory = true;
    auto formatInfo = sgl::vk::getImageFormatInfo(format);
    size_t sizeInBytes = imageSettings.width * imageSettings.height * formatInfo.formatSizeInBytes;

    const char* SHADER_STRING_COPY_IMAGE_TO_BUFFER_COMPUTE_FMT = R"(
    #version 450 core
    $5
    layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
    #define NUM_CHANNELS $1
    #define tvec4 $2
    #define tvecx $3
    layout(binding = 0, $0) uniform restrict readonly $4image2D srcImage;
    layout(binding = 1, std430) writeonly buffer DestBuffer {
        tvecx destBuffer[];
    };
    void main() {
        ivec2 srcImageSize = imageSize(srcImage);
        ivec2 idx = ivec2(gl_GlobalInvocationID.xy);
        if (idx.x >= srcImageSize.x || idx.y >= srcImageSize.y) {
            return;
        }
        int linearIdx = idx.x + idx.y * srcImageSize.x;
        tvec4 imageEntry = imageLoad(srcImage, idx);
    #if NUM_CHANNELS == 1
        destBuffer[linearIdx] = tvecx(imageEntry.x);
    #elif NUM_CHANNELS == 2
        destBuffer[linearIdx] = tvecx(imageEntry.xy);
    #elif NUM_CHANNELS == 4
        destBuffer[linearIdx] = tvecx(imageEntry);
    #else
    #error Unsupported number of image channels.
    #endif
    }
    )";
    std::string imageTypePrefix;
    if (formatInfo.channelCategory == sgl::ChannelCategory::UINT) {
        imageTypePrefix = "u";
    } if (formatInfo.channelCategory == sgl::ChannelCategory::INT) {
        imageTypePrefix = "i";
    }
    std::string extensionString;
    if (formatInfo.channelSizeInBytes == 1) {
        extensionString = "#extension GL_EXT_shader_8bit_storage : require";
    } else if (formatInfo.channelSizeInBytes == 2) {
        extensionString = "#extension GL_EXT_shader_16bit_storage : require";
    }
    auto shaderStringWriteImageCompute = sgl::formatStringPositional(
            SHADER_STRING_COPY_IMAGE_TO_BUFFER_COMPUTE_FMT,
            sgl::vk::getImageFormatGlslString(format),
            sgl::vk::getImageFormatNumChannels(format),
            sgl::vk::getImageFormatGlslTypeStringUnsized(formatInfo.channelCategory, 4),
            sgl::vk::getImageFormatGlslTypeStringSized(format),
            imageTypePrefix,
            extensionString);
    auto shaderStages = shaderManager->compileComputeShaderFromStringCached(
            "CopyImageToBufferShader.Compute", shaderStringWriteImageCompute);

    std::string errorMessage;
    try {
        for (int it = 0; it < 1000; it++) {
            // Create semaphore.
            uint64_t timelineValue = 0;
            sgl::vk::SemaphoreVkComputeApiInteropPtr semaphoreVulkan = sgl::vk::createSemaphoreVkComputeApiInterop(
                    device, 0, VK_SEMAPHORE_TYPE_TIMELINE, timelineValue);
            auto fence = std::make_shared<sgl::vk::Fence>(device);

            // Create image and buffers.
            auto imageViewVulkan = std::make_shared<sgl::vk::ImageView>(
                    std::make_shared<sgl::vk::Image>(device, imageSettings));
            sgl::vk::UnsampledImageVkComputeApiExternalMemoryPtr imageInterop =
                    sgl::vk::createUnsampledImageVkComputeApiExternalMemory(imageViewVulkan->getImage());
            auto imageInteropCuda = std::static_pointer_cast<sgl::vk::UnsampledImageVkCudaInterop>(imageInterop);

            sgl::vk::BufferSettings bufferSettings{};
            bufferSettings.sizeInBytes = sizeInBytes;
            bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            auto bufferVulkan = std::make_shared<sgl::vk::Buffer>(device, bufferSettings);
            bufferSettings.memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
            bufferSettings.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            auto stagingBufferVulkan = std::make_shared<sgl::vk::Buffer>(device, bufferSettings);

            // Create command buffer.
            sgl::vk::CommandPoolType commandPoolType;
            commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            auto commandBuffer = std::make_shared<sgl::vk::CommandBuffer>(device, commandPoolType);

            sgl::vk::ComputePipelineInfo computePipelineInfo(shaderStages);
            sgl::vk::ComputePipelinePtr computePipeline(new sgl::vk::ComputePipeline(device, computePipelineInfo));
            auto computeData = std::make_shared<sgl::vk::ComputeData>(renderer, computePipeline);
            computeData->setStaticImageView(imageViewVulkan, 0);
            computeData->setStaticBuffer(bufferVulkan, 1);

            // Write data with CUDA.
            CUsurfObject surfaceObject = imageInteropCuda->getCudaSurfaceObject();
            CUarray arrayL0 = imageInteropCuda->getCudaMipmappedArrayLevel(0);
            writeCudaSurfaceObjectIncreasingIndices(
                    cuStream, surfaceObject, arrayL0, formatInfo, imageSettings.width, imageSettings.height);
            if (useSemaphore) {
                timelineValue++;
                semaphoreVulkan->signalSemaphoreComputeApi(getStreamWrapper(), timelineValue);
            } else {
                sgl::waitForCompletion(sgl::vk::decideInteropComputeApi(device), getStreamWrapper());
            }

            // Copy image data to buffer with Vulkan.
            renderer->pushCommandBuffer(commandBuffer);
            commandBuffer->setFence(fence);
            if (useSemaphore) {
                semaphoreVulkan->setWaitSemaphoreValue(timelineValue);
                commandBuffer->pushWaitSemaphore(semaphoreVulkan);
            }
            renderer->beginCommandBuffer();
            renderer->insertImageMemoryBarrier(
                    imageViewVulkan->getImage(),
                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_QUEUE_FAMILY_EXTERNAL, renderer->getDevice()->getGraphicsQueueIndex());
            renderer->dispatch(computeData, sgl::uiceil(imageSettings.width, 16u), sgl::uiceil(imageSettings.height, 16u), 1);
            renderer->insertBufferMemoryBarrier(
                    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    bufferVulkan);
            bufferVulkan->copyDataTo(stagingBufferVulkan, commandBuffer->getVkCommandBuffer());
            renderer->endCommandBuffer();
            renderer->submitToQueue();
            fence->wait();

            // Check equality.
            void* hostPtr = stagingBufferVulkan->mapMemory();
            if (!checkIsArrayLinearTyped(formatInfo, imageSettings.width, imageSettings.height, hostPtr, errorMessage)) {
                stagingBufferVulkan->unmapMemory();
                ASSERT_TRUE(false) << errorMessage;
            }
            stagingBufferVulkan->unmapMemory();

            device->waitIdle(); // Should not be necessary.
            computeData = {};

            // Free data.
            imageInterop = {};
            imageInteropCuda = {};
            imageViewVulkan = {};
            bufferVulkan = {};
            stagingBufferVulkan = {};
        }
    } catch (sgl::UnsupportedComputeApiFeatureException const& e) {
        errorMessage = e.what();
    }

    shaderStages = {};
    delete renderer;
    delete shaderManager;

    if (!errorMessage.empty()) {
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
INSTANTIATE_TEST_SUITE_P(TestFormatsAsync, InteropTestCudaVulkanImageCudaWriteVulkanRead, testedImageFormatsReadWriteAsync, PrintToStringFormatSemaphoreConfig());
INSTANTIATE_TEST_SUITE_P(TestFormatsSync, InteropTestCudaVulkanImageCudaWriteVulkanRead, testedImageFormatsReadWriteSync, PrintToStringFormatSemaphoreConfig());
