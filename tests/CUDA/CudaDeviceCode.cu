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

#define DLL_OBJECT

#include <stdexcept>
#include <Math/Math.hpp>
#include "CudaDeviceCode.hpp"

#include <string>

#include "Graphics/Vulkan/libs/hip/include/hip/amd_detail/hip_fp16_gcc.h"

static bool isCudaRuntimeApiInitialized = false;

static void errorCheckCuda(cudaError_t cudaError, const char* name) {
    if (cudaError != cudaSuccess) {
        throw std::runtime_error(
            std::string() + "CUDA error (" + std::to_string(int(cudaError)) + ") in " + name + ": "
            + cudaGetErrorString(cudaError));
    }
}

bool getIsCudaRuntimeApiInitialized() {
    return isCudaRuntimeApiInitialized;
}

void setCudaDevice(CUdevice cuDevice) {
    cudaError_t cudaError = cudaSetDevice(cuDevice);
    errorCheckCuda(cudaError, "cudaSetDevice");
    cudaError = cudaFree(nullptr);
    errorCheckCuda(cudaError, "cudaSetDevice");
    isCudaRuntimeApiInitialized = true;
    // https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DRIVER.html
    //return cudaDeviceGetExecutionCtx();
}


template<typename T, int C> struct MakeVec;
template<> struct MakeVec<float, 1> {
    typedef union { float1 data; float arr[1]; } type;
};
template<> struct MakeVec<float, 2> {
    typedef union { float2 data; float arr[2]; } type;
};
template<> struct MakeVec<float, 4> {
    typedef union { float4 data; float arr[4]; } type;
};
template<> struct MakeVec<uint32_t, 1> {
    typedef union { uint1 data; uint32_t arr[1]; } type;
};
template<> struct MakeVec<uint32_t, 2> {
    typedef union { uint2 data; uint32_t arr[2]; } type;
};
template<> struct MakeVec<uint32_t, 4> {
    typedef union { uint4 data; uint32_t arr[4]; } type;
};
template<> struct MakeVec<uint16_t, 1> {
    typedef union { ushort1 data; uint16_t arr[1]; } type;
};
template<> struct MakeVec<uint16_t, 2> {
    typedef union { ushort2 data; uint16_t arr[2]; } type;
};
template<> struct MakeVec<uint16_t, 4> {
    typedef union { ushort4 data; uint16_t arr[4]; } type;
};
template<> struct MakeVec<__half, 1> {
    typedef union { float1 data; float arr[1]; } type;
};
template<> struct MakeVec<__half, 2> {
    typedef union { float2 data; float arr[2]; } type;
};
template<> struct MakeVec<__half, 4> {
    typedef union { float4 data; float arr[4]; } type;
};

template<typename T, int C>
__global__ void writeCudaSurfaceObjectIncreasingIndicesKernel(
        cudaSurfaceObject_t surfaceObject, int width, int height) {
    int idX = static_cast<int>(blockIdx.x) * blockDim.x + threadIdx.x;
    int idY = static_cast<int>(blockIdx.y) * blockDim.y + threadIdx.y;
    int linearIdx = (idX + idY * width) * C;
    if (idX >= width || idY >= height) {
        return;
    }
    typedef typename MakeVec<T, C>::type vect;
    vect element;
    for (int c = 0; c < C; c++) {
        if constexpr (std::is_same_v<T, __half>) {
            element.arr[c] = float(linearIdx + c);
        } else {
            element.arr[c] = T(linearIdx + c);
        }
    }
    surf2Dwrite(element.data, surfaceObject, idX * sizeof(vect), idY);
}

void writeCudaSurfaceObjectIncreasingIndices(
        CUstream cuStream, CUsurfObject surfaceObject, CUarray arrayL0, const sgl::FormatInfo& formatInfo, size_t width, size_t height) {
    auto iwidth = static_cast<int>(width);
    auto iheight = static_cast<int>(height);
    dim3 blockDim(16, 16, 1);
    dim3 gridDim(sgl::iceil(iwidth, static_cast<int>(blockDim.x)), sgl::iceil(iheight, static_cast<int>(blockDim.y)), 1);
    if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<float, 1><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<float, 2><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT32) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<float, 4><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<uint32_t, 1><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<uint32_t, 2><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::UINT32) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<uint32_t, 4><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<uint16_t, 1><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<uint16_t, 2><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::UINT16) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<uint16_t, 4><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 1 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<__half, 1><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 2 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<__half, 2><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else if (formatInfo.numChannels == 4 && formatInfo.channelFormat == sgl::ChannelFormat::FLOAT16) {
        writeCudaSurfaceObjectIncreasingIndicesKernel<__half, 4><<<gridDim, blockDim, 0, cuStream>>>(surfaceObject, iwidth, iheight);
    } else {
        throw std::runtime_error("Error in writeCudaSurfaceObjectIncreasingIndices: Unsupported number of channels.");
    }
    errorCheckCuda(cudaPeekAtLastError(), "cudaPeekAtLastError");
    errorCheckCuda(cudaStreamSynchronize(cuStream), "cudaDeviceSynchronize");
    /*auto* data = new float[width * height * formatInfo.numChannels];
    cudaMemcpy2DFromArrayAsync(data, width * sizeof(float) * formatInfo.numChannels, reinterpret_cast<cudaArray_t>(arrayL0), 0, 0, width * sizeof(float) * formatInfo.numChannels, height, cudaMemcpyDeviceToHost, cuStream);
    errorCheckCuda(cudaStreamSynchronize(cuStream), "cudaDeviceSynchronize");
    delete[] data;*/
}
