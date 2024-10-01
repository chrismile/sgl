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

#ifndef SGL_WEBGPU_BUFFER_HPP
#define SGL_WEBGPU_BUFFER_HPP

#include <vector>
#include <memory>
#include <functional>

#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

class Device;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

struct DLL_OBJECT BufferSettings {
    uint64_t sizeInBytes = 0;
    WGPUBufferUsageFlags usage{};
    bool mappedAtCreation = false;
    char const* label = nullptr;
};

class DLL_OBJECT Buffer {
public:
    Buffer(Device* device, const BufferSettings& bufferSettings);
    ~Buffer();

    void write(void const* data, uint64_t dataSize, WGPUQueue queue);
    void write(uint64_t bufferOffset, void const* data, uint64_t dataSize, WGPUQueue queue);
    void copyToBuffer(const BufferPtr& dstBuffer, WGPUCommandEncoder encoder);
    void copyToBuffer(
            const BufferPtr& dstBuffer, uint64_t srcOffset, size_t dstOffset, size_t copySize, WGPUCommandEncoder encoder);

    [[nodiscard]] inline Device* getDevice() { return device; }
    [[nodiscard]] inline WGPUBuffer getWGPUBuffer() { return buffer; }
    [[nodiscard]] inline size_t getSizeInBytes() const { return bufferSettings.sizeInBytes; }
    [[nodiscard]] inline WGPUBufferUsageFlags getVkBufferUsageFlags() const { return bufferSettings.usage; }

    void mapAsyncRead(const std::function<void(const void* dataPtr)>& onBufferMappedCallback);
    void mapAsyncRead(size_t offset, size_t size, const std::function<void(const void* dataPtr)>& onBufferMappedCallback);
    void mapAsyncWrite(const std::function<void(void* dataPtr)>& onBufferMappedCallback);
    void mapAsyncWrite(size_t offset, size_t size, const std::function<void(void* dataPtr)>& onBufferMappedCallback);
    void mapAsyncReadWrite(const std::function<void(void* dataPtr)>& onBufferMappedCallback);
    void mapAsyncReadWrite(size_t offset, size_t size, const std::function<void(void* dataPtr)>& onBufferMappedCallback);

    const void* mapSyncRead();
    const void* mapSyncRead(size_t offset, size_t size);
    void* mapSyncWrite();
    void* mapSyncWrite(size_t offset, size_t size);
    void* mapSyncReadWrite();
    void* mapSyncReadWrite(size_t offset, size_t size);
    void unmapSync();

private:
    Device* device = nullptr;
    BufferSettings bufferSettings{};
    WGPUBuffer buffer{};
};

}}

#endif //SGL_WEBGPU_BUFFER_HPP
