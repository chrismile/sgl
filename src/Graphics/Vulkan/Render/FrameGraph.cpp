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

#include <set>
#include <unordered_set>
#include <Utils/File/Logfile.hpp>
#include <Utils/CircularQueue.hpp>
#include <Utils/AppSettings.hpp>
#include "../Utils/Swapchain.hpp"
#include "Renderer.hpp"
#include "FrameGraph.hpp"

namespace sgl { namespace vk {
/*
RenderPass::RenderPass(FrameGraph& frameGraph, PassId passId)
        : frameGraph(frameGraph), device(frameGraph.getDevice()), passId(passId) {}

void RenderPass::_addIngoingEdge(FrameGraphEdge& edge) {
    ingoingEdges.push_back(edge);
    std::sort(ingoingEdges.begin(), ingoingEdges.end());
}

void RenderPass::_addOutgoingEdge(FrameGraphEdge& edge) {
    outgoingEdges.push_back(edge);
    std::sort(outgoingEdges.begin(), outgoingEdges.end());
}



void FrameGraph::addPass(RenderPassPtr& renderPass) {
    dirty = true;
}

void FrameGraph::addEdge(PassId passId0, PassId passId1, uint32_t priority) {
    dirty = true;
}

void FrameGraph::setFinalRenderPass(RenderPassPtr& renderPass) {
    finalRenderPass = renderPass;
}

struct PipelineBarrierData {
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
};
typedef std::pair<VkPipelineStageFlags, VkPipelineStageFlags> PipelineStages;
typedef std::map<PipelineStages, PipelineBarrierData> PipelineBarrierCollection;
typedef std::vector<PipelineBarrierCollection> PipelineBarrierFrameData;

struct ResourceAccess {
    uint32_t passIdx;
    VkPipelineStageFlagBits pipelineStageFlags;
    bool writeAccess;
};

class FrameResourceAccessTracker {
public:
    void addAccess(VkBuffer buffer, uint32_t passIdx, VkPipelineStageFlagBits pipelineStageFlags, bool writeAccess);
    void addAccess(VkImage image, uint32_t passIdx, VkPipelineStageFlagBits pipelineStageFlags, bool writeAccess);

private:
    std::map<VkBuffer, std::vector<ResourceAccess>> bufferAccessList;
    std::map<VkImage, std::vector<ResourceAccess>> imageAccessList;
};

void FrameResourceAccessTracker::addAccess(
        VkBuffer buffer, uint32_t passIdx, VkPipelineStageFlagBits pipelineStageFlags, bool writeAccess) {
    ResourceAccess resourceAccess;
    resourceAccess.passIdx = passIdx;
    resourceAccess.pipelineStageFlags = pipelineStageFlags;
    resourceAccess.writeAccess = writeAccess;
    bufferAccessList[buffer].push_back(resourceAccess);
}

void FrameResourceAccessTracker::addAccess(
        VkImage image, uint32_t passIdx, VkPipelineStageFlagBits pipelineStageFlags, bool writeAccess) {
    ResourceAccess resourceAccess;
    resourceAccess.passIdx = passIdx;
    resourceAccess.pipelineStageFlags = pipelineStageFlags;
    resourceAccess.writeAccess = writeAccess;
    imageAccessList[image].push_back(resourceAccess);
}

PipelineBarrierCollection buildPipelineBarrierCollection(
        RenderData* renderData, GraphicsPipeline* graphicsPipeline, uint32_t frameIdx) {
    PipelineBarrierCollection pipelineBarrierCollection;

    RenderData::FrameData& frameData = renderData->getFrameData(frameIdx);
    const std::vector<InterfaceVariableDescriptor>& inputVariableDescriptors =
            graphicsPipeline->getShaderStages()->getInputVariableDescriptors();
    const std::map<uint32_t, std::vector<DescriptorInfo>>& descriptorSetsInfo =
            graphicsPipeline->getShaderStages()->getDescriptorSetsInfo();
    auto descIt = descriptorSetsInfo.find(0);
    if (descIt != descriptorSetsInfo.end()) {
        const std::vector<DescriptorInfo>& descriptorInfoList = descIt->second;
        for (const DescriptorInfo& descriptorInfo : descriptorInfoList) {
            descriptorInfo.shaderStageFlags;
            descriptorInfo.readOnly;
            descriptorInfo.binding;

            if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                    || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                    || descriptorInfo.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                    || descriptorInfo.type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
                auto it = frameData.imageViews.find(descriptorInfo.binding);
                if (it == frameData.imageViews.end()) {
                    Logfile::get()->throwError(
                            "Error in buildPipelineBarrierCollection: Couldn't find image view with binding "
                            + std::to_string(descriptorInfo.binding) + ".");
                }
                it->second->getImage()->getVkImage();
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                auto it = frameData.bufferViews.find(descriptorInfo.binding);
                if (it == frameData.bufferViews.end()) {
                    Logfile::get()->throwError(
                            "Error in buildPipelineBarrierCollection: Couldn't find buffer view with binding "
                            + std::to_string(descriptorInfo.binding) + ".");
                }
                it->second->getBuffer()->getVkBuffer();
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                       || descriptorInfo.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                auto it = frameData.buffers.find(descriptorInfo.binding);
                if (it == frameData.buffers.end()) {
                    Logfile::get()->throwError(
                            "Error in buildPipelineBarrierCollection: Couldn't find buffer with binding "
                            + std::to_string(descriptorInfo.binding) + ".");
                }
                it->second->getVkBuffer();
            } else if (descriptorInfo.type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
                // TODO
            }
        }
    }

    pipelineBarrierCollection[std::make_pair()];
}

PipelineBarrierFrameData buildPipelineBarrierFrameData(RenderData* renderData, GraphicsPipeline* graphicsPipeline) {
    Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
    size_t numImages = swapchain->getNumImages();

    PipelineBarrierFrameData pipelineBarrierFrameData;
    pipelineBarrierFrameData.reserve(numImages);
    for (size_t frameIdx = 0; frameIdx < numImages; frameIdx++) {
        pipelineBarrierFrameData.emplace_back(buildPipelineBarrierCollection(renderData, uint32_t(frameIdx)));
    }

    return pipelineBarrierFrameData;
}

void FrameGraph::_build() {
    RenderPass* currentRenderPass = finalRenderPass.get();
    if (currentRenderPass == nullptr) {
        Logfile::get()->throwError("Error in FrameGraph::_build: No final render pass was set.");
    }

    // Clear old linearized data.
    linearizedRenderPasses.clear();
    imageDependenciesMap.clear();
    bufferDependenciesMap.clear();

    // Linearize the render passes by utilizing a breadth-first search.
    std::set<FrameGraphEdge> visitedEdges;
    CircularQueue<FrameGraphEdge> edgeQueue;
    FrameGraphEdge firstEdge = std::make_pair(0, currentRenderPass);
    edgeQueue.push_back(firstEdge);
    visitedEdges.insert(firstEdge);
    while (!edgeQueue.is_empty()) {
        FrameGraphEdge currentEdge = edgeQueue.pop_front();
        currentRenderPass = currentEdge.second;

        for (FrameGraphEdge& ingoingEdge : currentRenderPass->ingoingEdges) {
            if (visitedEdges.find(ingoingEdge) != visitedEdges.end()) {
                continue;
            }
            edgeQueue.push_back(firstEdge);
            visitedEdges.insert(ingoingEdge);
        }
    }

    // Compute all image/buffer memory dependencies.
    for (size_t i = 0; i < linearizedRenderPasses.size(); i++) {
        RenderPass* renderPass = linearizedRenderPasses.at(i);
        RenderData* renderData;
        GraphicsPipelinePtr graphicsPipeline = renderPass->getGraphicsPipeline();


        renderData->getFrameData(frameIdx);
    }

    dirty = false;
}

void FrameGraph::render() {
    if (dirty) {
        _build();
    }

    for (size_t i = 0; i < linearizedRenderPasses.size(); i++) {
        RenderPass* renderPass = linearizedRenderPasses.at(i);
        renderPass->_render();

        size_t numImageMemoryBarriers = 0;
        VkImageMemoryBarrier* imageMemoryBarriers = nullptr;
        size_t numBufferMemoryBarriers = 0;
        VkBufferMemoryBarrier* bufferMemoryBarriers = nullptr;

        auto imageDependenciesIt = imageDependenciesMap.find(i);
        auto bufferDependenciesIt = bufferDependenciesMap.find(i);
        bool hasImageDependency = imageDependenciesIt != imageDependenciesMap.end();
        bool hasBufferDependency = bufferDependenciesIt != bufferDependenciesMap.end();
        if (hasImageDependency) {
            numImageMemoryBarriers = imageDependenciesIt->second.size();
            imageMemoryBarriers = imageDependenciesIt->second.data();
        }
        if (hasBufferDependency) {
            numBufferMemoryBarriers = bufferDependenciesIt->second.size();
            bufferMemoryBarriers = bufferDependenciesIt->second.data();
        }
        vkCmdPipelineBarrier(
                renderer->getVkCommandBuffer(),
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr,
                numBufferMemoryBarriers, bufferMemoryBarriers, numImageMemoryBarriers, imageMemoryBarriers);
    }
}
*/
} }
