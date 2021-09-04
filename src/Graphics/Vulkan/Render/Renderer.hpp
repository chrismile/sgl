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

#ifndef SGL_RENDERER_HPP
#define SGL_RENDERER_HPP

#include <vector>
#include <memory>
#include <glm/mat4x4.hpp>
#include "../libs/volk/volk.h"

#include <Utils/CircularQueue.hpp>
#include <Math/Geometry/MatrixUtil.hpp>

namespace sgl { namespace vk {

class Device;
class Semaphore;
typedef std::shared_ptr<Semaphore> SemaphorePtr;
class Fence;
typedef std::shared_ptr<Fence> FencePtr;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class Image;
typedef std::shared_ptr<Image> ImagePtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;
class ComputeData;
typedef std::shared_ptr<ComputeData> ComputeDataPtr;
class RasterData;
typedef std::shared_ptr<RasterData> RasterDataPtr;
class RayTracingData;
typedef std::shared_ptr<RayTracingData> RayTracingDataPtr;
class Pipeline;
typedef std::shared_ptr<Pipeline> PipelinePtr;
class GraphicsPipeline;
typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;
class ComputePipeline;
typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;
class RayTracingPipeline;
typedef std::shared_ptr<RayTracingPipeline> RayTracingPipelinePtr;

class DLL_OBJECT Renderer {
public:
    Renderer(Device* device, uint32_t numDescriptors = 1000);
    ~Renderer();

    // @see beginCommandBuffer and @see endCommandBuffer need to be called before calling any other command.
    void beginCommandBuffer();
    VkCommandBuffer endCommandBuffer();
    /// Use VK_NULL_HANDLE to reset the custom command buffer.
    void setCustomCommandBuffer(VkCommandBuffer commandBuffer, bool useGraphicsQueue = true);
    void resetCustomCommandBuffer();

    // Graphics pipeline.
    void render(const RasterDataPtr& rasterData);
    void render(const RasterDataPtr& rasterData, const FramebufferPtr& framebuffer);
    void setModelMatrix(const glm::mat4 &matrix);
    void setViewMatrix(const glm::mat4 &matrix);
    void setProjectionMatrix(const glm::mat4 &matrix);

    // Compute pipeline.
    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    // Ray tracing pipeline.
    void traceRays(
            const RayTracingDataPtr& rayTracingData,
            uint32_t launchSizeX, uint32_t launchSizeY, uint32_t launchSizeZ = 1);

    // Image pipeline barrier.
    void transitionImageLayout(vk::ImagePtr& image, VkImageLayout newLayout);

    // Push constants.
    void pushConstants(
            PipelinePtr& pipeline, VkShaderStageFlagBits shaderStageFlagBits,
            uint32_t offset, uint32_t size, const void* data);
    template<class T>
    void pushConstants(
            PipelinePtr& pipeline, VkShaderStageFlagBits shaderStageFlagBits,
            uint32_t offset, const T& data) {
        pushConstants(pipeline, shaderStageFlagBits, offset, sizeof(T), &data);
    }

    // Synchronization primitives.
    void insertMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    void insertBufferMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            BufferPtr& buffer);
    void insertBufferMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
            BufferPtr& buffer);

    /**
     * For headless rendering without a swapchain.
     * @param waitSemaphores Semaphore to wait on before executing the submitted work.
     * @param signalSemaphores Semaphore to signal after executing the submitted work.
     * @param fence Fence to check on the CPU whether the execution is still in-flight.
     * @param waitStage The pipeline stages to wait on.
     */
    void submitToQueue(
            const SemaphorePtr& waitSemaphore, const SemaphorePtr& signalSemaphore, const FencePtr& fence,
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    /**
     * For headless rendering without a swapchain.
     * @param waitSemaphores Semaphores to wait on before executing the submitted work.
     * @param signalSemaphores Semaphores to signal after executing the submitted work.
     * @param fence Fence to check on the CPU whether the execution is still in-flight.
     * @param waitStages An array of pipeline stages to wait on.
     */
    void submitToQueue(
            const std::vector<SemaphorePtr>& waitSemaphores, const std::vector<SemaphorePtr>& signalSemaphores,
            const FencePtr& fence,
            const std::vector<VkPipelineStageFlags>& waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT });
    /**
     * For headless rendering without a swapchain. This command waits for the queue to become idle manually.
     */
    void submitToQueueImmediate();

    // Access to internal state.
    inline Device* getDevice() { return device; }
    inline VkCommandBuffer getVkCommandBuffer() { return commandBuffer; }
    inline VkDescriptorPool getVkDescriptorPool() { return globalDescriptorPool; }
    inline void clearGraphicsPipeline() {
        graphicsPipeline = GraphicsPipelinePtr();
        lastFramebuffer = FramebufferPtr();
        recordingCommandBufferStarted = true;
    }
    inline const glm::mat4& getModelMatrix() const { return matrixBlock.mMatrix; }
    inline const glm::mat4& getViewMatrix() const { return matrixBlock.vMatrix; }
    inline const glm::mat4& getProjectionMatrix() const { return matrixBlock.pMatrix; }
    inline const glm::mat4& getModelViewProjectionMatrix() const { return matrixBlock.mvpMatrix; }

private:
    Device* device;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandBuffer customCommandBuffer = VK_NULL_HANDLE;
    bool useGraphicsQueue = true;

    // Global descriptor pool that can be used by ComputeData, RasterData and RayTracingData.
    VkDescriptorPool globalDescriptorPool;

    // Rasterizer state.
    GraphicsPipelinePtr graphicsPipeline;
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    VkClearDepthStencilValue clearDepthStencil = { 1.0f, 0 };
    FramebufferPtr lastFramebuffer;

    // Compute state.
    ComputePipelinePtr computePipeline;

    // Ray tracing state.
    RayTracingPipelinePtr rayTracingPipeline;

    // Global state.
    bool updateMatrixBlock();
    struct MatrixBlock {
        glm::mat4 mMatrix = matrixIdentity(); // Model matrix
        glm::mat4 vMatrix = matrixIdentity(); // View matrix
        glm::mat4 pMatrix = matrixIdentity(); // Projection matrix
        glm::mat4 mvpMatrix = matrixIdentity(); // Model-view-projection matrix
    };
    bool matrixBlockNeedsUpdate = true;
    MatrixBlock matrixBlock;
    BufferPtr currentMatrixBlockBuffer;
    VkDescriptorSet matrixBlockDescriptorSet;
    bool recordingCommandBufferStarted = true;

    // Some data needs to be stored per swapchain image.
    struct FrameCache {
        CircularQueue<BufferPtr> freeCameraMatrixBuffers;
        CircularQueue<VkDescriptorSet> freeMatrixBlockDescriptorSets;
        CircularQueue<BufferPtr> allCameraMatrixBuffers;
        CircularQueue<VkDescriptorSet> allMatrixBlockDescriptorSets;
    };
    const uint32_t maxFrameCacheSize = 1000;
    std::vector<FrameCache> frameCaches;
    size_t frameIndex = 0;
    VkDescriptorPool matrixBufferDescriptorPool;
    VkDescriptorSetLayout matrixBufferDesciptorSetLayout;
};

}}

#endif //SGL_RENDERER_HPP
