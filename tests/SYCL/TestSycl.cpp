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

#include "../SYCL/SyclDeviceCode.hpp"

class TestSycl : public ::testing::Test {
protected:
    void SetUp() override {
        sgl::Logfile::get()->createLogfile("LogfileSyclD3D12.html", "TestSyclD3D12");

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
