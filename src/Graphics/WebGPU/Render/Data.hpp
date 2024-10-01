/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_WEBGPU_DATA_HPP
#define SGL_WEBGPU_DATA_HPP

#include <vector>
#include <map>
#include <memory>
#include "../Buffer/Buffer.hpp"
#include <webgpu/webgpu.h>

namespace sgl {
typedef uint32_t ListenerToken;
}

namespace sgl { namespace webgpu {

class Device;
class Renderer;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class TextureView;
typedef std::shared_ptr<TextureView> TextureViewPtr;
class Sampler;
typedef std::shared_ptr<Sampler> SamplerPtr;
class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;
class ComputePipeline;
typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;
class RenderPipeline;
typedef std::shared_ptr<RenderPipeline> RenderPipelinePtr;

enum class DataType {
    COMPUTE, RASTER
};

struct DataSize {
    size_t indexBufferSize = 0;
    size_t vertexBufferSize = 0;
    size_t storageBufferSize = 0;
    size_t uniformBufferSize = 0;
    size_t imageSize = 0;
    size_t accelerationStructureSize = 0;
};

class DLL_OBJECT Data {
    friend class Renderer;
public:
    Data(Renderer* renderer, ShaderStagesPtr& shaderStages);
    virtual ~Data();
    [[nodiscard]] virtual DataType getDataType() const = 0;

    inline const ShaderStagesPtr& getShaderStages() { return shaderStages; }

    void onSwapchainRecreated();

    void setBuffer(const BufferPtr& buffer, uint32_t bindingIndex);
    void setBuffer(const BufferPtr& buffer, const std::string& descName);
    void setBufferOptional(const BufferPtr& buffer, const std::string& descName);

    void setTextureView(const TextureViewPtr& textureView, uint32_t bindingIndex);
    void setTextureView(const TextureViewPtr& textureView, const std::string& descName);
    void setTextureViewOptional(const TextureViewPtr& textureView, const std::string& descName);
    
    void setSampler(const SamplerPtr& sampler, uint32_t bindingIndex);
    void setSampler(const SamplerPtr& sampler, const std::string& descName);
    void setSamplerOptional(const SamplerPtr& sampler, const std::string& descName);

    // Binds a small dummy buffer (of size 4 bytes) in order to avoid warnings.
    void setBufferUnused(uint32_t bindingIndex);
    void setBufferUnused(const std::string& descName);

    BufferPtr getBuffer(uint32_t bindingIndex);
    BufferPtr getBuffer(const std::string& name);

    TextureViewPtr getTextureView(uint32_t bindingIndex);
    TextureViewPtr getTextureView(const std::string& name);

    virtual DataSize getDataSize();
    size_t getDataSizeSizeInBytes();

    [[nodiscard]] WGPUBindGroup getWGPUBindGroup() { return bindGroup; }

protected:
    void _updateBindingGroups();

private:
    ListenerToken swapchainRecreatedEventListenerToken;
    bool isDirty = false;

    Renderer* renderer;
    Device* device;
    ShaderStagesPtr shaderStages;

    // Frame data.
    std::map<uint32_t, BufferPtr> buffers;
    std::map<uint32_t, TextureViewPtr> textureViews;
    std::map<uint32_t, SamplerPtr> samplers;
    WGPUBindGroup bindGroup = {}; //< Currently, only group 0 is supported.
};

class DLL_OBJECT ComputeData : public Data {
    friend class Renderer;
public:
    explicit ComputeData(Renderer* renderer, ComputePipelinePtr& computePipeline);
    [[nodiscard]] DataType getDataType() const override { return DataType::COMPUTE; }

    inline const ComputePipelinePtr& getComputePipeline() { return computePipeline; }

    /**
     * Dispatches the compute shader using the passed command buffer.
     * NOTE: The preferred way for this is using @see sgl::vk::Renderer.
     * @param groupCountX The group count in x direction.
     * @param groupCountY The group count in x direction.
     * @param groupCountZ The group count in x direction.
     * @param commandBuffer The command buffer in which to enqueue the dispatched compute shader.
     */
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, WGPUCommandEncoder commandEncoder);

    /**
     * Dispatches the compute shader using the passed command buffer.
     * NOTE: The preferred way for this is using @see sgl::vk::Renderer.
     * @param dispatchIndirectBuffer A buffer containing a struct of the type VkDispatchIndirectCommand.
     * @param offset A byte offset into the dispatch indirect buffer.
     * @param commandBuffer The command buffer in which to enqueue the dispatched compute shader.
     */
    void dispatchIndirect(
            const sgl::webgpu::BufferPtr& dispatchIndirectBuffer, uint64_t offset, WGPUCommandEncoder commandEncoder);
    void dispatchIndirect(
            const sgl::webgpu::BufferPtr& dispatchIndirectBuffer, WGPUCommandEncoder commandEncoder);

protected:
    ComputePipelinePtr computePipeline;
};

class DLL_OBJECT RenderData : public Data {
public:
    RenderData(Renderer* renderer, RenderPipelinePtr& renderPipeline);
    [[nodiscard]] DataType getDataType() const override { return DataType::RASTER; }

