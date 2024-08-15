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

#ifndef SGL_INTEROPCUSTOM_HPP
#define SGL_INTEROPCUSTOM_HPP

#include <memory>
#include "../Buffers/Buffer.hpp"

namespace sgl { namespace vk {

/**
 * Interop for custom APIs from a Vulkan buffer.
 * It is assumed that an API like CUDA is used, where ownership of the file descriptor is transferred,
 * but ownership of a Windows handle is not transferred.
 */
class DLL_OBJECT BufferCustomInteropVk
{
public:
    explicit BufferCustomInteropVk(vk::BufferPtr& vulkanBuffer);
    virtual ~BufferCustomInteropVk();

    inline const sgl::vk::BufferPtr& getVulkanBuffer() { return vulkanBuffer; }
#ifdef _WIN32
    [[nodiscard]] inline HANDLE getHandle() const { return handle; }
#else
    [[nodiscard]] inline int getFileDescriptor() const { return fileDescriptor; }
#endif

protected:
    sgl::vk::BufferPtr vulkanBuffer;

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

typedef std::shared_ptr<BufferCustomInteropVk> BufferCustomInteropVkPtr;

}}

#endif //SGL_INTEROPCUSTOM_HPP
