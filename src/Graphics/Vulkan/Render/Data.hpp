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

class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class BufferView;
typedef std::shared_ptr<BufferView> BufferViewPtr;

class Image;
typedef std::shared_ptr<Image> ImagePtr;
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class ImageSampler;
typedef std::shared_ptr<ImageSampler> ImageSamplerPtr;
class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

class GraphicsPipeline;
typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;
class ComputePipeline;
typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;
class RayTracingPipeline;
typedef std::shared_ptr<RayTracingPipeline> RayTracingPipelinePtr;
class Renderer;

class RenderData;
typedef std::shared_ptr<RenderData> RenderDataPtr;
typedef uint32_t ListenerToken;

class DLL_OBJECT RenderData {
    friend class Renderer;
public:
    RenderData(Renderer* renderer, ShaderStagesPtr& shaderStages);
    virtual ~RenderData();

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
    void setStaticBuffer(BufferPtr& buffer, uint32_t binding);
    void setStaticBuffer(BufferPtr& buffer, const std::string& descName);

    void setStaticBufferView(BufferViewPtr& bufferView, uint32_t binding);
    void setStaticBufferView(BufferViewPtr& bufferView, const std::string& descName);

    void setStaticImageView(ImageViewPtr& imageView, uint32_t binding);
    void setImageSampler(ImageSamplerPtr& imageSampler, uint32_t binding);
    void setStaticTexture(TexturePtr& texture, uint32_t binding);

    void setStaticImageView(ImageViewPtr& imageView, const std::string& descName);
    void setImageSampler(ImageSamplerPtr& imageSampler, const std::string& descName);
    void setStaticTexture(TexturePtr& texture, const std::string& descName);

    /*
     * Dynamic data changes per frame. After adding the buffer, the per-frame buffer needs to be retrieved by calling
     * getBuffer.
     */
    void setDynamicBuffer(BufferPtr& buffer, uint32_t binding);
    void setDynamicBuffer(BufferPtr& buffer, const std::string& descName);

    void setDynamicBufferView(BufferViewPtr& bufferView, uint32_t binding);
    void setDynamicBufferView(BufferViewPtr& bufferView, const std::string& descName);

    void setDynamicImageView(ImageViewPtr& imageView, uint32_t binding);
    void setDynamicImageView(ImageViewPtr& imageView, const std::string& descName);

    BufferPtr getBuffer(uint32_t binding);
    BufferPtr getBuffer(const std::string& name);

    inline VkDescriptorSet getVkDescriptorSet(uint32_t frameIdx) { return frameDataList.at(frameIdx).descriptorSet; }
    VkDescriptorSet getVkDescriptorSet();


    struct FrameData {
        std::map<uint32_t, BufferPtr> buffers;
        std::map<uint32_t, BufferViewPtr> bufferViews;
        std::map<uint32_t, ImageViewPtr> imageViews;
        std::map<uint32_t, ImageSamplerPtr> imageSamplers;
        std::map<uint32_t, VkAccelerationStructureKHR> accelerationStructures; // TODO
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };
    inline FrameData& getFrameData(uint32_t frameIdx) { return frameDataList.at(frameIdx); }

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

    std::vector<FrameData> frameDataList;
};

class DLL_OBJECT ComputeData : public RenderData {
public:
    explicit ComputeData(Renderer* renderer, ComputePipelinePtr& computePipeline);

    inline ComputePipelinePtr getComputePipeline() { return computePipeline; }

protected:
    ComputePipelinePtr computePipeline;
};

class DLL_OBJECT RasterData : public RenderData {
public:
    RasterData(Renderer* renderer, GraphicsPipelinePtr& graphicsPipeline);
    void setIndexBuffer(BufferPtr& buffer, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void setVertexBuffer(BufferPtr& buffer, uint32_t binding);
    void setVertexBuffer(BufferPtr& buffer, const std::string& name);

    inline void setNumInstances(size_t numInstances) { this->numInstances = numInstances; }
    inline size_t getNumInstances() const { return numInstances; }

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

class DLL_OBJECT RayTracingData : public RenderData {
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
