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
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;
class CommandBuffer;
typedef std::shared_ptr<CommandBuffer> CommandBufferPtr;
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
    explicit Renderer(Device* device, uint32_t numDescriptors = 1000);
    ~Renderer();

    // @see beginCommandBuffer and @see endCommandBuffer need to be called before calling any other command.
    void beginCommandBuffer();
    VkCommandBuffer endCommandBuffer();
    void setUseComputeQueue(bool _useComputeQueue);
    [[nodiscard]] inline bool getUseGraphicsQueue() const { return useGraphicsQueue; }
    [[nodiscard]] inline bool getUseComputeQueue() const { return !useGraphicsQueue; }
    /// Use VK_NULL_HANDLE to reset the custom command buffer.
    void setCustomCommandBuffer(VkCommandBuffer _commandBuffer, bool _useGraphicsQueue = true);
    void resetCustomCommandBuffer();
    void pushCommandBuffer(const sgl::vk::CommandBufferPtr& commandBuffer);
    std::vector<sgl::vk::CommandBufferPtr> getFrameCommandBuffers();
    /// Submits all previously queued work to the GPU and syncs with the CPU.
    void syncWithCpu();
    inline sgl::vk::CommandBufferPtr& getCommandBuffer() { return frameCommandBuffers.back(); }
    inline VkCommandBuffer getVkCommandBuffer() { return commandBuffer; }


    // Graphics pipeline.
    void render(const RasterDataPtr& rasterData);
    void render(const RasterDataPtr& rasterData, const FramebufferPtr& framebuffer);
    void setModelMatrix(const glm::mat4 &matrix);
    void setViewMatrix(const glm::mat4 &matrix);
    void setProjectionMatrix(const glm::mat4 &matrix);

    // Compute pipeline.
    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX);
    void dispatch(const ComputeDataPtr& computeData, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    // dispatchIndirectBuffer is a buffer containing a struct of the type VkDispatchIndirectCommand.
    void dispatchIndirect(
            const ComputeDataPtr& computeData, const sgl::vk::BufferPtr& dispatchIndirectBuffer, VkDeviceSize offset);
    void dispatchIndirect(
            const ComputeDataPtr& computeData, const sgl::vk::BufferPtr& dispatchIndirectBuffer);

    // Ray tracing pipeline.
    void traceRays(
            const RayTracingDataPtr& rayTracingData,
            uint32_t launchSizeX, uint32_t launchSizeY, uint32_t launchSizeZ = 1);

    // Image pipeline barrier.
    void transitionImageLayout(vk::ImagePtr& image, VkImageLayout newLayout);
    void transitionImageLayout(vk::ImageViewPtr& imageView, VkImageLayout newLayout);
    void transitionImageLayoutSubresource(
            vk::ImagePtr& image, VkImageLayout newLayout,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void insertImageMemoryBarrier(
            const vk::ImagePtr& image, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
    void insertImageMemoryBarriers(
            const std::vector<vk::ImagePtr>& images, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
    void insertImageMemoryBarrier(
            const vk::ImageViewPtr& imageView, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
    void insertImageMemoryBarrierSubresource(
            const vk::ImagePtr& image, VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

    // Push constants.
    void pushConstants(
            const PipelinePtr& pipeline, VkShaderStageFlagBits shaderStageFlagBits,
            uint32_t offset, uint32_t size, const void* data);
    template<class T>
    void pushConstants(
            const PipelinePtr& pipeline, VkShaderStageFlagBits shaderStageFlagBits,
            uint32_t offset, const T& data) {
        static_assert(!std::is_pointer_v<T>, "pushConstants<T> requires T to be a non-pointer type.");
        pushConstants(pipeline, shaderStageFlagBits, offset, sizeof(T), &data);
    }

    /**
     * Resolves a multisampled image.
     * @param sourceImage The multisampled source image.
     * @param destImage The destination image.
     */
    void resolveImage(const sgl::vk::ImageViewPtr& sourceImage, const sgl::vk::ImageViewPtr& destImage);

#ifdef VK_NV_cooperative_vector
    // Cooperative vector functionality.
    void convertCooperativeVectorMatrixNV(const VkConvertCooperativeVectorMatrixInfoNV& convertCoopVecMatInfo);
    void convertCooperativeVectorMatrixNV(
            const std::vector<VkConvertCooperativeVectorMatrixInfoNV>& convertCoopVecMatInfos);
#endif

    // Synchronization primitives.
    void insertMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    void insertBufferMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            const BufferPtr& buffer);
    void insertBufferMemoryBarrier(
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
            const BufferPtr& buffer);

    /**
     * For headless rendering without a swapchain. Submits and then resets all stored frame command buffers.
     * To store command buffers besides the main command buffer, pushCommandBuffer can be used.
     */
    void submitToQueue();
    /**
     * For headless rendering without a swapchain. It is recommended to use this function only for custom command
     * buffers (i.e., not set using pushCommandBuffer, but with setCustomCommandBuffer).
     * @param waitSemaphores Semaphore to wait on before executing the submitted work.
     * @param signalSemaphores Semaphore to signal after executing the submitted work.
     * @param fence Fence to check on the CPU whether the execution is still in-flight.
     * @param waitStage The pipeline stages to wait on.
     */
    void submitToQueue(
            const SemaphorePtr& waitSemaphore, const SemaphorePtr& signalSemaphore, const FencePtr& fence,
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    /**
     * For headless rendering without a swapchain. It is recommended to use this function only for custom command
     * buffers (i.e., not set using pushCommandBuffer, but with setCustomCommandBuffer).
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
    inline VkDescriptorPool getVkDescriptorPool() { return globalDescriptorPool; }
    inline void clearGraphicsPipeline() {
        graphicsPipeline = GraphicsPipelinePtr();
        lastFramebuffer = FramebufferPtr();
        recordingCommandBufferStarted = true;
    }
    [[nodiscard]] inline bool getIsCommandBufferInRecordingState() const { return isCommandBufferInRecordingState; }
    [[nodiscard]] inline const glm::mat4& getModelMatrix() const { return matrixBlock.mMatrix; }
    [[nodiscard]] inline const glm::mat4& getViewMatrix() const { return matrixBlock.vMatrix; }
    [[nodiscard]] inline const glm::mat4& getProjectionMatrix() const { return matrixBlock.pMatrix; }
    [[nodiscard]] inline const glm::mat4& getModelViewProjectionMatrix() const { return matrixBlock.mvpMatrix; }

private:
    Device* device;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffersVk;
    std::vector<sgl::vk::CommandBufferPtr> commandBuffers;
    std::vector<sgl::vk::CommandBufferPtr> frameCommandBuffers;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandBuffer customCommandBuffer = VK_NULL_HANDLE;

    // Global descriptor pool that can be used by ComputeData, RasterData and RayTracingData.
    VkDescriptorPool globalDescriptorPool;

    // Rasterizer state.
    GraphicsPipelinePtr graphicsPipeline;
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
    bool useMatrixBlock = true;
    bool matrixBlockNeedsUpdate = true;
    MatrixBlock matrixBlock;
    BufferPtr currentMatrixBlockBuffer;
    VkDescriptorSet matrixBlockDescriptorSet;
    bool useGraphicsQueue = true;
    bool recordingCommandBufferStarted = true;
    bool isCommandBufferInRecordingState = false;
    bool cachedUseGraphicsQueue = true;
    VkCommandBuffer cachedCommandBuffer = VK_NULL_HANDLE;
    bool cachedRecordingCommandBufferStarted = false;
    bool cachedIsCommandBufferInRecordingState = false;

    // Some data needs to be stored per swapchain image.
    struct FrameCache {
        CircularQueue<BufferPtr> freeCameraMatrixBuffers;
        CircularQueue<VkDescriptorSet> freeMatrixBlockDescriptorSets;
        CircularQueue<BufferPtr> allCameraMatrixBuffers;
        CircularQueue<VkDescriptorSet> allMatrixBlockDescriptorSets;
        std::vector<sgl::vk::CommandBufferPtr> frameCommandBuffers;
    };
    const uint32_t maxFrameCacheSize = 1000;
    std::vector<FrameCache> frameCaches;
    size_t frameIndex = 0;
    VkDescriptorPool matrixBufferDescriptorPool;
    VkDescriptorSetLayout matrixBufferDesciptorSetLayout;
};

}}

#endif //SGL_RENDERER_HPP
