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
#include "Helpers.hpp"
#include "Renderer.hpp"
#include "AccelerationStructure.hpp"
#include "ComputePipeline.hpp"
#include "GraphicsPipeline.hpp"
#include "RayTracingPipeline.hpp"
#include "Data.hpp"

namespace sgl { namespace vk {

RenderData::RenderData(Renderer* renderer, ShaderStagesPtr& shaderStages)
        : renderer(renderer), device(renderer->getDevice()), shaderStages(shaderStages) {
    swapchainRecreatedEventListenerToken = EventManager::get()->addListener(
            RESOLUTION_CHANGED_EVENT, [this](const EventPtr&){ this->onSwapchainRecreated(); });
    onSwapchainRecreated();
}

RenderData::~RenderData() {
    EventManager::get()->removeListener(RESOLUTION_CHANGED_EVENT, swapchainRecreatedEventListenerToken);

    for (FrameData& frameData : frameDataList) {
        vkFreeDescriptorSets(
                device->getVkDevice(), renderer->getVkDescriptorPool(),
                1, &frameData.descriptorSet);
    }
    frameDataList.clear();
}

//RenderDataPtr RenderData::copy(ShaderStagesPtr& shaderStages) {
//    RenderDataPtr renderDataCopy(new RenderData(this->renderer, shaderStages));
//    renderDataCopy->frameDataList = this->frameDataList;
//    for (FrameData& frameData : renderDataCopy->frameDataList) {
//        frameData.descriptorSet = VK_NULL_HANDLE;
//    }
//    renderDataCopy->buffersStatic = buffersStatic;
//    renderDataCopy->bufferViewsStatic = bufferViewsStatic;
//    renderDataCopy->imageViewsStatic = imageViewsStatic;
//    renderDataCopy->accelerationStructuresStatic = accelerationStructuresStatic;
//    return renderDataCopy;
//}


void RenderData::setStaticBuffer(const BufferPtr& buffer, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.buffers[binding] = buffer;
    }
    buffersStatic[binding] = true;
    isDirty = true;
}
void RenderData::setStaticBuffer(const BufferPtr& buffer, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticBuffer(buffer, descriptorInfo.binding);
}
void RenderData::setStaticBufferOptional(const BufferPtr& buffer, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setStaticBuffer(buffer, binding);
    }
}
void RenderData::setStaticBufferUnused(uint32_t binding) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByBinding(0, binding);
    for (FrameData& frameData : frameDataList) {
        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            || descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                   || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        frameData.buffers[binding] = std::make_shared<sgl::vk::Buffer>(
                device, sizeof(uint32_t), usageFlags, VMA_MEMORY_USAGE_GPU_ONLY);
    }
    buffersStatic[binding] = true;
    isDirty = true;
}
void RenderData::setStaticBufferUnused(const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    for (FrameData& frameData : frameDataList) {
        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                || descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        frameData.buffers[descriptorInfo.binding] = std::make_shared<sgl::vk::Buffer>(
                device, sizeof(uint32_t), usageFlags, VMA_MEMORY_USAGE_GPU_ONLY);
    }
    buffersStatic[descriptorInfo.binding] = true;
    isDirty = true;
}

void RenderData::setStaticBufferView(const BufferViewPtr& bufferView, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.bufferViews[binding] = bufferView;
    }
    bufferViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setStaticBufferView(const BufferViewPtr& bufferView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticBufferView(bufferView, descriptorInfo.binding);
}
void RenderData::setStaticBufferViewOptional(const BufferViewPtr& bufferView, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setStaticBufferView(bufferView, binding);
    }
}

void RenderData::setStaticImageView(const ImageViewPtr& imageView, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.imageViews[binding] = imageView;
    }
    imageViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setImageSampler(const ImageSamplerPtr& imageSampler, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.imageSamplers[binding] = imageSampler;
    }
    isDirty = true;
}
void RenderData::setStaticTexture(const TexturePtr& texture, uint32_t binding) {
    setStaticImageView(texture->getImageView(), binding);
    setImageSampler(texture->getImageSampler(), binding);
}

void RenderData::setStaticImageView(const ImageViewPtr& imageView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticImageView(imageView, descriptorInfo.binding);
}
void RenderData::setImageSampler(const ImageSamplerPtr& imageSampler, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setImageSampler(imageSampler, descriptorInfo.binding);
}
void RenderData::setStaticTexture(const TexturePtr& texture, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setStaticImageView(texture->getImageView(), descriptorInfo.binding);
    setImageSampler(texture->getImageSampler(), descriptorInfo.binding);
}
void RenderData::setStaticImageViewOptional(const ImageViewPtr& imageView, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setStaticImageView(imageView, binding);
    }
}
void RenderData::setImageSamplerOptional(const ImageSamplerPtr& imageSampler, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setImageSampler(imageSampler, binding);
    }
}
void RenderData::setStaticTextureOptional(const TexturePtr& texture, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setStaticImageView(texture->getImageView(), binding);
        setImageSampler(texture->getImageSampler(), binding);
    }
}

