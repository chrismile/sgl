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

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>

#include "../Utils/Device.hpp"
#include "Buffer.hpp"

namespace sgl { namespace webgpu {

Buffer::Buffer(Device* device, const BufferSettings& bufferSettings) : device(device), bufferSettings(bufferSettings) {
    WGPUBufferDescriptor bufferDesc{};
    bufferDesc.size = bufferSettings.sizeInBytes;
    bufferDesc.usage = bufferSettings.usage;
    bufferDesc.mappedAtCreation = bufferSettings.mappedAtCreation;
    bufferDesc.label = bufferSettings.label;
    buffer = wgpuDeviceCreateBuffer(device->getWGPUDevice(), &bufferDesc);
}

Buffer::~Buffer() {
    wgpuBufferRelease(buffer);
}

void Buffer::write(void const* data, uint64_t dataSize, WGPUQueue queue) {
    write(0, data, dataSize, queue);
}

void Buffer::write(uint64_t bufferOffset, void const* data, uint64_t dataSize, WGPUQueue queue) {
    if (dataSize > getSizeInBytes()) {
        sgl::Logfile::get()->throwError("Error in Buffer::write: Buffer too small.");
    }
    wgpuQueueWriteBuffer(queue, buffer, bufferOffset, data, dataSize);
}

void Buffer::copyToBuffer(const BufferPtr& dstBuffer, WGPUCommandEncoder encoder) {
    copyToBuffer(dstBuffer, 0, 0, getSizeInBytes(), encoder);
}

void Buffer::copyToBuffer(
        const BufferPtr& dstBuffer, uint64_t srcOffset, size_t dstOffset, size_t copySize, WGPUCommandEncoder encoder) {
    if (srcOffset + copySize > dstOffset + dstBuffer->getSizeInBytes()) {
        Logfile::get()->throwError(
                "Error in Buffer::copyToBuffer: The destination buffer is not large enough to hold the copied data!");
    }
    wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer, srcOffset, dstBuffer->buffer, dstOffset, copySize);
}


void Buffer::mapAsyncRead(const std::function<void(const void* dataPtr)>& onBufferMappedCallback) {
    mapAsyncRead(0, getSizeInBytes(), onBufferMappedCallback);
}

void Buffer::mapAsyncRead(
        size_t offset, size_t size, const std::function<void(const void* dataPtr)>& onBufferMappedCallback) {
    struct ContextMapAsyncR {
        WGPUBuffer buffer{};
        size_t offset{};
        size_t size{};
        std::function<void(const void* dataPtr)> onBufferMappedCallback;
    };
    ContextMapAsyncR context{};
    context.buffer = buffer;
    context.offset = offset;
    context.size = size;
    context.onBufferMappedCallback = onBufferMappedCallback;
    wgpuBufferMapAsync(
            buffer, WGPUMapMode_Read, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userData) {
                if (status != WGPUBufferMapAsyncStatus_Success) {
                    sgl::Logfile::get()->writeError(
                            "Error in Buffer::mapAsyncRead: Failed with error code 0x"
                            + sgl::toHexString(status) + ".");
                    return;
                }
                auto* context = reinterpret_cast<ContextMapAsyncR*>(userData);
                const void* dataPtr = wgpuBufferGetConstMappedRange(context->buffer, context->offset, context->size);
                context->onBufferMappedCallback(dataPtr);
                wgpuBufferUnmap(context->buffer);
            }, (void*)&context);
}

void Buffer::mapAsyncWrite(const std::function<void(void* dataPtr)>& onBufferMappedCallback) {
    mapAsyncWrite(0, getSizeInBytes(), onBufferMappedCallback);
}

void Buffer::mapAsyncWrite(
        size_t offset, size_t size, const std::function<void(void* dataPtr)>& onBufferMappedCallback) {
    struct ContextMapAsyncW {
        WGPUBuffer buffer{};
        size_t offset{};
        size_t size{};
        std::function<void(void* dataPtr)> onBufferMappedCallback;
    };
    ContextMapAsyncW context{};
    context.buffer = buffer;
    context.offset = offset;
    context.size = size;
    context.onBufferMappedCallback = onBufferMappedCallback;
    wgpuBufferMapAsync(
            buffer, WGPUMapMode_Write, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userData) {
                if (status != WGPUBufferMapAsyncStatus_Success) {
                    sgl::Logfile::get()->writeError(
                            "Error in Buffer::mapAsyncWrite: Failed with error code 0x"
                            + sgl::toHexString(status) + ".");
                    return;
                }
                auto* context = reinterpret_cast<ContextMapAsyncW*>(userData);
                void* dataPtr = wgpuBufferGetMappedRange(context->buffer, context->offset, context->size);
                context->onBufferMappedCallback(dataPtr);
                wgpuBufferUnmap(context->buffer);
            }, (void*)&context);
}

