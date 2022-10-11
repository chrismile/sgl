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
#include "../Utils/SyncObjects.hpp"
#include "../Render/GraphicsPipeline.hpp"
#include "../Render/ComputePipeline.hpp"
#include "../Render/RayTracingPipeline.hpp"
#include "../Buffers/Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Data.hpp"
#include "Renderer.hpp"

namespace sgl { namespace vk {

Renderer::Renderer(Device* device, uint32_t numDescriptors) : device(device) {
    std::vector<VkDescriptorPoolSize> globalPoolSizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, numDescriptors },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numDescriptors },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, numDescriptors },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numDescriptors },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, numDescriptors },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, numDescriptors },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numDescriptors },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, numDescriptors },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, numDescriptors },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, numDescriptors },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, numDescriptors }
    };

#if VK_VERSION_1_2 && VK_HEADER_VERSION >= 162
    if (device->isDeviceExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) {
        globalPoolSizes.push_back({ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, numDescriptors });
    }
#endif

    VkDescriptorPoolCreateInfo globalPoolInfo = {};
    globalPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    globalPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    globalPoolInfo.poolSizeCount = uint32_t(globalPoolSizes.size());
    globalPoolInfo.pPoolSizes = globalPoolSizes.data();
    globalPoolInfo.maxSets = numDescriptors;

    if (vkCreateDescriptorPool(
            device->getVkDevice(), &globalPoolInfo, nullptr, &globalDescriptorPool) != VK_SUCCESS) {
        Logfile::get()->throwError("Error in Renderer::Renderer: Failed to create global descriptor pool!");
    }


    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    if (device->getPhysicalDeviceFeatures().geometryShader) {
        uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    if (device->getPhysicalDeviceMeshShaderFeaturesNV().meshShader) {
        uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_MESH_BIT_NV;
    }
#ifdef VK_EXT_mesh_shader
    if (device->getPhysicalDeviceMeshShaderFeaturesEXT().meshShader) {
        uboLayoutBinding.stageFlags |= VK_SHADER_STAGE_MESH_BIT_EXT;
    }
#endif

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = 1;
    descriptorSetLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(
            device->getVkDevice(), &descriptorSetLayoutInfo, nullptr,
            &matrixBufferDesciptorSetLayout) != VK_SUCCESS) {
        Logfile::get()->throwError("Error in Renderer::Renderer: Failed to create descriptor set layout!");
    }


    VkDescriptorPoolSize poolSize;
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = maxFrameCacheSize;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = maxFrameCacheSize;

    if (vkCreateDescriptorPool(
            device->getVkDevice(), &poolInfo, nullptr, &matrixBufferDescriptorPool) != VK_SUCCESS) {
        Logfile::get()->throwError("Error in Renderer::Renderer: Failed to create matrix block descriptor pool!");
    }
}

Renderer::~Renderer() {
    if (!customCommandBuffer) {
        device->waitIdle();
    }

    for (FrameCache& frameCache : frameCaches) {
        while (!frameCache.allMatrixBlockDescriptorSets.is_empty()) {
            VkDescriptorSet descriptorSet = frameCache.allMatrixBlockDescriptorSets.pop_front();
            vkFreeDescriptorSets(
                    device->getVkDevice(), matrixBufferDescriptorPool, 1, &descriptorSet);
        }
    }
    frameCaches.clear();
    frameCaches.shrink_to_fit();

    vkDestroyDescriptorSetLayout(device->getVkDevice(), matrixBufferDesciptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device->getVkDevice(), matrixBufferDescriptorPool, nullptr);
    vkDestroyDescriptorPool(device->getVkDevice(), globalDescriptorPool, nullptr);
    matrixBufferDesciptorSetLayout = {};
    matrixBufferDescriptorPool = {};
    globalDescriptorPool = {};

    if (!commandBuffersVk.empty()) {
        commandBuffers.clear();
        vkFreeCommandBuffers(
                device->getVkDevice(), commandPool,
                uint32_t(commandBuffersVk.size()), commandBuffersVk.data());
        commandBuffersVk.clear();
    }
}

