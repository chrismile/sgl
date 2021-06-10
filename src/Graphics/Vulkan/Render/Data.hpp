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

#include "../Shader/Shader.hpp"
#include "../Buffers/Buffer.hpp"

namespace sgl { namespace vk {

class GraphicsPipeline;
typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;
class ComputePipeline;
typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;
class RayTracingPipeline;
typedef std::shared_ptr<RayTracingPipeline> RayTracingPipelinePtr;
class Renderer;

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

class RenderData;
typedef std::shared_ptr<RenderData> RenderDataPtr;
typedef uint32_t ListenerToken;

class RenderData : public Data {
    friend class Renderer;
public:
    RenderData(Renderer* renderer, ShaderStagesPtr& shaderStages);
    ~RenderData();

    /**
     * Creates a shallow copy of the render data using the passed shader stages.
     * NOTE: The descriptor layouts of the new shaders have to match!
     * @param shaderStages The shader stages to use in the copy.
     */
    RenderDataPtr copy(ShaderStagesPtr& shaderStages);

    void onSwapchainRecreated();

    /*
     * The content of static data can only be updated on the CPU when vkQueueWaitIdle was called on the command queue.
     * They should be used, e.g., for look-up tables or for objects only used exclusively by the GPU.
     * It is recommended to create the objects using the memory usage VMA_MEMORY_USAGE_GPU_ONLY.
     */
    void addStaticBuffer(BufferPtr& buffer, uint32_t binding);
    void addStaticBuffer(BufferPtr& buffer, const std::string& descName);

    void addStaticBufferView(BufferViewPtr& bufferView, uint32_t binding);
    void addStaticBufferView(BufferViewPtr& bufferView, const std::string& descName);

    void addStaticImageView(ImageViewPtr& imageView, uint32_t binding);
    void addImageSampler(ImageSamplerPtr& imageSampler, uint32_t binding);
    void addStaticTexture(TexturePtr& texture, uint32_t binding) ;

    void addStaticImageView(ImageViewPtr& imageView, const std::string& descName);
    void addImageSampler(ImageSamplerPtr& imageSampler, const std::string& descName);
    void addStaticTexture(TexturePtr& texture, const std::string& descName);

    /*
     * Dynamic data changes per frame. After adding the buffer, the per-frame buffer needs to be retrieved by calling
     * getBuffer.
     */
    void addDynamicBuffer(BufferPtr& buffer, uint32_t binding);
    void addDynamicBuffer(BufferPtr& buffer, const std::string& descName);

    void addDynamicBufferView(BufferViewPtr& bufferView, uint32_t binding);
    void addDynamicBufferView(BufferViewPtr& bufferView, const std::string& descName);

    void addDynamicImageView(ImageViewPtr& imageView, uint32_t binding);
    void addDynamicImageView(ImageViewPtr& imageView, const std::string& descName);

    BufferPtr getBuffer(uint32_t binding);
    BufferPtr getBuffer(const std::string& name);

    inline VkDescriptorSet getVkDescriptorSet(uint32_t frameIdx) { return frameDataList.at(frameIdx).descriptorSet; }
    VkDescriptorSet getVkDescriptorSet();

private:
    void _updateDescriptorSets();
    ListenerToken swapchainRecreatedEventListenerToken;
    bool isDirty = false;

    Renderer* renderer;
    Device* device;
    ShaderStagesPtr shaderStages;

    std::map<uint32_t, bool> buffersStatic;
    std::map<uint32_t, bool> bufferViewsStatic;
    std::map<uint32_t, bool> imageViewsStatic;
    std::map<uint32_t, bool> accelerationStructuresStatic;

    struct FrameData {
        std::map<uint32_t, BufferPtr> buffers;
        std::map<uint32_t, BufferViewPtr> bufferViews;
        std::map<uint32_t, ImageViewPtr> imageViews;
        std::map<uint32_t, ImageSamplerPtr> imageSamplers;
        std::map<uint32_t, VkAccelerationStructureKHR> accelerationStructures; // TODO
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };
    std::vector<FrameData> frameDataList;
};

class RasterData : public RenderData {
public:
    RasterData(Renderer* renderer, GraphicsPipelinePtr& graphicsPipeline);
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
    RayTracingData(Renderer* renderer, RayTracingPipelinePtr& rayTracingPipeline);

    inline RayTracingPipelinePtr getRayTracingPipeline() { return rayTracingPipeline; }

protected:
    RayTracingPipelinePtr rayTracingPipeline;
};

typedef std::shared_ptr<ComputeData> ComputeDataPtr;
typedef std::shared_ptr<RasterData> RasterDataPtr;
typedef std::shared_ptr<RayTracingData> RayTracingDataPtr;

}}

#endif //SGL_DATA_HPP
