/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_COMPRESSION_HPP
#define SGL_COMPRESSION_HPP

#include <cstdint>
#include <memory>

namespace sgl {

#ifdef SUPPORT_VULKAN
namespace vk { class ImageView; typedef std::shared_ptr<ImageView> ImageViewPtr; class Renderer; }
#endif

DLL_OBJECT bool compressImageCpuBC6H(
        uint16_t* imageDataHalf, uint32_t imageWidth, uint32_t imageHeight,
        uint8_t*& imageDataOutCpu, size_t& imageDataOutputSizeInBytes
#ifdef SUPPORT_VULKAN
        , sgl::vk::Renderer* renderer = nullptr
#endif
        );

#ifdef SUPPORT_VULKAN
DLL_OBJECT sgl::vk::ImageViewPtr compressImageVulkanBC6H(
        uint16_t* imageDataHalf, uint32_t imageWidth, uint32_t imageHeight,
        sgl::vk::Renderer* renderer, bool runSync = true);
#endif

}

#endif //SGL_COMPRESSION_HPP