void RenderData::setTopLevelAccelerationStructure(const TopLevelAccelerationStructurePtr& tlas, uint32_t binding) {
    for (FrameData& frameData : frameDataList) {
        frameData.accelerationStructures[binding] = tlas;
    }
    imageViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setTopLevelAccelerationStructure(
        const TopLevelAccelerationStructurePtr& tlas, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setTopLevelAccelerationStructure(tlas, descriptorInfo.binding);
}
void RenderData::setTopLevelAccelerationStructureOptional(
        const TopLevelAccelerationStructurePtr& tlas, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setTopLevelAccelerationStructure(tlas, binding);
    }
}


void RenderData::setDynamicBuffer(const BufferPtr& buffer, uint32_t binding) {
    frameDataList.front().buffers[binding] = buffer;
    for (size_t i = 1; i < frameDataList.size(); i++) {
        FrameData& frameData = frameDataList.at(i);
        frameData.buffers[binding] = buffer->copy(false);
    }
    buffersStatic[binding] = true;
    isDirty = true;
}
void RenderData::setDynamicBuffer(const BufferPtr& buffer, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setDynamicBuffer(buffer, descriptorInfo.binding);
}
void RenderData::setDynamicBufferOptional(const BufferPtr& buffer, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setDynamicBuffer(buffer, binding);
    }
}

void RenderData::setDynamicBufferView(const BufferViewPtr& bufferView, uint32_t binding) {
    frameDataList.front().bufferViews[binding] = bufferView;
    for (size_t i = 1; i < frameDataList.size(); i++) {
        FrameData& frameData = frameDataList.at(i);
        frameData.bufferViews[binding] = bufferView->copy(true, false);
    }
    bufferViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setDynamicBufferView(const BufferViewPtr& bufferView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setDynamicBufferView(bufferView, descriptorInfo.binding);
}
void RenderData::setDynamicBufferViewOptional(const BufferViewPtr& bufferView, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setDynamicBufferView(bufferView, binding);
    }
}

void RenderData::setDynamicImageView(const ImageViewPtr& imageView, uint32_t binding) {
    frameDataList.front().imageViews[binding] = imageView;
    for (size_t i = 1; i < frameDataList.size(); i++) {
        FrameData& frameData = frameDataList.at(i);
        frameData.imageViews[binding] = imageView->copy(true, false);
    }
    imageViewsStatic[binding] = true;
    isDirty = true;
}
void RenderData::setDynamicImageView(const ImageViewPtr& imageView, const std::string& descName) {
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, descName);
    setDynamicImageView(imageView, descriptorInfo.binding);
}
void RenderData::setDynamicImageViewOptional(const ImageViewPtr& imageView, const std::string& descName) {
    uint32_t binding;
    if (shaderStages->getDescriptorBindingByNameOptional(0, descName, binding)) {
        setDynamicImageView(imageView, binding);
    }
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

ImageViewPtr RenderData::getImageView(uint32_t binding) {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    return frameDataList.at(swapchain ? swapchain->getImageIndex() : 0).imageViews.at(binding);
}
ImageViewPtr RenderData::getImageView(const std::string& name) {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, name);
    return frameDataList.at(swapchain ? swapchain->getImageIndex() : 0).imageViews.at(descriptorInfo.binding);
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
    if (descriptorSetLayouts.size() < 2 && getRenderDataType() == RenderDataType::RASTER) {
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
            VkAccelerationStructureKHR accelerationStructure;
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
                if (descriptorInfo.size > 0) {
                    descWriteData.bufferInfo.range = std::min(it->second->getSizeInBytes(), size_t(descriptorInfo.size));
                } else {
                    descWriteData.bufferInfo.range = it->second->getSizeInBytes();
                }
                descriptorWrite.pBufferInfo = &descWriteData.bufferInfo;
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
                auto it = frameData.accelerationStructures.find(descriptorInfo.binding);
                if (it == frameData.accelerationStructures.end()) {
                    Logfile::get()->throwError(
                            "Error in RenderData::_updateDescriptorSets: Couldn't find acceleration structure with "
                            "binding " + std::to_string(descriptorInfo.binding) + ".");
                }
                descWriteData.accelerationStructure = it->second->getAccelerationStructure();
                descWriteData.accelerationStructureInfo.sType =
                        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                descWriteData.accelerationStructureInfo.accelerationStructureCount = 1;
                descWriteData.accelerationStructureInfo.pAccelerationStructures = &descWriteData.accelerationStructure;
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

RenderDataSize RenderData::getRenderDataSize() {
    RenderDataSize renderDataSize;

    const auto& frameData = frameDataList.front();

    for (const auto& buffer : frameData.buffers) {
        const auto& descriptorInfo =
                shaderStages->getDescriptorInfoByBinding(0, buffer.first);
        if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            renderDataSize.storageBufferSize += buffer.second->getSizeInBytes();
        } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                || descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            renderDataSize.uniformBufferSize += buffer.second->getSizeInBytes();
        }
    }

    for (const auto& buffer : frameData.bufferViews) {
        const auto& descriptorInfo =
                shaderStages->getDescriptorInfoByBinding(0, buffer.first);
        if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            renderDataSize.storageBufferSize += buffer.second->getBuffer()->getSizeInBytes();
        } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                   || descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            renderDataSize.uniformBufferSize += buffer.second->getBuffer()->getSizeInBytes();
        }
    }

    for (const auto& imageView : frameData.imageViews) {
        auto imageFormat = imageView.second->getImage()->getImageSettings();
        renderDataSize.imageSize +=
                imageFormat.width * imageFormat.height * imageFormat.depth * imageFormat.arrayLayers
                * getImageFormatEntryByteSize(imageFormat.format);
    }

    for (const auto& tlas : frameData.accelerationStructures) {
        renderDataSize.accelerationStructureSize += tlas.second->getAccelerationStructureSizeInBytes();
    }

    return renderDataSize;
}

