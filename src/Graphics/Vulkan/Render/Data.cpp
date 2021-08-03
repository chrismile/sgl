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

#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>
#include "../Utils/Device.hpp"
#include "../Utils/Swapchain.hpp"
#include "Renderer.hpp"
#include "GraphicsPipeline.hpp"
#include "RayTracingPipeline.hpp"
#include "Data.hpp"

namespace sgl { namespace vk {

ComputeData::ComputeData(ComputePipelinePtr& computePipeline) : computePipeline(computePipeline) {
}


RenderData::RenderData(Renderer* renderer, ShaderStagesPtr& shaderStages)
        : renderer(renderer), device(renderer->getDevice()), shaderStages(shaderStages) {
    swapchainRecreatedEventListenerToken = EventManager::get()->addListener(
            SWAPCHAIN_RECREATED_EVENT, [this](EventPtr){ this->onSwapchainRecreated(); });
    onSwapchainRecreated();
}

RenderData::~RenderData() {
    EventManager::get()->removeListener(SWAPCHAIN_RECREATED_EVENT, swapchainRecreatedEventListenerToken);

    for (FrameData& frameData : frameDataList) {
        vkFreeDescriptorSets(
                device->getVkDevice(), renderer->getVkDescriptorPool(),
                1, &frameData.descriptorSet);
    }
    frameDataList.clear();
}

RenderDataPtr RenderData::copy(ShaderStagesPtr& shaderStages) {
    RenderDataPtr renderDataCopy(new RenderData(this->renderer, shaderStages));
    renderDataCopy->frameDataList = this->frameDataList;
    for (FrameData& frameData : renderDataCopy->frameDataList) {
        frameData.descriptorSet = VK_NULL_HANDLE;
    }
    renderDataCopy->buffersStatic = buffersStatic;
    renderDataCopy->bufferViewsStatic = bufferViewsStatic;
    renderDataCopy->imageViewsStatic = imageViewsStatic;
    renderDataCopy->accelerationStructuresStatic = accelerationStructuresStatic;
    return renderDataCopy;
}


void RenderData::setStaticBuffer(BufferPtr& buffer, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.buffers[binding] = buffer;
    }
    buffersStatic[binding] = true;
    isDirty = true;
}
void RenderData::setStaticBuffer(BufferPtr& buffer, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticBuffer(buffer, descriptorInfo.binding);
}

void RenderData::setStaticBufferView(BufferViewPtr& bufferView, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.bufferViews[binding] = bufferView;
    }
    bufferViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setStaticBufferView(BufferViewPtr& bufferView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticBufferView(bufferView, descriptorInfo.binding);
}

void RenderData::setStaticImageView(ImageViewPtr& imageView, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.imageViews[binding] = imageView;
    }
    imageViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setImageSampler(ImageSamplerPtr& imageSampler, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.imageSamplers[binding] = imageSampler;
    }
    isDirty = true;
}
void RenderData::setStaticTexture(TexturePtr& texture, uint32_t binding) {
    setStaticImageView(texture->getImageView(), binding);
    setImageSampler(texture->getImageSampler(), binding);
}

void RenderData::setStaticImageView(ImageViewPtr& imageView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticImageView(imageView, descriptorInfo.binding);
}
void RenderData::setImageSampler(ImageSamplerPtr& imageSampler, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setImageSampler(imageSampler, descriptorInfo.binding);
}
void RenderData::setStaticTexture(TexturePtr& texture, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticImageView(texture->getImageView(), descriptorInfo.binding);
    setImageSampler(texture->getImageSampler(), descriptorInfo.binding);
}


void RenderData::setDynamicBuffer(BufferPtr& buffer, uint32_t binding) {
    frameDataList.front().buffers[binding] = buffer;
    for (size_t i = 1; i < frameDataList.size(); i++) {
        FrameData& frameData = frameDataList.at(i);
        frameData.buffers[binding] = buffer->copy(false);
    }
    buffersStatic[binding] = true;
    isDirty = true;
}
void RenderData::setDynamicBuffer(BufferPtr& buffer, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setDynamicBuffer(buffer, descriptorInfo.binding);
}