    void setIndexBuffer(const BufferPtr& buffer, WGPUIndexFormat indexType = WGPUIndexFormat_Uint32);
    void setVertexBuffer(const BufferPtr& buffer, uint32_t binding);
    void setVertexBuffer(const BufferPtr& buffer, const std::string& name);
    void setVertexBufferOptional(const BufferPtr& buffer, const std::string& name);

    [[nodiscard]] inline bool getHasIndexBuffer() const { return indexBuffer.get(); }
    [[nodiscard]] inline size_t getNumIndices() const { return numIndices; }
    [[nodiscard]] inline WGPUIndexFormat getIndexFormat() const { return indexType; }
    [[nodiscard]] inline BufferPtr getIndexBuffer() { return indexBuffer; }
    [[nodiscard]] inline WGPUBuffer getWGPUIndexBuffer() { return indexBuffer->getWGPUBuffer(); }

    /// setNumVertices should only be used when using programmable fetching (i.e., no vertex or index buffer set).
    void setNumVertices(size_t _numVertices);
    [[nodiscard]] inline size_t getNumVertices() const { return numVertices; }
    [[nodiscard]] inline const std::vector<BufferPtr>& getVertexBuffers() { return vertexBuffers; }
    [[nodiscard]] inline const std::vector<WGPUBuffer>& getWGPUVertexBuffers() { return wgpuVertexBuffers; }
    [[nodiscard]] inline const std::vector<uint32_t>& getVertexBufferSlots() { return vertexBufferSlots; }

    inline void setNumInstances(size_t _numInstances) { this->numInstances = _numInstances; }
    [[nodiscard]] inline size_t getNumInstances() const { return numInstances; }

    /**
     * Sets the indirect draw command buffer. It contains entries for one of the following densely packed structs.
     * @param buffer The buffer containing the array of struct entries.
     * @param offset Offset in bytes to the first entry in the buffer.
     *
     * https://www.w3.org/TR/webgpu/#indirect-drawindexed-parameters
     * uint32_t indexCount;
     * uint32_t instanceCount;
     * uint32_t firstIndex;
     * int32_t baseVertex;
     * uint32_t firstInstance;
     *
     * https://www.w3.org/TR/webgpu/#indirect-draw-parameters
     * uint32_t vertexCount;
     * uint32_t instanceCount;
     * uint32_t firstVertex;
     * uint32_t firstInstance;
     *
     * https://www.w3.org/TR/webgpu/#indirect-dispatch-parameters
     * dispatchIndirectParameters[0] = workgroupCountX;
     * dispatchIndirectParameters[1] = workgroupCountY;
     * dispatchIndirectParameters[2] = workgroupCountZ;
     */
    void setIndirectDrawBuffer(const BufferPtr& buffer, uint64_t offset = 0) {
        indirectDrawBuffer = buffer;
        //indirectDrawBufferStride = stride;
        indirectDrawBufferOffset = offset;
    }
    /**
     * @param drawCount The number of elements to read from the indirect draw buffer.
     */
    void setIndirectDrawCount(uint32_t drawCount) {
        indirectDrawCount = drawCount;
    }
    [[nodiscard]] inline bool getUseIndirectDraw() const { return indirectDrawBuffer.get() != nullptr; }
    [[nodiscard]] const BufferPtr& getIndirectDrawBuffer() const { return indirectDrawBuffer; };
    [[nodiscard]] WGPUBuffer getWGPUIndirectDrawBuffer() const { return indirectDrawBuffer->getWGPUBuffer(); };
    //[[nodiscard]] uint32_t getIndirectDrawBufferStride() const { return indirectDrawBufferStride; };
    [[nodiscard]] uint64_t getIndirectDrawBufferOffset() const { return indirectDrawBufferOffset; };
    // For vkCmdDrawIndirect, vkCmdDrawIndexedIndirect, vkCmdDrawMeshTasksIndirectNV and vkCmdDrawMeshTasksIndirectEXT.
    [[nodiscard]] uint32_t getIndirectDrawCount() const { return indirectDrawCount; };

    [[nodiscard]] inline const RenderPipelinePtr& getRenderPipeline() { return renderPipeline; }

    DataSize getDataSize() override;

protected:
    RenderPipelinePtr renderPipeline;

    size_t numInstances = 1;

    BufferPtr indexBuffer;
    WGPUIndexFormat indexType = WGPUIndexFormat_Uint32;
    size_t numIndices = 0;

    size_t numVertices = 0;
    std::vector<BufferPtr> vertexBuffers;
    std::vector<WGPUBuffer> wgpuVertexBuffers;
    std::vector<uint32_t> vertexBufferSlots;

    // In case indirect draw is used.
    BufferPtr indirectDrawBuffer;
    //uint32_t indirectDrawBufferStride = 0; // WGPU doesn't support stride, probably due to Metal.
    uint64_t indirectDrawBufferOffset = 0;
    uint32_t indirectDrawCount = 0;
};

}}

#endif //SGL_WEBGPU_DATA_HPP
