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

#include <array>
#include <memory>
#include "../libs/volk/volk.h"

#include "../Shader/Shader.hpp"
#include "../Buffers/Buffer.hpp"

#include "ShaderGroupSettings.hpp"

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

class TopLevelAccelerationStructure;
typedef std::shared_ptr<TopLevelAccelerationStructure> TopLevelAccelerationStructurePtr;

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

enum class RenderDataType {
    COMPUTE, RASTER, RAYTRACING
};

struct RenderDataSize {
    size_t indexBufferSize = 0;
    size_t vertexBufferSize = 0;
    size_t storageBufferSize = 0;
    size_t uniformBufferSize = 0;
    size_t imageSize = 0;
    size_t accelerationStructureSize = 0;
};

class DLL_OBJECT RenderData {
    friend class Renderer;
public:
    RenderData(Renderer* renderer, ShaderStagesPtr& shaderStages);
    virtual ~RenderData();
    [[nodiscard]] virtual RenderDataType getRenderDataType() const = 0;

    inline const ShaderStagesPtr& getShaderStages() { return shaderStages; }

    /**
     * Creates a shallow copy of the render data using the passed shader stages.
     * NOTE: The descriptor layouts of the new shaders have to match!
     * @param shaderStages The shader stages to use in the copy.
     */
    //RenderDataPtr copy(ShaderStagesPtr& shaderStages);

    void onSwapchainRecreated();

    /*
     * The content of static data can only be updated on the CPU when vkQueueWaitIdle was called on the command queue.
     * They should be used, e.g., for look-up tables or for objects only used exclusively by the GPU.
     * It is recommended to create the objects using the memory usage VMA_MEMORY_USAGE_GPU_ONLY.
     */
    void setStaticBuffer(const BufferPtr& buffer, uint32_t binding);
    void setStaticBuffer(const BufferPtr& buffer, const std::string& descName);
    void setStaticBufferOptional(const BufferPtr& buffer, const std::string& descName);

    void setStaticBufferView(const BufferViewPtr& bufferView, uint32_t binding);
    void setStaticBufferView(const BufferViewPtr& bufferView, const std::string& descName);
    void setStaticBufferViewOptional(const BufferViewPtr& bufferView, const std::string& descName);

    void setStaticImageView(const ImageViewPtr& imageView, uint32_t binding);
    void setImageSampler(const ImageSamplerPtr& imageSampler, uint32_t binding);
    void setStaticTexture(const TexturePtr& texture, uint32_t binding);

    void setStaticImageView(const ImageViewPtr& imageView, const std::string& descName);
    void setImageSampler(const ImageSamplerPtr& imageSampler, const std::string& descName);
    void setStaticTexture(const TexturePtr& texture, const std::string& descName);
    void setStaticImageViewOptional(const ImageViewPtr& imageView, const std::string& descName);
    void setImageSamplerOptional(const ImageSamplerPtr& imageSampler, const std::string& descName);
    void setStaticTextureOptional(const TexturePtr& texture, const std::string& descName);

    void setTopLevelAccelerationStructure(const TopLevelAccelerationStructurePtr& tlas, uint32_t binding);
    void setTopLevelAccelerationStructure(const TopLevelAccelerationStructurePtr& tlas, const std::string& descName);
    void setTopLevelAccelerationStructureOptional(
            const TopLevelAccelerationStructurePtr& tlas, const std::string& descName);

    // Binds a small dummy buffer (of size 4 bytes) in order to avoid warnings.
    void setStaticBufferUnused(uint32_t binding);
    void setStaticBufferUnused(const std::string& descName);

    /*
     * Dynamic data changes per frame. After adding the buffer, the per-frame buffer needs to be retrieved by calling
     * getBuffer.
     */
    void setDynamicBuffer(const BufferPtr& buffer, uint32_t binding);
    void setDynamicBuffer(const BufferPtr& buffer, const std::string& descName);
    void setDynamicBufferOptional(const BufferPtr& buffer, const std::string& descName);