void RenderData::setDynamicBufferView(BufferViewPtr& bufferView, uint32_t binding) {
    frameDataList.front().bufferViews[binding] = bufferView;
    for (size_t i = 1; i < frameDataList.size(); i++) {
        FrameData& frameData = frameDataList.at(i);
        frameData.bufferViews[binding] = bufferView->copy(true, false);
    }
    bufferViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setDynamicBufferView(BufferViewPtr& bufferView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setDynamicBufferView(bufferView, descriptorInfo.binding);
}

void RenderData::setDynamicImageView(ImageViewPtr& imageView, uint32_t binding) {
    frameDataList.front().imageViews[binding] = imageView;
    for (size_t i = 1; i < frameDataList.size(); i++) {
        FrameData& frameData = frameDataList.at(i);
        frameData.imageViews[binding] = imageView->copy(true, false);
    }
    imageViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setDynamicImageView(ImageViewPtr& imageView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setDynamicImageView(imageView, descriptorInfo.binding);
}

BufferPtr RenderData::getBuffer(uint32_t binding) {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    return frameDataList.at(swapchain ? swapchain->getImageIndex() : 0).buffers.at(binding);
}
BufferPtr RenderData::getBuffer(const std::string& name) {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, name);
    return frameDataList.at(swapchain ? swapchain->getImageIndex() : 0).buffers.at(descriptorInfo.binding);
}

VkDescriptorSet RenderData::getVkDescriptorSet() {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    return frameDataList.at(swapchain ? swapchain->getImageIndex() : 0).descriptorSet;
}


void RenderData::_updateDescriptorSets() {
    if (!isDirty) {
        return;
    }
    isDirty = false;

    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = shaderStages->getVkDescriptorSetLayouts();

    if (descriptorSetLayouts.size() > 2) {
        Logfile::get()->writeInfo(
                "Warning in RenderData::RenderData: More than two descriptor sets used by the shaders."
                "So far, sgl only supports one user-defined set (0) and one transformation matrix set (1).");
    }
    if (descriptorSetLayouts.size() < 2) {
        Logfile::get()->throwError(
                "Expected exactly two descriptor sets - one user-defined set (0) and one transformation matrix "
                "set (1).");
    }

    const VkDescriptorSetLayout& descriptorSetLayout = descriptorSetLayouts.at(0);
    const std::vector<DescriptorInfo>& descriptorSetInfo = shaderStages->getDescriptorSetsInfo().find(0)->second;

    for (FrameData& frameData : frameDataList) {
        if (frameData.descriptorSet == VK_NULL_HANDLE) {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = renderer->getVkDescriptorPool();
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout;

            if (vkAllocateDescriptorSets(
                    device->getVkDevice(), &allocInfo, &frameData.descriptorSet) != VK_SUCCESS) {
                Logfile::get()->throwError(
                        "Error in Renderer::updateMatrixBlock: Failed to allocate descriptor sets!");
            }
        }

        struct DescWriteData {
            VkDescriptorImageInfo imageInfo;
            VkBufferView bufferView;
            VkDescriptorBufferInfo bufferInfo;
            VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo;
        };
        std::vector<DescWriteData> descWriteDataArray;
        descWriteDataArray.resize(descriptorSetInfo.size());
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(descriptorSetInfo.size());

        for (size_t i = 0; i < descriptorSetInfo.size(); i++) {
            DescWriteData& descWriteData = descWriteDataArray.at(i);
            const DescriptorInfo& descriptorInfo = descriptorSetInfo.at(i);
            VkWriteDescriptorSet& descriptorWrite = descriptorWrites.at(i);
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = frameData.descriptorSet;
            descriptorWrite.dstBinding = descriptorInfo.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = descriptorInfo.type;
            descriptorWrite.descriptorCount = 1;

            if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_SAMPLER
                        || descriptorInfo.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        || descriptorInfo.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                        || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_SAMPLER
                            || descriptorInfo.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                    auto it = frameData.imageSamplers.find(descriptorInfo.binding);
                    if (it == frameData.imageSamplers.end()) {
                        Logfile::get()->throwError(
                                "Error in RenderData::_updateDescriptorSets: Couldn't find sampler with binding "
                                + std::to_string(descriptorInfo.binding) + ".");
                    }
                    descWriteData.imageInfo.sampler = it->second->getVkSampler();
                }
                if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                            || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                            || descriptorInfo.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                            || descriptorInfo.type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
                    auto it = frameData.imageViews.find(descriptorInfo.binding);
                    if (it == frameData.imageViews.end()) {
                        Logfile::get()->throwError(
                                "Error in RenderData::_updateDescriptorSets: Couldn't find image view with binding "
                                + std::to_string(descriptorInfo.binding) + ".");
                    }
                    descWriteData.imageInfo.imageView = it->second->getVkImageView();
                    VkImageLayout imageLayout;
                    if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                        imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    } else {
                        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    descWriteData.imageInfo.imageLayout = imageLayout;
                }
                descriptorWrite.pImageInfo = &descWriteData.imageInfo;
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                auto it = frameData.bufferViews.find(descriptorInfo.binding);
                if (it == frameData.bufferViews.end()) {
                    Logfile::get()->throwError(
                            "Error in RenderData::_updateDescriptorSets: Couldn't find buffer view with binding "
                            + std::to_string(descriptorInfo.binding) + ".");
                }
                descWriteData.bufferView = it->second->getVkBufferView();
                descriptorWrite.pTexelBufferView = &descWriteData.bufferView;
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                auto it = frameData.buffers.find(descriptorInfo.binding);
                if (it == frameData.buffers.end()) {
                    Logfile::get()->throwError(
                            "Error in RenderData::_updateDescriptorSets: Couldn't find buffer with binding "
                            + std::to_string(descriptorInfo.binding) + ".");
                }
                descWriteData.bufferInfo.buffer = it->second->getVkBuffer();
                descWriteData.bufferInfo.offset = 0;
                descWriteData.bufferInfo.range = it->second->getSizeInBytes();
                descriptorWrite.pBufferInfo = &descWriteData.bufferInfo;
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
                auto it = frameData.accelerationStructures.find(descriptorInfo.binding);
                if (it == frameData.accelerationStructures.end()) {
                    Logfile::get()->throwError(
                            "Error in RenderData::_updateDescriptorSets: Couldn't find acceleration structure with "
                            "binding " + std::to_string(descriptorInfo.binding) + ".");
                }
                descWriteData.accelerationStructureInfo.sType =
                        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                descWriteData.accelerationStructureInfo.accelerationStructureCount = 1;
                descWriteData.accelerationStructureInfo.pAccelerationStructures = &it->second;
                descriptorWrite.pNext = &descWriteData.accelerationStructureInfo;
            }
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(
                    device->getVkDevice(), uint32_t(descriptorWrites.size()), descriptorWrites.data(),
                    0, nullptr);
        }
    }
}