size_t RenderData::getRenderDataSizeSizeInBytes() {
    RenderDataSize renderDataSize = getRenderDataSize();
    return
            renderDataSize.indexBufferSize + renderDataSize.vertexBufferSize + renderDataSize.storageBufferSize
            + renderDataSize.uniformBufferSize + renderDataSize.imageSize + renderDataSize.accelerationStructureSize;
}


ComputeData::ComputeData(Renderer* renderer, ComputePipelinePtr& computePipeline)
        : RenderData(renderer, computePipeline->getShaderStages()), computePipeline(computePipeline) {
}

void ComputeData::dispatch(
        uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(
            commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipeline->getVkPipeline());

    _updateDescriptorSets();
    VkDescriptorSet descriptorSet = getVkDescriptorSet();
    if (descriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                computePipeline->getVkPipelineLayout(),
                0, 1, &descriptorSet, 0, nullptr);
    }

    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void ComputeData::pushConstants(uint32_t offset, uint32_t size, const void* data, VkCommandBuffer commandBuffer) {
    vkCmdPushConstants(
            commandBuffer, computePipeline->getVkPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
}


RasterData::RasterData(Renderer* renderer, GraphicsPipelinePtr& graphicsPipeline)
        : RenderData(renderer, graphicsPipeline->getShaderStages()),
        graphicsPipeline(graphicsPipeline) {
}

void RasterData::setIndexBuffer(const BufferPtr& buffer, VkIndexType indexType) {
    indexBuffer = buffer;
    this->indexType = indexType;
    numIndices = buffer->getSizeInBytes() / getIndexTypeByteSize(indexType);
}

void RasterData::setVertexBuffer(const BufferPtr& buffer, uint32_t binding) {
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

void RasterData::setVertexBuffer(const BufferPtr& buffer, const std::string& name) {
    uint32_t location = graphicsPipeline->getShaderStages()->getInputVariableLocationIndex(name);
    setVertexBuffer(buffer, location);
}

void RasterData::setVertexBufferOptional(const BufferPtr& buffer, const std::string& name) {
    if (graphicsPipeline->getShaderStages()->getHasInputVariable(name)) {
        uint32_t location = graphicsPipeline->getShaderStages()->getInputVariableLocationIndex(name);
        setVertexBuffer(buffer, location);
    }
}

RenderDataSize RasterData::getRenderDataSize() {
    RenderDataSize renderDataSize = RenderData::getRenderDataSize();

    if (indexBuffer) {
        renderDataSize.indexBufferSize = indexBuffer->getSizeInBytes();
    }

    for (auto& buffer : vertexBuffers) {
        renderDataSize.vertexBufferSize += buffer->getSizeInBytes();
    }

    return renderDataSize;
}


RayTracingData::RayTracingData(
        Renderer* renderer, RayTracingPipelinePtr& rayTracingPipeline, const ShaderGroupSettings& settings)
        : RenderData(renderer, rayTracingPipeline->getShaderStages()), rayTracingPipeline(rayTracingPipeline),
        shaderGroupSettings(settings) {
    stridedDeviceAddressRegions = rayTracingPipeline->getStridedDeviceAddressRegions(shaderGroupSettings);
}

void RayTracingData::setShaderGroupSettings(const ShaderGroupSettings& settings) {
    shaderGroupSettings = settings;
    stridedDeviceAddressRegions = rayTracingPipeline->getStridedDeviceAddressRegions(shaderGroupSettings);
}

}}
