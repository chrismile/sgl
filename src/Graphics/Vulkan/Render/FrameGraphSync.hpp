/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#ifndef SGL_FRAMEGRAPHSYNC_HPP
#define SGL_FRAMEGRAPHSYNC_HPP

#include <vulkan/vulkan.h>
#include <Graphics/Vulkan/Buffers/Buffer.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>

namespace sgl { namespace vk {

enum class ResourceType {
    IMAGE, BUFFER
};

enum class ResourceUsage {
    INPUT_VARIABLE, INDEX_BUFFER, DESCRIPTOR_BINDING, OUTPUT_ATTACHMENT, COPY_SRC, COPY_DST, BLIT_SRC, BLIT_DST
};

enum class OutputAttachmentType {
    COLOR, DEPTH_STENCIL, RESOLVE
};

struct ResourceAccess {
    size_t passIdx;
    ResourceType resourceType;
    union {
        vk::Image* image;
        vk::Buffer* buffer;
    };
    ResourceUsage resourceUsage;
    VkShaderStageFlagBits shaderStage; ///< If resourceUsage == DESCRIPTOR_BINDING.
    VkDescriptorType descriptorType; ///< If resourceUsage == DESCRIPTOR_BINDING.
    bool writeAccess;
    OutputAttachmentType outputAttachmentType; ///< If resourceUsage == OUTPUT_ATTACHMENT.
};

struct MemoryBarrier {
    size_t srcPassIdx, dstPassIdx;
    ResourceType resourceType;
    union {
        vk::Image* image;
        vk::Buffer* buffer;
    };
};

} }

#endif //SGL_FRAMEGRAPHSYNC_HPP
