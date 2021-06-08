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

#ifndef SGL_DATA_HPP
#define SGL_DATA_HPP

#include <memory>
#include <vulkan/vulkan.h>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>

#include "../Shader/Shader.hpp"
#include "../Buffers/Buffer.hpp"

namespace sgl { namespace vk {

class GraphicsPipeline;
typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;
class ComputePipeline;
typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;
class RayTracingPipeline;
typedef std::shared_ptr<RayTracingPipeline> RayTracingPipelinePtr;

class Data {
public:
    virtual ~Data() {}
};

class ComputeData : public Data {
public:
    ComputeData(ComputePipelinePtr& computePipeline);

    inline ComputePipelinePtr getComputePipeline() { return computePipeline; }

protected:
    ComputePipelinePtr computePipeline;
};

class RenderData : public Data {
public:
    RenderData(Device* device, ShaderStagesPtr& shaderStages);

    void onSwapchainRecreated();

    /*
     * The content of static buffers can, if at all, only be updated on the CPU when vkQueueWaitIdle was called on the
     * queue. They should be used, e.g., for look-up tables or when they are only used exclusively by the GPU.
     */
    BufferPtr addStaticBuffer(size_t sizeInBytes, VkBufferUsageFlags bufferUsageFlags, uint32_t binding) {
        BufferPtr buffer(new Buffer(device, sizeInBytes, bufferUsageFlags, VMA_MEMORY_USAGE_GPU_ONLY));
        if (staticBuffers.size() <= binding) {
            staticBuffers.resize(binding + 1);
        }
        staticBuffers.at(binding) = buffer;
    }
    BufferPtr addStaticBuffer(size_t sizeInBytes, void* data, VkBufferUsageFlags bufferUsageFlags, uint32_t binding) {
        BufferPtr buffer(new Buffer(device, sizeInBytes, bufferUsageFlags, VMA_MEMORY_USAGE_GPU_ONLY));
        if (staticBuffers.size() <= binding) {
            staticBuffers.resize(binding + 1);
        }
        staticBuffers.at(binding) = buffer;
    }

    /*
     * Dynamic buffers change per frame. After adding the buffer, the per-frame buffer needs to be retrieved by calling
     * getDynamicBuffer.
     */
    void addDynamicBuffer(size_t sizeInBytes, VkBufferUsageFlags bufferUsageFlags, uint32_t binding) {
        Swapchain* swapchain = AppSettings::get()->getSwapchain();
        size_t numImages = swapchain ? swapchain->getNumImages() : 1;
        const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByBinding(0, binding);
        for (size_t i = 0; i < numImages; i++) {
            if (dynamicBuffers.at(i).size() <= binding) {
                dynamicBuffers.at(i).resize(binding + 1);
                dynamicBuffersSettings.resize(binding + 1);
            }
            BufferPtr buffer(new Buffer(
                    device, sizeInBytes, descriptorInfo.type, VMA_MEMORY_USAGE_CPU_TO_GPU));
            dynamicBuffers.at(i).at(binding) = buffer;
        }
    }

    BufferPtr getDynamicBuffer(uint32_t binding) {
        Swapchain* swapchain = AppSettings::get()->getSwapchain();
        return dynamicBuffers.at(swapchain ? swapchain->getImageIndex() : 0).at(binding);
    }
    BufferPtr getDynamicBuffer(const std::string& name) {
        Swapchain* swapchain = AppSettings::get()->getSwapchain();
        const DescriptorInfo& descriptorInfo = shaderStages->getDescriptorInfoByName(0, name);
        return dynamicBuffers.at(swapchain ? swapchain->getImageIndex() : 0).at(descriptorInfo.binding);
    }

private:
    Device* device;
    ShaderStagesPtr shaderStages;

    std::vector<BufferPtr> staticBuffers;
    std::vector<std::vector<BufferPtr>> dynamicBuffers;

    struct BufferSettings {
        size_t sizeInBytes;
        VkBufferUsageFlags bufferUsageFlags;
    };
    std::vector<BufferSettings> dynamicBuffersSettings;

    // Descriptor set for buffers changing, if at all, only when vkQueueWaitIdle was called on the queue.
    VkDescriptorSet staticDescriptorSet;
    // Descriptor sets for buffers changing per frame (one decriptor set per frame).
    std::vector<VkDescriptorSet> dynamicDescriptorSet;
};

class RasterData : public RenderData {
public:
    RasterData(GraphicsPipelinePtr& graphicsPipeline);
    void setIndexBuffer(BufferPtr& buffer, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void setVertexBuffer(BufferPtr& buffer, uint32_t binding);
    void setVertexBuffer(BufferPtr& buffer, const std::string& name);

    inline void setNumInstances(size_t numInstances) { this->numInstances = numInstances; }
    inline size_t getNumInstances() { return numInstances; }

    inline bool hasIndexBuffer() const { return indexBuffer.get(); }
    inline size_t getNumIndices() const { return numIndices; }
    inline VkIndexType getIndexType() const { return indexType; }
    inline VkBuffer getVkIndexBuffer() { return indexBuffer->getVkBuffer(); }

    inline size_t getNumVertices() const { return numVertices; }
    inline const std::vector<VkBuffer>& getVkVertexBuffers() { return vulkanVertexBuffers; }

    inline GraphicsPipelinePtr getGraphicsPipeline() { return graphicsPipeline; }

protected:
    GraphicsPipelinePtr graphicsPipeline;

    size_t numInstances = 1;

    BufferPtr indexBuffer;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    size_t numIndices = 0;

    std::vector<BufferPtr> vertexBuffers;
    size_t numVertices = 0;
    std::vector<VkBuffer> vulkanVertexBuffers;
};

class RayTracingData : public RenderData {
public:
    RayTracingData(RayTracingPipelinePtr& rayTracingPipeline);

    inline RayTracingPipelinePtr getRayTracingPipeline() { return rayTracingPipeline; }

protected:
    RayTracingPipelinePtr rayTracingPipeline;
};

typedef std::shared_ptr<ComputeData> ComputeDataPtr;
typedef std::shared_ptr<RasterData> RasterDataPtr;
typedef std::shared_ptr<RayTracingData> RayTracingDataPtr;

}}

#endif //SGL_DATA_HPP