    void setDynamicBufferView(const BufferViewPtr& bufferView, uint32_t binding);
    void setDynamicBufferView(const BufferViewPtr& bufferView, const std::string& descName);
    void setDynamicBufferViewOptional(const BufferViewPtr& bufferView, const std::string& descName);

    void setDynamicImageView(const ImageViewPtr& imageView, uint32_t binding);
    void setDynamicImageView(const ImageViewPtr& imageView, const std::string& descName);
    void setDynamicImageViewOptional(const ImageViewPtr& imageView, const std::string& descName);

    BufferPtr getBuffer(uint32_t binding);
    BufferPtr getBuffer(const std::string& name);

    ImageViewPtr getImageView(uint32_t binding);
    ImageViewPtr getImageView(const std::string& name);

    virtual RenderDataSize getRenderDataSize();
    size_t getRenderDataSizeSizeInBytes();

    inline VkDescriptorSet getVkDescriptorSet(uint32_t frameIdx) { return frameDataList.at(frameIdx).descriptorSet; }
    VkDescriptorSet getVkDescriptorSet();

    struct FrameData {
        std::map<uint32_t, BufferPtr> buffers;
        std::map<uint32_t, BufferViewPtr> bufferViews;
        std::map<uint32_t, ImageViewPtr> imageViews;
        std::map<uint32_t, ImageSamplerPtr> imageSamplers;
        std::map<uint32_t, TopLevelAccelerationStructurePtr> accelerationStructures;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };
    inline FrameData& getFrameData(uint32_t frameIdx) { return frameDataList.at(frameIdx); }

protected:
    void _updateDescriptorSets();

private:
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
    [[nodiscard]] RenderDataType getRenderDataType() const override { return RenderDataType::COMPUTE; }

    inline const ComputePipelinePtr& getComputePipeline() { return computePipeline; }

     /**
      * Dispatches the compute shader using the passed command buffer.
      * NOTE: The preferred way for this is using @see sgl::vk::Renderer.
      * @param groupCountX The group count in x direction.
      * @param groupCountY The group count in x direction.
      * @param groupCountZ The group count in x direction.
      * @param commandBuffer The command buffer in which to enqueue the dispatched compute shader.
      */
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, VkCommandBuffer commandBuffer);

    /**
     * Dispatches the compute shader using the passed command buffer.
     * NOTE: The preferred way for this is using @see sgl::vk::Renderer.
     * @param dispatchIndirectBuffer A buffer containing a struct of the type VkDispatchIndirectCommand.
     * @param offset A byte offset into the dispatch indirect buffer.
     * @param commandBuffer The command buffer in which to enqueue the dispatched compute shader.
     */
    void dispatchIndirect(
            const sgl::vk::BufferPtr& dispatchIndirectBuffer, VkDeviceSize offset, VkCommandBuffer commandBuffer);
    void dispatchIndirect(
            const sgl::vk::BufferPtr& dispatchIndirectBuffer, VkCommandBuffer commandBuffer);

    // Push constants for @see dispatch.
    void pushConstants(uint32_t offset, uint32_t size, const void* data, VkCommandBuffer commandBuffer);
    template<class T>
    void pushConstants(uint32_t offset, const T& data, VkCommandBuffer commandBuffer) {
        pushConstants(offset, sizeof(T), &data, commandBuffer);
    }

protected:
    ComputePipelinePtr computePipeline;
};

class DLL_OBJECT RasterData : public RenderData {
public:
    RasterData(Renderer* renderer, GraphicsPipelinePtr& graphicsPipeline);
    [[nodiscard]] RenderDataType getRenderDataType() const override { return RenderDataType::RASTER; }

