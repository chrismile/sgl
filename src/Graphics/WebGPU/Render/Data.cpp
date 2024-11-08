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

#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Graphics/Window.hpp>
#include "../Utils/Device.hpp"
#include "../Buffer/Buffer.hpp"
#include "../Texture/Texture.hpp"
#include "../Shader/Shader.hpp"
#include "../Render/ComputePipeline.hpp"
#include "../Render/RenderPipeline.hpp"
#include "Renderer.hpp"
#include "Data.hpp"

namespace sgl { namespace webgpu {

Data::Data(Renderer* renderer, ShaderStagesPtr& shaderStages)
        : renderer(renderer), device(renderer->getDevice()), shaderStages(shaderStages) {
    swapchainRecreatedEventListenerToken = EventManager::get()->addListener(
            RESOLUTION_CHANGED_EVENT, [this](const EventPtr&){ this->onSwapchainRecreated(); });
    onSwapchainRecreated();
}

Data::~Data() {
    EventManager::get()->removeListener(RESOLUTION_CHANGED_EVENT, swapchainRecreatedEventListenerToken);

    if (bindGroup) {
        wgpuBindGroupRelease(bindGroup);
        bindGroup = {};
    }
}

void Data::setBuffer(const BufferPtr& buffer, uint32_t bindingIndex) {
    buffers[bindingIndex] = buffer;
    isDirty = true;
}
void Data::setBuffer(const BufferPtr& buffer, const std::string& descName) {
    const BindingEntry& bindingEntry = shaderStages->getBindingEntryByName(0, descName);
    setBuffer(buffer, bindingEntry.bindingIndex);
}
void Data::setBufferOptional(const BufferPtr& buffer, const std::string& descName) {
    uint32_t bindingIndex;
    if (shaderStages->getBindingEntryByNameOptional(0, descName, bindingIndex)) {
        setBuffer(buffer, bindingIndex);
    }
}

void Data::setBufferUnused(uint32_t bindingIndex) {
    const BindingEntry& descriptorInfo = shaderStages->getBindingEntryByIndex(0, bindingIndex);
    WGPUBufferUsageFlags usageFlags = WGPUBufferUsage_Storage;
    if (descriptorInfo.bindingEntryType == BindingEntryType::UNIFORM_BUFFER) {
        usageFlags = WGPUBufferUsage_Uniform;
    } else if (descriptorInfo.bindingEntryType == BindingEntryType::STORAGE_BUFFER) {
        usageFlags = WGPUBufferUsage_Storage;
    }
    sgl::webgpu::BufferSettings bufferSettings{};
    bufferSettings.sizeInBytes = sizeof(uint32_t);
    bufferSettings.usage = usageFlags;
    buffers[bindingIndex] = std::make_shared<sgl::webgpu::Buffer>(device, bufferSettings);
    isDirty = true;
}
void Data::setBufferUnused(const std::string& descName) {
    const BindingEntry& descriptorInfo = shaderStages->getBindingEntryByName(0, descName);
    WGPUBufferUsageFlags usageFlags = WGPUBufferUsage_Storage;
    if (descriptorInfo.bindingEntryType == BindingEntryType::UNIFORM_BUFFER) {
        usageFlags = WGPUBufferUsage_Uniform;
    } else if (descriptorInfo.bindingEntryType == BindingEntryType::STORAGE_BUFFER) {
        usageFlags = WGPUBufferUsage_Storage;
    }
    sgl::webgpu::BufferSettings bufferSettings{};
    bufferSettings.sizeInBytes = sizeof(uint32_t);
    bufferSettings.usage = usageFlags;
    buffers[descriptorInfo.bindingIndex] = std::make_shared<sgl::webgpu::Buffer>(device, bufferSettings);
    isDirty = true;
}

void Data::setTextureView(const TextureViewPtr& textureView, uint32_t bindingIndex) {
    textureViews[bindingIndex] = textureView;
    isDirty = true;
}
void Data::setTextureView(const TextureViewPtr& textureView, const std::string& descName) {
    const BindingEntry& descriptorInfo = shaderStages->getBindingEntryByName(0, descName);
    setTextureView(textureView, descriptorInfo.bindingIndex);
}
void Data::setTextureViewOptional(const TextureViewPtr& textureView, const std::string& descName) {
    uint32_t bindingIndex;
    if (shaderStages->getBindingEntryByNameOptional(0, descName, bindingIndex)) {
        setTextureView(textureView, bindingIndex);
    }
}
void Data::setSampler(const SamplerPtr& sampler, uint32_t bindingIndex) {
    samplers[bindingIndex] = sampler;
    isDirty = true;
}
void Data::setSampler(const SamplerPtr& sampler, const std::string& descName) {
    const BindingEntry& descriptorInfo = shaderStages->getBindingEntryByName(0, descName);
    setSampler(sampler, descriptorInfo.bindingIndex);
}
void Data::setSamplerOptional(const SamplerPtr& sampler, const std::string& descName) {
    uint32_t bindingIndex;
    if (shaderStages->getBindingEntryByNameOptional(0, descName, bindingIndex)) {
        setSampler(sampler, bindingIndex);
    }
}

BufferPtr Data::getBuffer(uint32_t bindingIndex) {
    return buffers.at(bindingIndex);
}
BufferPtr Data::getBuffer(const std::string& name) {
    const BindingEntry& descriptorInfo = shaderStages->getBindingEntryByName(0, name);
    return buffers.at(descriptorInfo.bindingIndex);
}

TextureViewPtr Data::getTextureView(uint32_t bindingIndex) {
    return textureViews.at(bindingIndex);
}
TextureViewPtr Data::getTextureView(const std::string& name) {
    const BindingEntry& descriptorInfo = shaderStages->getBindingEntryByName(0, name);
    return textureViews.at(descriptorInfo.bindingIndex);
}

void Data::_updateBindingGroups() {
    if (!isDirty) {
        return;
    }
    isDirty = false;

    const std::vector<WGPUBindGroupLayout>& bindGroupLayouts = shaderStages->getWGPUBindGroupLayouts();

    /*if (bindGroupLayouts.size() > 2) {
        Logfile::get()->writeInfo(
                "Warning in Data::Data: More than two descriptor sets used by the shaders."
                "So far, sgl only supports one user-defined set (0) and one transformation matrix set (1).");
    }
    if (bindGroupLayouts.size() < 2 && getDataType() == DataType::RASTER) {
        Logfile::get()->throwError(
                "Expected exactly two descriptor sets - one user-defined set (0) and one transformation matrix "
                "set (1).");
    }*/
    if (bindGroupLayouts.size() > 1) {
        Logfile::get()->writeInfo(
                "Warning in Data::Data: More than one descriptor set used by the shaders."
                "So far, sgl only supports one user-defined set (0).");
    }

    const WGPUBindGroupLayout& bindGroupLayout = bindGroupLayouts.at(0);
    const std::vector<BindingEntry>& descriptorSetInfo = shaderStages->getBindGroupsInfo().find(0)->second;

    if (bindGroup != nullptr) {
        wgpuBindGroupRelease(bindGroup);
        bindGroup = {};
    }

    std::vector<WGPUBindGroupEntry> bindGroupEntries;
    bindGroupEntries.resize(descriptorSetInfo.size());

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    for (size_t i = 0; i < descriptorSetInfo.size(); i++) {
        const BindingEntry& descriptorInfo = descriptorSetInfo.at(i);
        WGPUBindGroupEntry& bindGroupEntry = bindGroupEntries.at(i);
        bindGroupEntry.binding = descriptorInfo.bindingIndex;

        if (descriptorInfo.bindingEntryType == BindingEntryType::TEXTURE
                || descriptorInfo.bindingEntryType == BindingEntryType::SAMPLER
                || descriptorInfo.bindingEntryType == BindingEntryType::STORAGE_TEXTURE) {
            if (descriptorInfo.bindingEntryType == BindingEntryType::SAMPLER) {
                auto it = samplers.find(descriptorInfo.bindingIndex);
                if (it == samplers.end()) {
                    Logfile::get()->throwError(
                            "Error in Data::_updateBindingGroups: Couldn't find sampler with binding "
                            + std::to_string(descriptorInfo.bindingIndex) + ".");
                }
                bindGroupEntry.sampler = it->second->getWGPUSampler();
            }
            if (descriptorInfo.bindingEntryType == BindingEntryType::TEXTURE
                    || descriptorInfo.bindingEntryType == BindingEntryType::STORAGE_TEXTURE) {
                auto it = textureViews.find(descriptorInfo.bindingIndex);
                if (it == textureViews.end()) {
                    Logfile::get()->throwError(
                            "Error in Data::_updateBindingGroups: Couldn't find image view with binding "
                            + std::to_string(descriptorInfo.bindingIndex) + ".");
                }
                bindGroupEntry.textureView = it->second->getWGPUTextureView();
            }
        } else if (descriptorInfo.bindingEntryType == BindingEntryType::UNIFORM_BUFFER
                || descriptorInfo.bindingEntryType == BindingEntryType::STORAGE_BUFFER) {
            auto it = buffers.find(descriptorInfo.bindingIndex);
            if (it == buffers.end()) {
                Logfile::get()->throwError(
                        "Error in Data::_updateBindingGroups: Couldn't find buffer with binding "
                        + std::to_string(descriptorInfo.bindingIndex) + ".");
            }
            bindGroupEntry.buffer = it->second->getWGPUBuffer();
            bindGroupEntry.offset = 0;
            bindGroupEntry.size = it->second->getSizeInBytes();
        }
    }

    bindGroup = wgpuDeviceCreateBindGroup(device->getWGPUDevice(), &bindGroupDescriptor);
}

void Data::onSwapchainRecreated() {
}

DataSize Data::getDataSize() {
    DataSize dataSize;

    for (const auto& buffer : buffers) {
        const auto& descriptorInfo = shaderStages->getBindingEntryByIndex(0, buffer.first);
        if (descriptorInfo.bindingEntryType == BindingEntryType::STORAGE_BUFFER) {
            dataSize.storageBufferSize += buffer.second->getSizeInBytes();
        } else if (descriptorInfo.bindingEntryType == BindingEntryType::UNIFORM_BUFFER) {
            dataSize.uniformBufferSize += buffer.second->getSizeInBytes();
        }
    }

    for (const auto& textureView : textureViews) {
        auto textureSettings = textureView.second->getTextureSettings();
        dataSize.imageSize +=
                textureSettings.size.width * textureSettings.size.height * textureSettings.size.depthOrArrayLayers
                * getTextureFormatEntryByteSize(textureSettings.format);
    }

    return dataSize;
}

size_t Data::getDataSizeSizeInBytes() {
    DataSize dataSize = getDataSize();
    return
            dataSize.indexBufferSize + dataSize.vertexBufferSize + dataSize.storageBufferSize
            + dataSize.uniformBufferSize + dataSize.imageSize + dataSize.accelerationStructureSize;
}


ComputeData::ComputeData(Renderer* renderer, ComputePipelinePtr& computePipeline)
        : Data(renderer, computePipeline->getShaderStages()), computePipeline(computePipeline) {
}

void ComputeData::dispatch(
        uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, WGPUCommandEncoder commandEncoder) {
    WGPUComputePassDescriptor computePassDescriptor{};
    WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(
            commandEncoder, &computePassDescriptor);

    _updateBindingGroups();
    WGPUBindGroup bindGroup = getWGPUBindGroup();
    if (bindGroup != nullptr) {
        wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, bindGroup, 0, nullptr);
    }

