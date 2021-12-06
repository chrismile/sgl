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

#include "CommandBuffer.hpp"

namespace sgl { namespace vk {

CommandBuffer::CommandBuffer(Device* device, sgl::vk::CommandPoolType commandPoolType) : device(device) {
    commandBuffer = device->allocateCommandBuffer(commandPoolType, &commandPool);
}

CommandBuffer::CommandBuffer(VkCommandBuffer commandBuffer) : commandBuffer(commandBuffer) {
}

CommandBuffer::~CommandBuffer() {
    if (device) {
        vkFreeCommandBuffers(device->getVkDevice(), commandPool, 1, &commandBuffer);
    }
}

void CommandBuffer::_clearSyncObjects() {
    waitSemaphores.clear();
    waitSemaphoresVk.clear();
    waitDstStageMasks.clear();
    signalSemaphores.clear();
    signalSemaphoresVk.clear();
    fence = {};
}

void CommandBuffer::pushWaitSemaphore(const sgl::vk::SemaphorePtr& semaphore, VkPipelineStageFlags waitStage) {
    waitSemaphores.push_back(semaphore);
    waitSemaphoresVk.push_back(semaphore->getVkSemaphore());
    waitDstStageMasks.push_back(waitStage);
}

void CommandBuffer::pushSignalSemaphore(const sgl::vk::SemaphorePtr& semaphore) {
    signalSemaphores.push_back(semaphore);
    signalSemaphoresVk.push_back(semaphore->getVkSemaphore());
}

void CommandBuffer::pushWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage) {
    waitSemaphores.push_back({});
    waitSemaphoresVk.push_back(semaphore);
    waitDstStageMasks.push_back(waitStage);
}

void CommandBuffer::pushSignalSemaphore(VkSemaphore semaphore) {
    signalSemaphores.push_back({});
    signalSemaphoresVk.push_back(semaphore);
}

void CommandBuffer::popWaitSemaphore() {
    waitSemaphores.pop_back();
    waitSemaphoresVk.pop_back();
    waitDstStageMasks.pop_back();
}

void CommandBuffer::popSignalSemaphore() {
    signalSemaphores.pop_back();
    signalSemaphoresVk.pop_back();
}

bool CommandBuffer::hasWaitTimelineSemaphore() {
    bool hasWaitTimelineSemaphore = false;
    for (sgl::vk::SemaphorePtr& waitSemaphore : waitSemaphores) {
        if (waitSemaphore && waitSemaphore->isTimelineSemaphore()) {
            hasWaitTimelineSemaphore = true;
            break;
        }
    }
    return hasWaitTimelineSemaphore;
}

bool CommandBuffer::hasSignalTimelineSemaphore() {
    bool hasSignalTimelineSemaphore = false;
    for (sgl::vk::SemaphorePtr& signalSemaphore : signalSemaphores) {
        if (signalSemaphore && signalSemaphore->isTimelineSemaphore()) {
            hasSignalTimelineSemaphore = true;
            break;
        }
    }
    return hasSignalTimelineSemaphore;
}

std::vector<uint64_t> CommandBuffer::getWaitSemaphoreValues() {
    std::vector<uint64_t> values;
    values.reserve(waitSemaphores.size());
    for (sgl::vk::SemaphorePtr& semaphore : waitSemaphores) {
        values.push_back(semaphore ? semaphore->getWaitSemaphoreValue() : 0);
    }
    return values;
}

std::vector<uint64_t> CommandBuffer::getSignalSemaphoreValues() {
    std::vector<uint64_t> values;
    values.reserve(signalSemaphores.size());
    for (sgl::vk::SemaphorePtr& semaphore : signalSemaphores) {
        values.push_back(semaphore ? semaphore->getSignalSemaphoreValue() : 0);
    }
    return values;
}

}}