void RenderData::onSwapchainRecreated() {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    size_t numImages = swapchain ? swapchain->getNumImages() : 1;
    if (frameDataList.size() > numImages) {
        // Free the now unused frame data.
        for (size_t i = numImages; i < frameDataList.size(); i++) {
            FrameData& frameData = frameDataList.at(i);
            vkFreeDescriptorSets(
                    device->getVkDevice(), renderer->getVkDescriptorPool(),
                    1, &frameData.descriptorSet);
        }
        frameDataList.resize(numImages);
    } else if (frameDataList.size() < numImages) {
        if (frameDataList.empty()) {
            frameDataList.resize(numImages);
        } else {
            // Add frame data for the newly added swapchain images.
            FrameData& firstFrameData = frameDataList.front();
            size_t sizeDifference = numImages - frameDataList.size();
            for (size_t i = 0; i < sizeDifference; i++) {
                FrameData frameData;
                for (auto& it : firstFrameData.buffers) {
                    if (buffersStatic[it.first]) {
                        frameData.buffers.insert(it);
                    } else {
                        BufferPtr& firstBuffer = it.second;
                        BufferPtr newBuffer = firstBuffer->copy(false);
                        frameData.buffers.insert(std::make_pair(it.first, newBuffer));
                    }
                }
                for (auto& it : firstFrameData.bufferViews) {
                    if (bufferViewsStatic[it.first]) {
                        frameData.bufferViews.insert(it);
                    } else {
                        BufferViewPtr& firstBufferView = it.second;
                        BufferViewPtr newBufferView = firstBufferView->copy(true, false);
                        frameData.bufferViews.insert(std::make_pair(it.first, newBufferView));
                    }
                }
                for (auto& it : firstFrameData.imageViews) {
                    if (imageViewsStatic[it.first]) {
                        frameData.imageViews.insert(it);
                    } else {
                        ImageViewPtr& firstImageView = it.second;
                        ImageViewPtr newImageView = firstImageView->copy(true, false);
                        frameData.imageViews.insert(std::make_pair(it.first, newImageView));
                    }
                }
                for (auto& it : firstFrameData.imageSamplers) {
                    if (buffersStatic[it.first]) {
                        frameData.imageSamplers.insert(it);
                    } else {
                        Logfile::get()->throwError(
                                "Error in RenderData::onSwapchainRecreated: Dynamic samplers are not supported.");
                    }
                }
                for (auto& it : firstFrameData.accelerationStructures) {
                    if (buffersStatic[it.first]) {
                        frameData.accelerationStructures.insert(it);
                    } else {
                        Logfile::get()->throwError(
                                "Error in RenderData::onSwapchainRecreated: Dynamic acceleration structures are "
                                "not supported.");
                    }
                }
                frameDataList.push_back(frameData);
            }
        }
    }
}


