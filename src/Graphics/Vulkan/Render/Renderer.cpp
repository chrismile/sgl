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
#include "../Utils/Device.hpp"
#include "../Utils/Swapchain.hpp"
#include "../Render/GraphicsPipeline.hpp"
#include "../Buffers/Buffer.hpp"
#include "Data.hpp"
#include "Renderer.hpp"

namespace sgl { namespace vk {

Renderer::Renderer(Device* device) : device(device) {
}

Renderer::~Renderer() {
}

void Renderer::beginCommandBuffer() {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    frameIndex = swapchain->getImageIndex();
    if (frameCaches.size() != swapchain->getNumImages()) {
        frameCaches.resize(swapchain->getNumImages());
    }
    frameCaches.at(frameIndex).freeCameraMatrixBuffers = frameCaches.at(frameIndex).allCameraMatrixBuffers;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Renderer::beginCommandBuffer: Could not begin recording a command buffer.");
    }
}

VkCommandBuffer Renderer::endCommandBuffer() {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Renderer::beginCommandBuffer: Could not record a command buffer.");
    }

    return commandBuffer;
}

void Renderer::render(RasterDataPtr rasterData) {
    bool isNewPipeline = false;
    GraphicsPipelinePtr rasterDataGraphicsPipeline = rasterData->getGraphicsPipeline();
    if (graphicsPipeline != rasterDataGraphicsPipeline) {
        graphicsPipeline = rasterDataGraphicsPipeline;
        isNewPipeline = true;
    }

    const FramebufferPtr& framebuffer = graphicsPipeline->getFramebuffer();

    updateMatrixBlock();
    // Use: currentMatrixBlockBuffer


    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = framebuffer->getVkRenderPass();
    renderPassBeginInfo.framebuffer = framebuffer->getVkFramebuffer();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = framebuffer->getExtent2D();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDepthStencil;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (isNewPipeline) {
        vkCmdBindPipeline(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline->getVkPipeline());
    }

    std::vector<VkBuffer> vertexBuffers = rasterData->getVkVertexBuffers();
    VkDeviceSize offsets[] = { 0 };
    if (rasterData->hasIndexBuffer()) {
        VkBuffer indexBuffer = rasterData->getVkIndexBuffer();
        // VK_INDEX_TYPE_UINT32
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, rasterData->getIndexType());
    }
    vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertexBuffers.size()), vertexBuffers.data(), offsets);

    vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getVkPipelineLayout(),
            0, 1, &matrixBlockDescriptorSet, 0, nullptr);

    //std::vector<VkDescriptorSet>& dataDescriptorSets = rasterData->getVkDescriptorSets();
    //vkCmdBindDescriptorSets(
    //        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getVkPipelineLayout(),
    //        1, uint32_t(dataDescriptorSets.size()), dataDescriptorSets.data(), 0, nullptr);

    if (rasterData->hasIndexBuffer()) {
        vkCmdDrawIndexed(
                commandBuffer, static_cast<uint32_t>(rasterData->getNumIndices()),
                static_cast<uint32_t>(rasterData->getNumInstances()), 0, 0, 0);
    } else {
        vkCmdDraw(
                commandBuffer, static_cast<uint32_t>(rasterData->getNumVertices()),
                static_cast<uint32_t>(rasterData->getNumInstances()), 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::setModelMatrix(const glm::mat4 &matrix) {
    matrixBlock.mMatrix = matrix;
    matrixBlockNeedsUpdate = true;
}

void Renderer::setViewMatrix(const glm::mat4 &matrix) {
    matrixBlock.vMatrix = matrix;
    matrixBlockNeedsUpdate = true;
}

void Renderer::setProjectionMatrix(const glm::mat4 &matrix) {
    matrixBlock.pMatrix = matrix;
    matrixBlockNeedsUpdate = true;
}

void Renderer::updateMatrixBlock() {
    if (matrixBlockNeedsUpdate) {
        matrixBlock.mvpMatrix = matrixBlock.pMatrix * matrixBlock.vMatrix * matrixBlock.mMatrix;
        if (frameCaches.at(frameIndex).freeCameraMatrixBuffers.is_empty()) {
            BufferPtr buffer(new Buffer(
                    device, sizeof(MatrixBlock), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU));
            frameCaches.at(frameIndex).allCameraMatrixBuffers.push_back(buffer);
            frameCaches.at(frameIndex).freeCameraMatrixBuffers.push_back(buffer);
        }
        currentMatrixBlockBuffer = frameCaches.at(frameIndex).freeCameraMatrixBuffers.pop_front();

        void* bufferMemory = currentMatrixBlockBuffer->mapMemory();
        memcpy(bufferMemory, &matrixBlock, sizeof(MatrixBlock));
        currentMatrixBlockBuffer->unmapMemory();

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = currentMatrixBlockBuffer->getVkBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MatrixBlock);

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = matrixBlockDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(
                device->getVkDevice(), 1, &descriptorWrite,
                0, nullptr);

        matrixBlockNeedsUpdate = false;
    }
}

}}