    void setIndexBuffer(const BufferPtr& buffer, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void setVertexBuffer(const BufferPtr& buffer, uint32_t binding);
    void setVertexBuffer(const BufferPtr& buffer, const std::string& name);
    void setVertexBufferOptional(const BufferPtr& buffer, const std::string& name);

    [[nodiscard]] inline bool getHasIndexBuffer() const { return indexBuffer.get(); }
    [[nodiscard]] inline size_t getNumIndices() const { return numIndices; }
    [[nodiscard]] inline VkIndexType getIndexType() const { return indexType; }
    [[nodiscard]] inline VkBuffer getVkIndexBuffer() { return indexBuffer->getVkBuffer(); }

    [[nodiscard]] inline size_t getNumVertices() const { return numVertices; }
    [[nodiscard]] inline const std::vector<VkBuffer>& getVkVertexBuffers() { return vulkanVertexBuffers; }

    inline void setNumInstances(size_t _numInstances) { this->numInstances = _numInstances; }
    [[nodiscard]] inline size_t getNumInstances() const { return numInstances; }

    /// For use with https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkCmdDrawMeshTasksNV.html.
    void setMeshTasks(uint32_t _taskCount, uint32_t _firstTask) {
        taskCount = _taskCount;
        firstTask = _firstTask;
    }
    [[nodiscard]] inline uint32_t getTaskCount() const { return taskCount; }
    [[nodiscard]] inline uint32_t getFirstTask() const { return firstTask; }

    /*
     * For use with vkCmdDrawIndirect, vkCmdDrawIndirectCount, vkCmdDrawIndexedIndirect, vkCmdDrawIndexedIndirectCount,
     * vkCmdDrawMeshTasksIndirectNV and vkCmdDrawMeshTasksIndirectCountNV.
     */
    /**
     * Sets the indirect draw command buffer. It contains entries either of the type VkDrawIndirectCommand for
     * vkCmdDrawIndirect and vkCmdDrawIndirectCount, or VkDrawIndexedIndirectCommand for vkCmdDrawIndexedIndirect and
     * vkCmdDrawIndexedIndirectCount, or VkDrawMeshTasksIndirectCommandNV for vkCmdDrawMeshTasksIndirectNV and
     * vkCmdDrawMeshTasksIndirectCountNV.
     * @param buffer The buffer containing the VkDrawIndirectCommand/VkDrawIndexedIndirectCommand/
     * VkDrawMeshTasksIndirectCommandNV entries.
     * @param stride Stride in bytes between two entries. It is usually either sizeof(VkDrawIndirectCommand),
     * sizeof(VkDrawIndexedIndirectCommand) or sizeof(VkDrawMeshTasksIndirectCommandNV).
     * @param offset Offset in bytes to the first entry in the buffer.
     */
    void setIndirectDrawBuffer(const BufferPtr& buffer, uint32_t stride, VkDeviceSize offset = 0) {
        indirectDrawBuffer = buffer;
        indirectDrawBufferStride = stride;
        indirectDrawBufferOffset = offset;
    }
    /**
     * For vkCmdDrawIndirect, vkCmdDrawIndexedIndirect and vkCmdDrawMeshTasksIndirectNV.
     * @param drawCount The number of elements to read from the indirect draw buffer.
     */
    void setIndirectDrawCount(uint32_t drawCount) {
        indirectDrawCount = drawCount;
    }
    /**
     * For vkCmdDrawIndirectCount, vkCmdDrawIndexedIndirectCount and vkCmdDrawMeshTasksIndirectCountNV.
     * @param buffer The buffer to read the number of elements to read from the indirect draw buffer from.
     * @param maxDrawCount The maximum draw count. The actual draw count will be the minimum of this value and the
     * value read from the buffer.
     * @param offset Offset in bytes to the uint32_t value to read from the buffer.
     */
    void setIndirectDrawCountBuffer(const BufferPtr& buffer, uint32_t maxDrawCount, VkDeviceSize offset = 0) {
        indirectDrawCountBuffer = buffer;
        indirectMaxDrawCount = maxDrawCount;
        indirectDrawCountBufferOffset = offset;
    }
    [[nodiscard]] inline bool getUseIndirectDraw() const { return indirectDrawBuffer.get() != nullptr; }
    [[nodiscard]] inline bool getUseIndirectDrawCount() const { return indirectDrawCountBuffer.get() != nullptr; }
    [[nodiscard]] const BufferPtr& getIndirectDrawBuffer() const { return indirectDrawBuffer; };
    [[nodiscard]] VkBuffer getIndirectDrawBufferVk() const { return indirectDrawBuffer->getVkBuffer(); };
    [[nodiscard]] uint32_t getIndirectDrawBufferStride() const { return indirectDrawBufferStride; };
    [[nodiscard]] VkDeviceSize getIndirectDrawBufferOffset() const { return indirectDrawBufferOffset; };
    // For vkCmdDrawIndirect, vkCmdDrawIndexedIndirect and vkCmdDrawMeshTasksIndirectNV.
    [[nodiscard]] uint32_t getIndirectDrawCount() const { return indirectDrawCount; };
    // For vkCmdDrawIndirectCount, vkCmdDrawIndexedIndirectCount and vkCmdDrawMeshTasksIndirectCountNV.
    [[nodiscard]] const BufferPtr& getIndirectDrawCountBuffer() const { return indirectDrawCountBuffer; };
    [[nodiscard]] VkBuffer getIndirectDrawCountBufferVk() const { return indirectDrawCountBuffer->getVkBuffer(); };
    [[nodiscard]] uint32_t getIndirectMaxDrawCount() const { return indirectMaxDrawCount; };
    [[nodiscard]] VkDeviceSize getIndirectDrawCountBufferOffset() const { return indirectDrawCountBufferOffset; };

    [[nodiscard]] inline const GraphicsPipelinePtr& getGraphicsPipeline() { return graphicsPipeline; }

    RenderDataSize getRenderDataSize() override;

protected:
    GraphicsPipelinePtr graphicsPipeline;

    size_t numInstances = 1;

    BufferPtr indexBuffer;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    size_t numIndices = 0;

    std::vector<BufferPtr> vertexBuffers;
    size_t numVertices = 0;
    std::vector<VkBuffer> vulkanVertexBuffers;

    // In case task/mesh shaders are used.
    uint32_t taskCount = 0;
    uint32_t firstTask = 0;

    // In case indirect draw is used.
    BufferPtr indirectDrawBuffer;
    uint32_t indirectDrawBufferStride = 0;
    VkDeviceSize indirectDrawBufferOffset = 0;
    // For vkCmdDrawIndirect and vkCmdDrawIndexedIndirect.
    uint32_t indirectDrawCount = 0;
    // For vkCmdDrawIndirectCount and vkCmdDrawIndexedIndirectCount.
    BufferPtr indirectDrawCountBuffer;
    uint32_t indirectMaxDrawCount = 0;
    VkDeviceSize indirectDrawCountBufferOffset = 0;
};

class DLL_OBJECT RayTracingData : public RenderData {
public:
    RayTracingData(
            Renderer* renderer, RayTracingPipelinePtr& rayTracingPipeline,
            const ShaderGroupSettings& settings = ShaderGroupSettings());
    [[nodiscard]]  RenderDataType getRenderDataType() const override { return RenderDataType::RAYTRACING; }

    void setShaderGroupSettings(const ShaderGroupSettings& settings);
    [[nodiscard]] inline ShaderGroupSettings& getShaderGroupSettings() { return shaderGroupSettings; }
    [[nodiscard]] inline const ShaderGroupSettings& getShaderGroupSettings() const { return shaderGroupSettings; }

    [[nodiscard]] inline const RayTracingPipelinePtr& getRayTracingPipeline() { return rayTracingPipeline; }
    inline const std::array<VkStridedDeviceAddressRegionKHR, 4>& getStridedDeviceAddressRegions() {
        return stridedDeviceAddressRegions;
    }

protected:
    RayTracingPipelinePtr rayTracingPipeline;
    ShaderGroupSettings shaderGroupSettings;
    std::array<VkStridedDeviceAddressRegionKHR, 4> stridedDeviceAddressRegions{};
};

typedef std::shared_ptr<ComputeData> ComputeDataPtr;
typedef std::shared_ptr<RasterData> RasterDataPtr;
typedef std::shared_ptr<RayTracingData> RayTracingDataPtr;

}}

#endif //SGL_DATA_HPP