inline size_t getIndexTypeByteSize(VkIndexType indexType) {
    if (indexType == VK_INDEX_TYPE_UINT32) {
        return 4;
    } else if (indexType == VK_INDEX_TYPE_UINT16) {
        return 2;
    } else if (indexType == VK_INDEX_TYPE_UINT8_EXT) {
        return 1;
    } else {
        Logfile::get()->throwError("Error in getIndexTypeByteSize: Invalid index type.");
        return 1;
    }
}

RasterData::RasterData(Renderer* renderer, GraphicsPipelinePtr& graphicsPipeline)
        : RenderData(renderer, graphicsPipeline->getShaderStages()),
        graphicsPipeline(graphicsPipeline) {
}

void RasterData::setIndexBuffer(BufferPtr& buffer, VkIndexType indexType) {
    indexBuffer = buffer;
    this->indexType = indexType;
    numIndices = buffer->getSizeInBytes() / getIndexTypeByteSize(indexType);
}

void RasterData::setVertexBuffer(BufferPtr& buffer, uint32_t binding) {
    bool isFirstVertexBuffer = vertexBuffers.empty();

    const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions =
            graphicsPipeline->getVertexInputBindingDescriptions();
    if (uint32_t(vertexInputBindingDescriptions.size()) <= binding) {
        Logfile::get()->throwError(
                "Error in RasterData::setVertexBuffer: Binding point missing in vertex input binding "
                "description list.");
    }
    const VkVertexInputBindingDescription& vertexInputBindingDescription = vertexInputBindingDescriptions.at(binding);
    size_t numVerticesNew = buffer->getSizeInBytes() / vertexInputBindingDescription.stride;

    if (!isFirstVertexBuffer && numVertices != numVerticesNew) {
        Logfile::get()->throwError("Error in RasterData::setVertexBuffer: Inconsistent number of vertices.");
    }

    if (vertexBuffers.size() <= binding) {
        vertexBuffers.resize(binding + 1);
        vulkanVertexBuffers.resize(binding + 1);
    }

    vertexBuffers.at(binding) = buffer;
    vulkanVertexBuffers.at(binding) = buffer->getVkBuffer();
    numVertices = numVerticesNew;
}

void RasterData::setVertexBuffer(BufferPtr& buffer, const std::string& name) {
    uint32_t location = graphicsPipeline->getShaderStages()->getInputVariableLocation(name);
    setVertexBuffer(buffer, location);
}


RayTracingData::RayTracingData(Renderer* renderer, RayTracingPipelinePtr& rayTracingPipeline)
        : RenderData(renderer, rayTracingPipeline->getShaderStages()),
        rayTracingPipeline(rayTracingPipeline) {
}

}}