void Renderer::beginCommandBuffer() {
    if (customCommandBuffer == VK_NULL_HANDLE) {
        if (frameCommandBuffers.empty()) {
            Swapchain* swapchain = AppSettings::get()->getSwapchain();
            size_t numImages = swapchain ? swapchain->getNumImages() : 1;
            frameIndex = swapchain ? swapchain->getImageIndex() : 0;
            if (frameCaches.size() != numImages) {
                frameCaches.resize(numImages);

                if (!commandBuffersVk.empty()) {
                    commandBuffers.clear();
                    vkFreeCommandBuffers(
                            device->getVkDevice(), commandPool,
                            uint32_t(commandBuffersVk.size()), commandBuffersVk.data());
                    commandBuffersVk.clear();
                }
                vk::CommandPoolType commandPoolType;
                commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandBuffersVk = device->allocateCommandBuffers(
                        commandPoolType, &commandPool, uint32_t(numImages));
                for (size_t frameIdx = 0; frameIdx < numImages; frameIdx++) {
                    commandBuffers.push_back(std::make_shared<sgl::vk::CommandBuffer>(
                            commandBuffersVk.at(frameIdx)));
                }
            }
            frameCaches.at(frameIndex).freeCameraMatrixBuffers =
                    frameCaches.at(frameIndex).allCameraMatrixBuffers;
            frameCaches.at(frameIndex).freeMatrixBlockDescriptorSets =
                    frameCaches.at(frameIndex).allMatrixBlockDescriptorSets;

            auto commandBufferPtr = commandBuffers.at(frameIndex);
            frameCommandBuffers.push_back(commandBufferPtr);
            commandBuffer = commandBufferPtr->getVkCommandBuffer();

            if (swapchain) {
                commandBufferPtr->pushWaitSemaphore(
                        swapchain->getImageAvailableSemaphores()[swapchain->getCurrentFrame()]);
            }
        }
    } else {
        frameIndex = 0;
        commandBuffer = customCommandBuffer;
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Renderer::beginCommandBuffer: Could not begin recording a command buffer.");
    }

    recordingCommandBufferStarted = true;
    isCommandBufferInRecordingState = true;
}

VkCommandBuffer Renderer::endCommandBuffer() {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Renderer::beginCommandBuffer: Could not record a command buffer.");
    }
    graphicsPipeline = GraphicsPipelinePtr();
    computePipeline = ComputePipelinePtr();
    rayTracingPipeline = RayTracingPipelinePtr();
    lastFramebuffer = FramebufferPtr();
    isCommandBufferInRecordingState = false;

    return commandBuffer;
}

std::vector<sgl::vk::CommandBufferPtr> Renderer::getFrameCommandBuffers() {
    std::vector<sgl::vk::CommandBufferPtr> frameCommandBuffersCopy = frameCommandBuffers;
    frameCommandBuffers = {};
    return frameCommandBuffersCopy;
}

void Renderer::setCustomCommandBuffer(VkCommandBuffer _commandBuffer, bool _useGraphicsQueue) {
    cachedCommandBuffer = commandBuffer;
    cachedUseGraphicsQueue = useGraphicsQueue;
    cachedRecordingCommandBufferStarted = recordingCommandBufferStarted;
    cachedIsCommandBufferInRecordingState = isCommandBufferInRecordingState;
    customCommandBuffer = _commandBuffer;
    useGraphicsQueue = _useGraphicsQueue;
}

void Renderer::resetCustomCommandBuffer() {
    commandBuffer = cachedCommandBuffer;
    recordingCommandBufferStarted = cachedRecordingCommandBufferStarted;
    isCommandBufferInRecordingState = cachedIsCommandBufferInRecordingState;
    useGraphicsQueue = cachedUseGraphicsQueue;
    cachedUseGraphicsQueue = true;
    customCommandBuffer = VK_NULL_HANDLE;
    cachedCommandBuffer = VK_NULL_HANDLE;
    cachedRecordingCommandBufferStarted = false;
    cachedIsCommandBufferInRecordingState = false;
}

void Renderer::pushCommandBuffer(const sgl::vk::CommandBufferPtr& commandBuffer) {
    frameCommandBuffers.push_back(commandBuffer);
    this->commandBuffer = commandBuffer->getVkCommandBuffer();
}