void Buffer::mapAsyncReadWrite(const std::function<void(void* dataPtr)>& onBufferMappedCallback) {
    mapAsyncReadWrite(0, getSizeInBytes(), onBufferMappedCallback);
}

void Buffer::mapAsyncReadWrite(
        size_t offset, size_t size, const std::function<void(void* dataPtr)>& onBufferMappedCallback) {
    struct ContextMapAsyncRW {
        WGPUBuffer buffer{};
        size_t offset{};
        size_t size{};
        std::function<void(void* dataPtr)> onBufferMappedCallback;
    };
    ContextMapAsyncRW context{};
    context.buffer = buffer;
    context.offset = offset;
    context.size = size;
    context.onBufferMappedCallback = onBufferMappedCallback;
    wgpuBufferMapAsync(
            buffer, WGPUMapMode_Read | WGPUMapMode_Write, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userData) {
                if (status != WGPUBufferMapAsyncStatus_Success) {
                    sgl::Logfile::get()->writeError(
                            "Error in Buffer::mapAsyncReadWrite: Failed with error code 0x"
                            + sgl::toHexString(status) + ".");
                    return;
                }
                auto* context = reinterpret_cast<ContextMapAsyncRW*>(userData);
                void* dataPtr = wgpuBufferGetMappedRange(context->buffer, context->offset, context->size);
                context->onBufferMappedCallback(dataPtr);
                wgpuBufferUnmap(context->buffer);
            }, (void*)&context);
}


const void* Buffer::mapSyncRead() {
    return mapSyncRead(0, getSizeInBytes());
}

const void* Buffer::mapSyncRead(size_t offset, size_t size) {
    struct ContextMapSyncR {
        WGPUBuffer buffer{};
        bool ready = false;
    };
    ContextMapSyncR context{};
    wgpuBufferMapAsync(
            buffer, WGPUMapMode_Read, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userData) {
                if (status != WGPUBufferMapAsyncStatus_Success) {
                    sgl::Logfile::get()->writeError(
                            "Error in Buffer::mapSyncRead: Failed with error code 0x"
                            + sgl::toHexString(status) + ".");
                    return;
                }
                auto* context = reinterpret_cast<ContextMapSyncR*>(userData);
                context->ready = true;
            }, (void*)&context);
    while (!context.ready) {
        device->pollEvents(true);
    }
    return wgpuBufferGetConstMappedRange(buffer, offset, size);
}

void* Buffer::mapSyncWrite() {
    return mapSyncWrite(0, getSizeInBytes());
}

void* Buffer::mapSyncWrite(size_t offset, size_t size) {
    struct ContextMapSyncW {
        WGPUBuffer buffer{};
        bool ready = false;
    };
    ContextMapSyncW context{};
    wgpuBufferMapAsync(
            buffer, WGPUMapMode_Write, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userData) {
                if (status != WGPUBufferMapAsyncStatus_Success) {
                    sgl::Logfile::get()->writeError(
                            "Error in Buffer::mapSyncWrite: Failed with error code 0x"
                            + sgl::toHexString(status) + ".");
                    return;
                }
                auto* context = reinterpret_cast<ContextMapSyncW*>(userData);
                context->ready = true;
            }, (void*)&context);
    while (!context.ready) {
        device->pollEvents(true);
    }
    return wgpuBufferGetMappedRange(buffer, offset, size);
}

void* Buffer::mapSyncReadWrite() {
    return mapSyncReadWrite(0, getSizeInBytes());
}

void* Buffer::mapSyncReadWrite(size_t offset, size_t size) {
    struct ContextMapSyncRW {
        WGPUBuffer buffer{};
        bool ready = false;
    };
    ContextMapSyncRW context{};
    wgpuBufferMapAsync(
            buffer, WGPUMapMode_Read | WGPUMapMode_Write, offset, size,
            [](WGPUBufferMapAsyncStatus status, void* userData) {
                if (status != WGPUBufferMapAsyncStatus_Success) {
                    sgl::Logfile::get()->writeError(
                            "Error in Buffer::mapSyncReadWrite: Failed with error code 0x"
                            + sgl::toHexString(status) + ".");
                    return;
                }
                auto* context = reinterpret_cast<ContextMapSyncRW*>(userData);
                context->ready = true;
            }, (void*)&context);
    while (!context.ready) {
        device->pollEvents(true);
    }
    return wgpuBufferGetMappedRange(buffer, offset, size);
}

void Buffer::unmapSync() {
    wgpuBufferUnmap(buffer);
}

}}
