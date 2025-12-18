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

#include "SyclDeviceCode.hpp"

sycl::event writeSyclBufferData(sycl::queue& queue, size_t numEntries, float* devicePtr) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.parallel_for<class LinearWriteKernel>(sycl::range<1>{numEntries}, [=](sycl::id<1> it) {
            const auto index = it[0];
            devicePtr[index] = float(index);
        });
    });
    return event;
}

sycl::event copySyclBindlessImageToBuffer(
        sycl::queue& queue, sycl::ext::oneapi::experimental::unsampled_image_handle img, size_t width, size_t height,
        float* devicePtr, const sycl::event& depEvent) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.depends_on(depEvent);
        cgh.parallel_for<class CopyBindlessImageToBufferKernel>(sycl::range<2>{width, height}, [=](sycl::id<2> it) {
            const auto x = it[0];
            const auto y = it[1];
            const auto index = x + y * width;
            auto data = sycl::ext::oneapi::experimental::fetch_image<float>(img, sycl::int2{x, y});
            devicePtr[index] = data;
        });
    });
    return event;
}

sycl::event writeSyclBindlessImageIncreasingIndices(
        sycl::queue& queue, sycl::ext::oneapi::experimental::unsampled_image_handle img, size_t width, size_t height) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.parallel_for<class WriteBindlessIncreasingIndicesKernel>(sycl::range<2>{width, height}, [=](sycl::id<2> it) {
            const auto x = it[0];
            const auto y = it[1];
            const auto index = x + y * width;
            sycl::ext::oneapi::experimental::write_image<float>(img, sycl::int2{x, y}, float(index));
        });
    });
    return event;
}