void Renderer::syncWithCpu() {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Renderer::beginCommandBuffer: Could not record a command buffer.");
    }

    submitToQueue();
    if (useGraphicsQueue) {
        device->waitGraphicsQueueIdle();
    } else {
        device->waitComputeQueueIdle();
    }

    auto commandBufferPtr = commandBuffers.at(frameIndex);
    frameCommandBuffers.push_back(commandBufferPtr);
    commandBuffer = commandBufferPtr->getVkCommandBuffer();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Renderer::beginCommandBuffer: Could not begin recording a command buffer.");
    }
}


void Renderer::render(const RasterDataPtr& rasterData) {
    const FramebufferPtr& framebuffer = rasterData->getGraphicsPipeline()->getFramebuffer();
    render(rasterData, framebuffer);
}

void Renderer::render(const RasterDataPtr& rasterData, const FramebufferPtr& framebuffer) {
    bool isNewPipeline = false;
    GraphicsPipelinePtr newGraphicsPipeline = rasterData->getGraphicsPipeline();
    if (graphicsPipeline != newGraphicsPipeline) {
        graphicsPipeline = newGraphicsPipeline;
        isNewPipeline = true;
    }

    uint32_t numSubpasses = framebuffer->getNumSubpasses();
    uint32_t subpassIndex = newGraphicsPipeline->getSubpassIndex();
    bool isFirstSubpass = subpassIndex == 0;
    bool isLastSubpass = subpassIndex == numSubpasses - 1;

    if (numSubpasses > 1 && !isFirstSubpass && !isLastSubpass && lastFramebuffer != framebuffer) {
        Logfile::get()->throwError(
                "Error in Renderer::render: The used framebuffer changed before having reached the last subpass!");
    }
    if (subpassIndex >= numSubpasses) {
        Logfile::get()->throwError(
                "Error in Renderer::render: subpassIndex >= numSubpasses!");
    }

    if (updateMatrixBlock() || recordingCommandBufferStarted) {
        vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getVkPipelineLayout(),
                1, 1, &matrixBlockDescriptorSet, 0, nullptr);
        recordingCommandBufferStarted = false;
    }

    if (isFirstSubpass) {
        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = framebuffer->getVkRenderPass();
        renderPassBeginInfo.framebuffer = framebuffer->getVkFramebuffer();
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = framebuffer->getExtent2D();
        if (framebuffer->getUseClear()) {
            const std::vector<VkClearValue>& clearValues = framebuffer->getVkClearValues();
            renderPassBeginInfo.clearValueCount = uint32_t(clearValues.size());
            renderPassBeginInfo.pClearValues = clearValues.data();
        }

        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    if (!isFirstSubpass && numSubpasses > 1) {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    }

    if (isNewPipeline) {
        vkCmdBindPipeline(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline->getVkPipeline());
    }

    if (rasterData->getHasIndexBuffer()) {
        VkBuffer indexBuffer = rasterData->getVkIndexBuffer();
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, rasterData->getIndexType());
    }

    std::vector<VkBuffer> vertexBuffers = rasterData->getVkVertexBuffers();
    if (!vertexBuffers.empty()) {
        std::vector<VkDeviceSize> offsets;
        offsets.resize(vertexBuffers.size(), 0);
        vkCmdBindVertexBuffers(commandBuffer, 0, uint32_t(vertexBuffers.size()), vertexBuffers.data(), offsets.data());
    }

    rasterData->_updateDescriptorSets();
    VkDescriptorSet descriptorSet = rasterData->getVkDescriptorSet();
    if (descriptorSet != VK_NULL_HANDLE) {
        if (graphicsPipeline->getShaderStages()->getVkDescriptorSetLayouts().size() == 1) {
            vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline->getVkPipelineLayout(),
                    0, 1, &descriptorSet, 0, nullptr);
        } else {
            VkDescriptorSet descriptorSets[2];
            descriptorSets[0] = descriptorSet;
            descriptorSets[1] = matrixBlockDescriptorSet;
            vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline->getVkPipelineLayout(),
                    0, 2, descriptorSets, 0, nullptr);
        }
    } else {
        vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->getVkPipelineLayout(),
                1, 1, &matrixBlockDescriptorSet, 0, nullptr);
    }

    if (graphicsPipeline->getShaderStages()->getHasVertexShader()) {
        if (rasterData->getUseIndirectDraw()) {
            if (rasterData->getUseIndirectDrawCount()) {
                if (rasterData->getHasIndexBuffer()) {
                    vkCmdDrawIndexedIndirectCount(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCountBufferVk(),
                            rasterData->getIndirectDrawCountBufferOffset(),
                            rasterData->getIndirectMaxDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                } else {
                    vkCmdDrawIndirectCount(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCountBufferVk(),
                            rasterData->getIndirectDrawCountBufferOffset(),
                            rasterData->getIndirectMaxDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                }
            } else {
                if (rasterData->getHasIndexBuffer()) {
                    vkCmdDrawIndexedIndirect(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                } else {
                    vkCmdDrawIndirect(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                }
            }
        } else if (rasterData->getHasIndexBuffer()) {
            vkCmdDrawIndexed(
                    commandBuffer, static_cast<uint32_t>(rasterData->getNumIndices()),
                    static_cast<uint32_t>(rasterData->getNumInstances()), 0, 0, 0);
        } else {
            vkCmdDraw(
                    commandBuffer, static_cast<uint32_t>(rasterData->getNumVertices()),
                    static_cast<uint32_t>(rasterData->getNumInstances()), 0, 0);
        }
    } else if (rasterData->getTaskCountNV() > 0) {
        if (graphicsPipeline->getShaderStages()->getHasMeshShaderNV()) {
            if (rasterData->getUseIndirectDraw()) {
                if (rasterData->getUseIndirectDrawCount()) {
                    vkCmdDrawMeshTasksIndirectCountNV(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCountBufferVk(),
                            rasterData->getIndirectDrawCountBufferOffset(),
                            rasterData->getIndirectMaxDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                } else {
                    vkCmdDrawMeshTasksIndirectNV(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                }
            } else {
                /**
                 * Assuming task/mesh shaders. The maximum number of tasks is relatively low on NVIDIA hardware, so
                 * split into multiple draw calls if necessary.
                 */
                uint32_t firstTask = rasterData->getFirstTaskNV();
                uint32_t taskCount = rasterData->getTaskCountNV();
                uint32_t taskCountMax = device->getPhysicalDeviceMeshShaderPropertiesNV().maxDrawMeshTasksCount;
                while (taskCount > taskCountMax) {
                    vkCmdDrawMeshTasksNV(commandBuffer, taskCountMax, firstTask);
                    firstTask += taskCountMax;
                    taskCount -= taskCountMax;
                }
                vkCmdDrawMeshTasksNV(commandBuffer, taskCount, firstTask);
            }
        }
#ifdef VK_EXT_mesh_shader
        else if (graphicsPipeline->getShaderStages()->getHasMeshShaderEXT()) {
            if (rasterData->getUseIndirectDraw()) {
                if (rasterData->getUseIndirectDrawCount()) {
                    vkCmdDrawMeshTasksIndirectCountEXT(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCountBufferVk(),
                            rasterData->getIndirectDrawCountBufferOffset(),
                            rasterData->getIndirectMaxDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                } else {
                    vkCmdDrawMeshTasksIndirectEXT(
                            commandBuffer, rasterData->getIndirectDrawBufferVk(),
                            rasterData->getIndirectDrawBufferOffset(),
                            rasterData->getIndirectDrawCount(),
                            rasterData->getIndirectDrawBufferStride());
                }
            } else {
                uint32_t groupCountX = rasterData->getMeshTasksGroupCountX();
                uint32_t groupCountY = rasterData->getMeshTasksGroupCountY();
                uint32_t groupCountZ = rasterData->getMeshTasksGroupCountZ();
                vkCmdDrawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ);
            }
        }
#endif
        else {
            sgl::Logfile::get()->throwError("Error in Renderer::render: Task count > 0, but no mesh shader set.");
        }
    }

    // Transition the image layouts.
    framebuffer->transitionAttachmentImageLayouts(subpassIndex);

    if (isLastSubpass) {
        vkCmdEndRenderPass(commandBuffer);
    }
    lastFramebuffer = framebuffer;
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

bool Renderer::updateMatrixBlock() {
    if (matrixBlockNeedsUpdate) {
        matrixBlock.mvpMatrix = matrixBlock.pMatrix * matrixBlock.vMatrix * matrixBlock.mMatrix;
        if (frameCaches.at(frameIndex).freeCameraMatrixBuffers.is_empty()) {
            BufferPtr buffer(new Buffer(
                    device, sizeof(MatrixBlock), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU));
            frameCaches.at(frameIndex).allCameraMatrixBuffers.push_back(buffer);
            frameCaches.at(frameIndex).freeCameraMatrixBuffers.push_back(buffer);

            VkDescriptorSet descriptorSet;
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = matrixBufferDescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &matrixBufferDesciptorSetLayout;

            if (vkAllocateDescriptorSets(device->getVkDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
                Logfile::get()->throwError(
                        "Error in Renderer::updateMatrixBlock: Failed to allocate descriptor sets!");
            }

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = buffer->getVkBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(MatrixBlock);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(
                    device->getVkDevice(), 1, &descriptorWrite,
                    0, nullptr);

            frameCaches.at(frameIndex).allMatrixBlockDescriptorSets.push_back(descriptorSet);
            frameCaches.at(frameIndex).freeMatrixBlockDescriptorSets.push_back(descriptorSet);
        }
        currentMatrixBlockBuffer = frameCaches.at(frameIndex).freeCameraMatrixBuffers.pop_front();
        matrixBlockDescriptorSet = frameCaches.at(frameIndex).freeMatrixBlockDescriptorSets.pop_front();

        void* bufferMemory = currentMatrixBlockBuffer->mapMemory();
        memcpy(bufferMemory, &matrixBlock, sizeof(MatrixBlock));
        currentMatrixBlockBuffer->unmapMemory();

        matrixBlockNeedsUpdate = false;
        return true;
    }
    return false;
}

void Renderer::dispatch(
        const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    bool isNewPipeline = false;
    ComputePipelinePtr newComputePipeline = computeData->getComputePipeline();
    if (computePipeline != newComputePipeline) {
        computePipeline = newComputePipeline;
        isNewPipeline = true;
    }

    if (isNewPipeline) {
        vkCmdBindPipeline(
                commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                computePipeline->getVkPipeline());
    }

    computeData->_updateDescriptorSets();
    VkDescriptorSet descriptorSet = computeData->getVkDescriptorSet();
    if (descriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                computePipeline->getVkPipelineLayout(),
                0, 1, &descriptorSet, 0, nullptr);
    }

    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void Renderer::dispatchIndirect(
        const ComputeDataPtr& computeData, const sgl::vk::BufferPtr& dispatchIndirectBuffer, VkDeviceSize offset) {
    bool isNewPipeline = false;
    ComputePipelinePtr newComputePipeline = computeData->getComputePipeline();
    if (computePipeline != newComputePipeline) {
        computePipeline = newComputePipeline;
        isNewPipeline = true;
    }

    if (isNewPipeline) {
        vkCmdBindPipeline(
                commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                computePipeline->getVkPipeline());
    }

    computeData->_updateDescriptorSets();
    VkDescriptorSet descriptorSet = computeData->getVkDescriptorSet();
    if (descriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                computePipeline->getVkPipelineLayout(),
                0, 1, &descriptorSet, 0, nullptr);
    }

    vkCmdDispatchIndirect(commandBuffer, dispatchIndirectBuffer->getVkBuffer(), offset);
}

void Renderer::dispatchIndirect(
        const ComputeDataPtr& computeData, const sgl::vk::BufferPtr& dispatchIndirectBuffer) {
    dispatchIndirect(computeData, dispatchIndirectBuffer, 0);
}

void Renderer::traceRays(
        const RayTracingDataPtr& rayTracingData,
        uint32_t launchSizeX, uint32_t launchSizeY, uint32_t launchSizeZ) {
    bool isNewPipeline = false;
    RayTracingPipelinePtr newRayTracingPipeline = rayTracingData->getRayTracingPipeline();
    if (rayTracingPipeline != newRayTracingPipeline) {
        rayTracingPipeline = newRayTracingPipeline;
        isNewPipeline = true;
    }

    if (isNewPipeline) {
        vkCmdBindPipeline(
                commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                rayTracingPipeline->getVkPipeline());
    }

    rayTracingData->_updateDescriptorSets();
    VkDescriptorSet descriptorSet = rayTracingData->getVkDescriptorSet();
    if (descriptorSet != VK_NULL_HANDLE) {
        if (rayTracingPipeline->getShaderStages()->getVkDescriptorSetLayouts().size() == 1) {
            vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    rayTracingPipeline->getVkPipelineLayout(),
                    0, 1, &descriptorSet, 0, nullptr);
        }
    }

    const std::array<VkStridedDeviceAddressRegionKHR, 4>& stridedDeviceAddressRegions =
            rayTracingData->getStridedDeviceAddressRegions();
    const VkStridedDeviceAddressRegionKHR& raygenShaderBindingTable = stridedDeviceAddressRegions.at(0);
    const VkStridedDeviceAddressRegionKHR& missShaderBindingTable = stridedDeviceAddressRegions.at(1);
    const VkStridedDeviceAddressRegionKHR& hitShaderBindingTable = stridedDeviceAddressRegions.at(2);
    const VkStridedDeviceAddressRegionKHR& callableShaderBindingTable = stridedDeviceAddressRegions.at(3);
    vkCmdTraceRaysKHR(
            commandBuffer,
            &raygenShaderBindingTable, &missShaderBindingTable, &hitShaderBindingTable, &callableShaderBindingTable,
            launchSizeX, launchSizeY, launchSizeZ);
}

void Renderer::transitionImageLayout(vk::ImagePtr& image, VkImageLayout newLayout) {
    image->transitionImageLayout(newLayout, commandBuffer);
}

void Renderer::transitionImageLayout(vk::ImageViewPtr& imageView, VkImageLayout newLayout) {
    imageView->transitionImageLayout(newLayout, commandBuffer);
}

void Renderer::transitionImageLayoutSubresource(
        vk::ImagePtr& image, VkImageLayout newLayout,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    image->transitionImageLayoutSubresource(
            newLayout, commandBuffer, baseMipLevel, levelCount, baseArrayLayer, layerCount);
}

void Renderer::insertImageMemoryBarrier(
        const vk::ImagePtr& image, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
    image->insertMemoryBarrier(commandBuffer, oldLayout, newLayout, srcStage, dstStage, srcAccessMask, dstAccessMask);
}

void Renderer::insertImageMemoryBarriers(
        const std::vector<vk::ImagePtr>& images, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    std::vector<VkImageMemoryBarrier> barriers(images.size());
    for (size_t i = 0; i < images.size(); i++) {
        const ImagePtr& image = images.at(i);
        barrier.image = image->getVkImage();
        barrier.subresourceRange.levelCount = image->getImageSettings().mipLevels;
        barrier.subresourceRange.layerCount = image->getImageSettings().arrayLayers;
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(image->getImageSettings().format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        barriers.at(i) = barrier;
        image->imageLayout = newLayout;
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,
            0, // 0 or VK_DEPENDENCY_BY_REGION_BIT
            0, nullptr,
            0, nullptr,
            uint32_t(images.size()), barriers.data()
    );
}

void Renderer::insertImageMemoryBarrier(
        const vk::ImageViewPtr& imageView, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
    imageView->insertMemoryBarrier(
            commandBuffer, oldLayout, newLayout, srcStage, dstStage, srcAccessMask, dstAccessMask);
}

void Renderer::insertImageMemoryBarrierSubresource(
        const vk::ImagePtr& image, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    image->insertMemoryBarrierSubresource(
            commandBuffer, oldLayout, newLayout, srcStage, dstStage, srcAccessMask, dstAccessMask,
            baseMipLevel, levelCount, baseArrayLayer, layerCount);
}

void Renderer::pushConstants(
        const PipelinePtr& pipeline, VkShaderStageFlagBits shaderStageFlagBits,
        uint32_t offset, uint32_t size, const void* data) {
    vkCmdPushConstants(commandBuffer, pipeline->getVkPipelineLayout(), shaderStageFlagBits, offset, size, data);
}

void Renderer::resolveImage(const sgl::vk::ImageViewPtr& sourceImage, const sgl::vk::ImageViewPtr& destImage) {
    const auto& sourceImageSettings = sourceImage->getImage()->getImageSettings();
    const auto& destImageSettings = destImage->getImage()->getImageSettings();

    VkImageResolve imageResolve{};
    imageResolve.extent = {
            sourceImageSettings.width, sourceImageSettings.height, sourceImageSettings.depth
    };
    imageResolve.srcSubresource.layerCount = sourceImageSettings.arrayLayers;
    imageResolve.srcSubresource.aspectMask = sourceImage->getVkImageAspectFlags();
    imageResolve.dstSubresource.layerCount = destImageSettings.arrayLayers;
    imageResolve.dstSubresource.aspectMask = destImage->getVkImageAspectFlags();

    vkCmdResolveImage(
            commandBuffer,
            sourceImage->getImage()->getVkImage(), sourceImage->getImage()->getVkImageLayout(),
            destImage->getImage()->getVkImage(), destImage->getImage()->getVkImageLayout(),
            1, &imageResolve);
}

void Renderer::insertMemoryBarrier(
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = srcAccessMask;
    memoryBarrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void Renderer::insertBufferMemoryBarrier(
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        const BufferPtr& buffer) {
    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = srcAccessMask;
    memoryBarrier.dstAccessMask = dstAccessMask;

    VkBufferMemoryBarrier bufferMemoryBarrier{};
    bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferMemoryBarrier.srcAccessMask = srcAccessMask;
    bufferMemoryBarrier.dstAccessMask = dstAccessMask;
    bufferMemoryBarrier.buffer = buffer->getVkBuffer();
    bufferMemoryBarrier.size = buffer->getSizeInBytes();

    if (!useGraphicsQueue) {
        bufferMemoryBarrier.srcQueueFamilyIndex = device->getComputeQueueIndex();
        bufferMemoryBarrier.dstQueueFamilyIndex = device->getComputeQueueIndex();
    }

    vkCmdPipelineBarrier(
            commandBuffer, srcStageMask, dstStageMask, 0, 1, &memoryBarrier, 1, &bufferMemoryBarrier, 0, nullptr);
}

void Renderer::insertBufferMemoryBarrier(
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
        const BufferPtr& buffer) {
    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = srcAccessMask;
    memoryBarrier.dstAccessMask = dstAccessMask;

    VkBufferMemoryBarrier bufferMemoryBarrier{};
    bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferMemoryBarrier.srcAccessMask = srcAccessMask;
    bufferMemoryBarrier.dstAccessMask = dstAccessMask;
    bufferMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
    bufferMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
    bufferMemoryBarrier.buffer = buffer->getVkBuffer();
    bufferMemoryBarrier.size = buffer->getSizeInBytes();

    vkCmdPipelineBarrier(
            commandBuffer, srcStageMask, dstStageMask, 0, 1, &memoryBarrier, 1, &bufferMemoryBarrier, 0, nullptr);
}


void Renderer::submitToQueue() {
    std::vector<uint64_t> waitSemaphoreValues;
    std::vector<uint64_t> signalSemaphoreValues;

    for (sgl::vk::CommandBufferPtr& frameCommandBuffer : frameCommandBuffers) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkTimelineSemaphoreSubmitInfo timelineSubmitInfo{};
        timelineSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

        if (frameCommandBuffer->hasWaitTimelineSemaphore()) {
            waitSemaphoreValues = frameCommandBuffer->getWaitSemaphoreValues();
            submitInfo.pNext = &timelineSubmitInfo;
            timelineSubmitInfo.waitSemaphoreValueCount = uint32_t(waitSemaphoreValues.size());
            timelineSubmitInfo.pWaitSemaphoreValues = waitSemaphoreValues.data();
        }
        if (frameCommandBuffer->hasSignalTimelineSemaphore()) {
            signalSemaphoreValues = frameCommandBuffer->getSignalSemaphoreValues();
            submitInfo.pNext = &timelineSubmitInfo;
            timelineSubmitInfo.signalSemaphoreValueCount = uint32_t(signalSemaphoreValues.size());
            timelineSubmitInfo.pSignalSemaphoreValues = signalSemaphoreValues.data();
        }

        submitInfo.waitSemaphoreCount = uint32_t(frameCommandBuffer->getWaitSemaphoresVk().size());
        submitInfo.pWaitSemaphores = frameCommandBuffer->getWaitSemaphoresVk().data();
        submitInfo.pWaitDstStageMask = frameCommandBuffer->getWaitDstStageMasks().data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = frameCommandBuffer->getVkCommandBufferPtr();
        submitInfo.signalSemaphoreCount = uint32_t(frameCommandBuffer->getSignalSemaphoresVk().size());
        submitInfo.pSignalSemaphores = frameCommandBuffer->getSignalSemaphoresVk().data();

        VkQueue queue = useGraphicsQueue ? device->getGraphicsQueue() : device->getComputeQueue();
        if (vkQueueSubmit(queue, 1, &submitInfo, frameCommandBuffer->getVkFence()) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Renderer::submitToQueue: Could not submit to the used queue.");
        }

        frameCommandBuffer->_clearSyncObjects();
    }

    frameCommandBuffers = {};
}

void Renderer::submitToQueue(
        const SemaphorePtr& waitSemaphore, const SemaphorePtr& signalSemaphore, const FencePtr& fence,
        VkPipelineStageFlags waitStage) {
    VkSemaphore waitSemaphoreVk = waitSemaphore ? waitSemaphore->getVkSemaphore() : VK_NULL_HANDLE;
    VkSemaphore signalSemaphoreVk = signalSemaphore ? signalSemaphore->getVkSemaphore() : VK_NULL_HANDLE;
    VkFence fenceVk = fence ? fence->getVkFence() : VK_NULL_HANDLE;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (waitSemaphoreVk) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphoreVk;
    }
    submitInfo.pWaitDstStageMask = &waitStage;
    if (signalSemaphoreVk) {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphoreVk;
    }
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue queue;
    if (device->getIsMainThread()) {
        queue = useGraphicsQueue ? device->getGraphicsQueue() : device->getComputeQueue();
    } else {
        queue = device->getWorkerThreadGraphicsQueue();
    }
    if (vkQueueSubmit(queue, 1, &submitInfo, fenceVk) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Renderer::submitToQueue: Could not submit to the used queue.");
    }
}

void Renderer::submitToQueue(
        const std::vector<SemaphorePtr>& waitSemaphores, const std::vector<SemaphorePtr>& signalSemaphores,
        const FencePtr& fence,
        const std::vector<VkPipelineStageFlags>& waitStages) {
    std::vector<VkSemaphore> waitSemaphoresVk;
    waitSemaphoresVk.reserve(waitSemaphores.size());
    for (const SemaphorePtr& semaphore : waitSemaphores) {
        waitSemaphoresVk.push_back(semaphore->getVkSemaphore());
    }

    std::vector<VkSemaphore> signalSemaphoresVk;
    signalSemaphoresVk.reserve(signalSemaphores.size());
    for (const SemaphorePtr& semaphore : signalSemaphores) {
        signalSemaphoresVk.push_back(semaphore->getVkSemaphore());
    }

    VkFence fenceVk = fence ? fence->getVkFence() : VK_NULL_HANDLE;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = uint32_t(waitSemaphoresVk.size());
    submitInfo.pWaitSemaphores = waitSemaphoresVk.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.signalSemaphoreCount = uint32_t(signalSemaphoresVk.size());
    submitInfo.pSignalSemaphores = signalSemaphoresVk.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue queue = useGraphicsQueue ? device->getGraphicsQueue() : device->getComputeQueue();
    if (vkQueueSubmit(queue, 1, &submitInfo, fenceVk) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Renderer::submitToQueue: Could not submit to the used queue.");
    }
}

void Renderer::submitToQueueImmediate() {
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VkQueue queue = useGraphicsQueue ? device->getGraphicsQueue() : device->getComputeQueue();
    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Renderer::submitToQueueImmediate: Could not submit to the used queue.");
    }
    if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError(
                "Error in Renderer::submitToQueueImmediate: vkQueueWaitIdle failed.");
    }
}

}}
