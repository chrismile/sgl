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
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>
#include <Utils/CircularQueue.hpp>
#include <Math/Geometry/MatrixUtil.hpp>

namespace sgl { namespace vk {

class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class RasterData;
typedef std::shared_ptr<RasterData> RasterDataPtr;
class GraphicsPipeline;
typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;

class DLL_OBJECT Renderer {
public:
    Renderer(Device* device);
    ~Renderer();

    // Graphics pipeline.
    void beginCommandBuffer();
    VkCommandBuffer endCommandBuffer();
    void render(RasterDataPtr rasterData);
    void setModelMatrix(const glm::mat4 &matrix);
    void setViewMatrix(const glm::mat4 &matrix);
    void setProjectionMatrix(const glm::mat4 &matrix);

    // Compute pipeline.
    void dispatch(uint32_t x, uint32_t y, uint32_t z);

    // Ray tracing pipeline.
    // TODO

private:
    // TODO
    Device* device;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    // Rasterizer state.
    GraphicsPipelinePtr graphicsPipeline;
    VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    VkClearDepthStencilValue clearDepthStencil = { 1.0f, 0 };

    // Global state.
    void updateMatrixBlock();
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

    // Some data needs to be stored per swapchain image.
    struct FrameCache {
        CircularQueue<BufferPtr> freeCameraMatrixBuffers;
        CircularQueue<BufferPtr> allCameraMatrixBuffers;
    };
    std::vector<FrameCache> frameCaches;
    size_t frameIndex = 0;
};

}}

#endif //SGL_RENDERER_HPP
