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

#define DLL_OBJECT

#include <stdexcept>
#include "SyclDeviceCode.hpp"

typedef sycl::half half;
#include "../Utils/FormatRange.hpp"

sycl::event writeSyclBufferData(sycl::queue& queue, size_t numEntries, float* devicePtr) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.parallel_for<class LinearWriteKernel>(sycl::range<1>{numEntries}, [=](sycl::id<1> it) {
            const auto index = it[0];
            devicePtr[index] = float(index);
        });
    });
    return event;
}


template<typename T, int C>
sycl::event copySyclBindlessImageToBuffer(
        sycl::queue& queue, syclexp::unsampled_image_handle img, size_t width, size_t height,
        T* devicePtr, const sycl::event& depEvent) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.depends_on(depEvent);
        cgh.parallel_for(sycl::range<2>{width, height}, [=](sycl::id<2> it) {
            const auto x = it[0];
            const auto y = it[1];
            const auto index = (x + y * width) * static_cast<size_t>(C);
            auto data = syclexp::fetch_image<sycl::vec<T, C>>(img, sycl::int2{x, y});
            for (int c = 0; c < C; c++) {
                devicePtr[index + c] = data[c];
            }
        });
    });
    return event;
}

sycl::event copySyclBindlessImageToBuffer(
        sycl::queue& queue, syclexp::unsampled_image_handle img,
        const sgl::FormatInfo& formatInfo, size_t width, size_t height,
        void* devicePtr, const sycl::event& depEvent) {
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        return copySyclBindlessImageToBuffer<float, 1>(queue, img, width, height, static_cast<float*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        return copySyclBindlessImageToBuffer<float, 2>(queue, img, width, height, static_cast<float*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        return copySyclBindlessImageToBuffer<float, 4>(queue, img, width, height, static_cast<float*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        return copySyclBindlessImageToBuffer<uint32_t, 1>(queue, img, width, height, static_cast<uint32_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        return copySyclBindlessImageToBuffer<uint32_t, 2>(queue, img, width, height, static_cast<uint32_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        return copySyclBindlessImageToBuffer<uint32_t, 4>(queue, img, width, height, static_cast<uint32_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        return copySyclBindlessImageToBuffer<uint16_t, 1>(queue, img, width, height, static_cast<uint16_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        return copySyclBindlessImageToBuffer<uint16_t, 2>(queue, img, width, height, static_cast<uint16_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        return copySyclBindlessImageToBuffer<uint16_t, 4>(queue, img, width, height, static_cast<uint16_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        return copySyclBindlessImageToBuffer<uint16_t, 1>(queue, img, width, height, static_cast<uint16_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        return copySyclBindlessImageToBuffer<uint16_t, 2>(queue, img, width, height, static_cast<uint16_t*>(devicePtr), depEvent);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        return copySyclBindlessImageToBuffer<uint16_t, 4>(queue, img, width, height, static_cast<uint16_t*>(devicePtr), depEvent);
    }
    throw std::runtime_error("Error in writeSyclBindlessImageIncreasingIndices: Unsupported number of channels.");
    return sycl::event();
}


template<typename T, int C>
sycl::event writeSyclBindlessImageIncreasingIndices(
        sycl::queue& queue, syclexp::unsampled_image_handle img, size_t width, size_t height) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.parallel_for(sycl::range<2>{width, height}, [=](sycl::id<2> it) {
            const auto x = it[0];
            const auto y = it[1];
            const auto index = (x + y * width) * static_cast<size_t>(C);
            if constexpr (std::is_same_v<T, sycl::half>) {
                sycl::vec<uint16_t, C> data;
                for (int c = 0; c < C; c++) {
                    data[c] = sycl::bit_cast<uint16_t>(sycl::half(float(index + c)));
                    //data[c] = sycl::detail::float2Half(index + c); // may not use __SYCL_DEVICE_ONLY__ code path
                    //data[c] = sycl::half(index + c); // sycl::half constructor doesn't work, converts to float
                }
                syclexp::write_image<sycl::vec<uint16_t, C>>(img, sycl::int2{x, y}, data);
            } else {
                sycl::vec<T, C> data;
                for (int c = 0; c < C; c++) {
                    data[c] = T(index + c);
                }
                syclexp::write_image<sycl::vec<T, C>>(img, sycl::int2{x, y}, data);
            }
        });
    });
    return event;
}

template<typename T, int C>
sycl::event writeSyclBindlessImageIncreasingIndicesModulo(
        sycl::queue& queue, syclexp::unsampled_image_handle img, size_t width, size_t height) {
    auto event = queue.submit([&](sycl::handler& cgh) {
        cgh.parallel_for(sycl::range<2>{width, height}, [=](sycl::id<2> it) {
            const auto x = it[0];
            const auto y = it[1];
            const auto index = (x + y * width) * static_cast<size_t>(C);
            if constexpr (std::is_same_v<T, sycl::half>) {
                sycl::vec<uint16_t, C> data;
                for (int c = 0; c < C; c++) {
                    data[c] = sycl::bit_cast<uint16_t>(sycl::half(float((index + c) % size_t(format_range<T>::modulo_value))));
                }
                syclexp::write_image<sycl::vec<uint16_t, C>>(img, sycl::int2{x, y}, data);
            } else {
                sycl::vec<T, C> data;
                for (int c = 0; c < C; c++) {
                    data[c] = T((index + c) % size_t(format_range<T>::modulo_value));
                }
                syclexp::write_image<sycl::vec<T, C>>(img, sycl::int2{x, y}, data);
            }
        });
    });
    return event;
}

sycl::event writeSyclBindlessImageIncreasingIndices(
        sycl::queue& queue, syclexp::unsampled_image_handle img,
        const sgl::FormatInfo& formatInfo, size_t width, size_t height) {
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        return writeSyclBindlessImageIncreasingIndices<float, 1>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        return writeSyclBindlessImageIncreasingIndices<float, 2>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        return writeSyclBindlessImageIncreasingIndices<float, 4>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        return writeSyclBindlessImageIncreasingIndices<uint32_t, 1>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        return writeSyclBindlessImageIncreasingIndices<uint32_t, 2>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        return writeSyclBindlessImageIncreasingIndices<uint32_t, 4>(queue, img, width, height);
    }
    // Maximum representable integer value is 2048 for uint16_t.
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        return writeSyclBindlessImageIncreasingIndicesModulo<uint16_t, 1>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        return writeSyclBindlessImageIncreasingIndicesModulo<uint16_t, 2>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        return writeSyclBindlessImageIncreasingIndicesModulo<uint16_t, 4>(queue, img, width, height);
    }
    // Maximum representable integer value is 2048 for float16_t.
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        return writeSyclBindlessImageIncreasingIndicesModulo<sycl::half, 1>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        return writeSyclBindlessImageIncreasingIndicesModulo<sycl::half, 2>(queue, img, width, height);
    }
    if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        return writeSyclBindlessImageIncreasingIndicesModulo<sycl::half, 4>(queue, img, width, height);
    }
    throw std::runtime_error("Error in writeSyclBindlessImageIncreasingIndices: Unsupported number of channels.");
    return sycl::event();
}