    wgpuComputePassEncoderSetPipeline(computePassEncoder, computePipeline->getWGPUPipeline());
    wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, groupCountX, groupCountY, groupCountZ);
    wgpuComputePassEncoderEnd(computePassEncoder);
    wgpuComputePassEncoderRelease(computePassEncoder);
}

void ComputeData::dispatchIndirect(
        const sgl::webgpu::BufferPtr& dispatchIndirectBuffer, uint64_t offset, WGPUCommandEncoder commandEncoder) {
    WGPUComputePassDescriptor computePassDescriptor{};
    WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(
            commandEncoder, &computePassDescriptor);

    _updateBindingGroups();
    WGPUBindGroup bindGroup = getWGPUBindGroup();
    if (bindGroup != nullptr) {
        wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, bindGroup, 0, nullptr);
    }

    wgpuComputePassEncoderSetPipeline(computePassEncoder, computePipeline->getWGPUPipeline());
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(
            computePassEncoder, dispatchIndirectBuffer->getWGPUBuffer(), offset);
    wgpuComputePassEncoderEnd(computePassEncoder);
    wgpuComputePassEncoderRelease(computePassEncoder);
}

void ComputeData::dispatchIndirect(
        const sgl::webgpu::BufferPtr& dispatchIndirectBuffer, WGPUCommandEncoder commandEncoder) {
    dispatchIndirect(dispatchIndirectBuffer, 0, commandEncoder);
}


