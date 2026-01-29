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
#include <Utils/File/Logfile.hpp>
#include <Graphics/Utils/FormatInfo.hpp>

#include "../Utils/Common.hpp"
#include "CommonSycl.hpp"
#include "SyclDeviceCode.hpp"

class TestSycl : public ::testing::Test {
protected:
    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileSycl.html", "TestSycl");

        sycl::property_list syclQueueProperties = sycl::property_list{sycl::property::queue::in_order(), sycl::ext::intel::property::queue::immediate_command_list()};
        syclQueue = new sycl::queue{sycl::gpu_selector_v, syclQueueProperties};
        std::cout << "Running on " << syclQueue->get_device().get_info<sycl::info::device::name>() << std::endl;
    }

    void TearDown() override {
        delete syclQueue;
    }

    sycl::queue* syclQueue = nullptr;
};

TEST_F(TestSycl, WriteKernelLinearTest) {
    const size_t numEntries = 2000;
    const size_t sizeInBytes = numEntries * sizeof(float);
    auto* hostPtr = sycl::malloc_host<float>(numEntries, *syclQueue);
    auto* devicePtr = sycl::malloc_device<float>(numEntries, *syclQueue);

    auto writeEvent = writeSyclBufferData(*syclQueue, numEntries, devicePtr);
    auto copyEvent = syclQueue->memcpy(hostPtr, devicePtr, sizeInBytes, writeEvent);
    copyEvent.wait_and_throw();

    for (size_t i = 0; i < numEntries; i++) {
        if (hostPtr[i] != float(i)) {
            FAIL() << "Incorrect data read from host copy pointer.";
        }
    }

    sycl::free(hostPtr, *syclQueue);
    sycl::free(devicePtr, *syclQueue);
}

TEST_F(TestSycl, WriteKernelImageTest) {
    if (!syclQueue->get_device().has(sycl::aspect::ext_oneapi_external_memory_import)
            || !syclQueue->get_device().has(sycl::aspect::ext_oneapi_bindless_images)) {
        GTEST_SKIP() << "External bindless images import not supported.";
    }

    syclexp::image_descriptor imageDescriptor{};
    imageDescriptor.width = 1024;
    imageDescriptor.height = 1024;
    imageDescriptor.num_channels = 1;
    imageDescriptor.verify();
    sgl::FormatInfo formatInfo{};
    formatInfo.channelCategory = sgl::ChannelCategory::FLOAT;
    formatInfo.channelFormat = sgl::ChannelFormat::FLOAT32;
    formatInfo.numChannels = 1;
    formatInfo.channelSizeInBytes = 4;
    formatInfo.formatSizeInBytes = formatInfo.numChannels * formatInfo.channelSizeInBytes;

    const size_t numEntries = imageDescriptor.width * imageDescriptor.height * imageDescriptor.num_channels;
    auto* hostPtr = sycl::malloc_host<float>(numEntries, *syclQueue);

    std::vector<syclexp::image_memory_handle_type> supportedImageMemoryHandleTypes =
            syclexp::get_image_memory_support(imageDescriptor, *syclQueue);
    if (supportedImageMemoryHandleTypes.empty()) {
        GTEST_FAIL() << "No image memory handle types supported.";
    }
    if (!syclexp::is_image_handle_supported<syclexp::unsampled_image_handle>(
        imageDescriptor, syclexp::image_memory_handle_type::opaque_handle, *syclQueue)) {
        GTEST_FAIL() << "image_memory_handle_type::opaque_handle is not supported.";
    }
    auto imageMemoryHandle = syclexp::alloc_image_mem(imageDescriptor, *syclQueue);

    syclexp::unsampled_image_handle imageSyclHandle =
            syclexp::create_image(imageMemoryHandle, imageDescriptor, *syclQueue);

    sycl::event writeImgEvent = writeSyclBindlessImageIncreasingIndices(
            *syclQueue, imageSyclHandle, formatInfo, imageDescriptor.width, imageDescriptor.height);
    auto copyEvent = syclQueue->ext_oneapi_copy(imageMemoryHandle, hostPtr, imageDescriptor, writeImgEvent);
    //auto barrierEvent = syclQueue->ext_oneapi_submit_barrier({ copyEvent }); // broken
    sycl::event signalSemaphoreEvent{};
    copyEvent.wait_and_throw();

    std::string errorMessage;
    if (!checkIsArrayLinearTyped(formatInfo, imageDescriptor.width, imageDescriptor.height, hostPtr, errorMessage)) {
        ASSERT_TRUE(false) << errorMessage;
    }

    sycl::free(hostPtr, *syclQueue);
    syclexp::destroy_image_handle(imageSyclHandle, *syclQueue);
    syclexp::free_image_mem(imageMemoryHandle, imageDescriptor.type, *syclQueue);
}
