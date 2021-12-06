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

#ifndef SGL_COMMANDBUFFER_HPP
#define SGL_COMMANDBUFFER_HPP

#include <vector>
#include "../libs/volk/volk.h"
#include "../Utils/SyncObjects.hpp"

namespace sgl { namespace vk {

class DLL_OBJECT CommandBuffer {
    friend class Renderer;
    friend class Swapchain;

public:
    /**
     * Creates a command buffer from the passed command pool type.
     * The command buffer is destroyed at the end of the lifetime of the object.
     * @param device The device to use for allocating the command buffer.
     * @param commandPoolType The command pool type to allocate a command buffer from.
     */
    CommandBuffer(Device* device, sgl::vk::CommandPoolType commandPoolType);

    /**
     * Creates a command buffer from the passed Vulkan object.
     * Users of this function have to take ownership of the allocated command buffer themselves.
     * @param commandBuffer
     */
    explicit CommandBuffer(VkCommandBuffer commandBuffer);

    ~CommandBuffer();

    inline VkCommandBuffer getVkCommandBuffer() { return commandBuffer; }
    inline VkCommandBuffer* getVkCommandBufferPtr() { return &commandBuffer; }

    void pushWaitSemaphore(
            const sgl::vk::SemaphorePtr& semaphore,
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    void pushSignalSemaphore(const sgl::vk::SemaphorePtr& semaphore);
    void pushWaitSemaphore(
            VkSemaphore semaphore, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    void pushSignalSemaphore(VkSemaphore semaphore);
    void popWaitSemaphore();
    void popSignalSemaphore();
    inline void setFence(const sgl::vk::FencePtr& _fence) { this->fence = _fence; }

    bool hasWaitTimelineSemaphore();
    bool hasSignalTimelineSemaphore();
    std::vector<uint64_t> getWaitSemaphoreValues();
    std::vector<uint64_t> getSignalSemaphoreValues();

    inline std::vector<sgl::vk::SemaphorePtr>& getWaitSemaphores() { return waitSemaphores; }
    inline std::vector<sgl::vk::SemaphorePtr>& getSignalSemaphores() { return signalSemaphores; }
    inline sgl::vk::FencePtr& getFence() { return fence; }
    inline std::vector<VkSemaphore>& getWaitSemaphoresVk() { return waitSemaphoresVk; }
    inline std::vector<VkSemaphore>& getSignalSemaphoresVk() { return signalSemaphoresVk; }
    inline std::vector<VkPipelineStageFlags>& getWaitDstStageMasks() { return waitDstStageMasks; }
    inline VkFence getVkFence() { if (fence) { return fence->getVkFence(); } return VK_NULL_HANDLE; }


private:
    Device* device = nullptr;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    void _clearSyncObjects();
    std::vector<sgl::vk::SemaphorePtr> waitSemaphores;
    std::vector<VkSemaphore> waitSemaphoresVk;
    std::vector<VkPipelineStageFlags> waitDstStageMasks;
    std::vector<sgl::vk::SemaphorePtr> signalSemaphores;
    std::vector<VkSemaphore> signalSemaphoresVk;
    sgl::vk::FencePtr fence;
};

typedef std::shared_ptr<CommandBuffer> CommandBufferPtr;

}}

#endif //SGL_COMMANDBUFFER_HPP