RenderData::RenderData(Renderer* renderer, RenderPipelinePtr& renderPipeline)
        : Data(renderer, renderPipeline->getShaderStages()), renderPipeline(renderPipeline) {
}

size_t getIndexTypeByteSize(WGPUIndexFormat indexType) {
    if (indexType == WGPUIndexFormat_Uint32) {
        return 4;
    } else if (indexType == WGPUIndexFormat_Uint16) {
        return 2;
    } else {
        Logfile::get()->throwError("Error in getIndexTypeByteSize: Invalid index type.");
        return 1;
    }
}

void RenderData::setIndexBuffer(const BufferPtr& buffer, WGPUIndexFormat indexType) {
    indexBuffer = buffer;
    this->indexType = indexType;
    numIndices = buffer->getSizeInBytes() / getIndexTypeByteSize(indexType);
}

void RenderData::setNumVertices(size_t _numVertices) {
    bool isFirstVertexBuffer = vertexBuffers.empty();
    if (!isFirstVertexBuffer && numVertices != _numVertices) {
        Logfile::get()->throwError("Error in RenderData::setNumVertices: Inconsistent number of vertices.");
    }
    numVertices = _numVertices;
}

void RenderData::setVertexBuffer(const BufferPtr& buffer, uint32_t bindingIndex) {
    bool isFirstVertexBuffer = vertexBuffers.empty();

    const auto& vertexBufferStrides = renderPipeline->getVertexBufferStrides();
    if (uint32_t(vertexBufferStrides.size()) <= bindingIndex) {
        Logfile::get()->throwError(
                "Error in RenderData::setVertexBuffer: Binding point missing in vertex input binding "
                "description list.");
    }
    size_t numVerticesNew = buffer->getSizeInBytes() / vertexBufferStrides.at(bindingIndex);

    if (!isFirstVertexBuffer && numVertices != numVerticesNew) {
        Logfile::get()->throwError("Error in RenderData::setVertexBuffer: Inconsistent number of vertices.");
    }

    if (vertexBuffers.size() <= bindingIndex) {
        vertexBuffers.resize(bindingIndex + 1);
        wgpuVertexBuffers.resize(bindingIndex + 1);
        vertexBufferSlots.resize(bindingIndex + 1);
    }

    vertexBuffers.at(bindingIndex) = buffer;
    wgpuVertexBuffers.at(bindingIndex) = buffer->getWGPUBuffer();
    vertexBufferSlots.at(bindingIndex) = bindingIndex;
    numVertices = numVerticesNew;
}

void RenderData::setVertexBuffer(const BufferPtr& buffer, const std::string& name) {
    uint32_t location = renderPipeline->getShaderStages()->getInputVariableLocationIndex(name);
    setVertexBuffer(buffer, location);
}

void RenderData::setVertexBufferOptional(const BufferPtr& buffer, const std::string& name) {
    if (renderPipeline->getShaderStages()->getHasInputVariable(name)) {
        uint32_t location = renderPipeline->getShaderStages()->getInputVariableLocationIndex(name);
        setVertexBuffer(buffer, location);
    }
}

DataSize RenderData::getDataSize() {
    DataSize dataSize = Data::getDataSize();

    if (indexBuffer) {
        dataSize.indexBufferSize = indexBuffer->getSizeInBytes();
    }

    for (auto& buffer : vertexBuffers) {
        dataSize.vertexBufferSize += buffer->getSizeInBytes();
    }

    return dataSize;
}

}}
