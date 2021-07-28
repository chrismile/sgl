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

#ifndef SGL_GRAPHICSPIPELINE_HPP
#define SGL_GRAPHICSPIPELINE_HPP

#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

#include "Pipeline.hpp"

namespace sgl { namespace vk {

/// Cf. VkPrimitiveTopology
enum class InputAssemblyTopology {
    POINT_LIST = 0,
    LINE_LIST = 1,
    LINE_STRIP = 2,
    TRIANGLE_LIST = 3,
    TRIANGLE_STRIP = 4,
    TRIANGLE_FAN = 5,
    LINE_LIST_WITH_ADJACENCY = 6,
    LINE_STRIP_WITH_ADJACENCY = 7,
    TRIANGLE_LIST_WITH_ADJACENCY = 8,
    TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    PATCH_LIST = 10,
};

class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;

enum class BlendMode {
    // No blending
    OVERWRITE,
    // Alpha blending
    BACK_TO_FRONT_STRAIGHT_ALPHA, BACK_TO_FRONT_PREMUL_ALPHA, FRONT_TO_BACK_PREMUL_ALPHA,
    // Additive blending modes & multiplicative blending
    BACK_ADDITIVE, BACK_SUBTRACTIVE, BACK_MULTIPLICATIVE
};

/// @see VkPrimitiveTopology
enum class PrimitiveTopology {
    POINT_LIST = 0,
    LINE_LIST = 1,
    LINE_STRIP = 2,
    TRIANGLE_LIST = 3,
    TRIANGLE_STRIP = 4,
    TRIANGLE_FAN = 5,
    LINE_LIST_WITH_ADJACENCY = 6,
    LINE_STRIP_WITH_ADJACENCY = 7,
    TRIANGLE_LIST_WITH_ADJACENCY = 8,
    TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    PATCH_LIST = 10
};

/// @see VkCullModeFlagBits
enum class CullMode {
    CULL_NONE = 0,
    CULL_FRONT = 0x00000001,
    CULL_BACK = 0x00000002,
    CULL_FRONT_AND_BACK = 0x00000003,
};

class DLL_OBJECT GraphicsPipelineInfo {
    friend class GraphicsPipeline;

public:
    explicit GraphicsPipelineInfo(const ShaderStagesPtr& shaderStages);

    /**
     * Resets to standard settings.
     * - Primitive data: Triangle list.
     */
    void reset();

    /// Sets the specified framebuffer (REQUIRED).
    void setFramebuffer(FramebufferPtr framebuffer, uint32_t subpassIndex = 0);

    void setBlendMode(BlendMode blendMode);
    void setInputAssemblyTopology(PrimitiveTopology primitiveTopology, bool primitiveRestartEnable = false);
    void setCullMode(CullMode cullMode);
    void setIsFrontFaceCcw(bool isFrontFaceCcw);
    void setMinSampleShading(bool enableMinSampleShading, float minSampleShading = 1.0f);
    inline BlendMode getBlendMode() { return currentBlendMode; }
    inline bool getIsBlendEnabled() { return currentBlendMode != BlendMode::OVERWRITE; }

    // Depth-stencil info.
    void setDepthTestEnabled(bool enableDepthTest);
    void setDepthWriteEnabled(bool enableDepthWrite);
    void setStencilTestEnabled(bool enableStencilTest);
    inline bool getDepthTestEnabled() const { return depthStencilInfo.depthTestEnable; }
    inline bool getDepthWriteEnabled() const { return depthStencilInfo.depthWriteEnable; }
    inline bool getStencilTestEnabled() const { return depthStencilInfo.stencilTestEnable; }

    /**
     * E.g., if we have struct Vertex { vec3 vertexPosition; float vertexAttribute; };
     * - addVertexBufferBinding(0, sizeof(Vertex));
     * @param binding The binding point of the vertex buffer.
     * @param stride The stride of one vertex in bytes.
     * @param inputRate The input rate (either 'vertex' or 'instance').
     */
    void setVertexBufferBinding(
            uint32_t binding, uint32_t stride, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

    /**
     * Specifies that an attribute should be read from an vertex buffer with the specified binding point.
     * The offset specifies the element offset in byte used for reading from the buffer.
     * Example for "layout(location = 1) in float vertexAttribute;" (for same vertex data as above):
     * - setInputAttributeDescription(0, offsetof(Vertex, vertexAttribute), 1);
     * @param bufferBinding The binding point of the vertex buffer (@see setVertexBufferBinding).
     * @param bufferOffset The element offset in byte used for reading from the buffer.
     * @param attributeLocation The shader location of the input attribute.
     */
    void setInputAttributeDescription(uint32_t bufferBinding, uint32_t bufferOffset, uint32_t attributeLocation);

    /**
     * Specifies that an attribute should be read from an vertex buffer with the specified binding point.
     * The offset specifies the element offset in byte used for reading from the buffer.
     * Example for "layout(location = 1) in float vertexAttribute;" (for same vertex data as above):
     * - setInputAttributeDescription(0, offsetof(Vertex, vertexAttribute), "vertexAttribute");
     * @param bufferBinding The binding point of the vertex buffer (@see setVertexBufferBinding).
     * @param bufferOffset The element offset in byte used for reading from the buffer.
     * @param attributeName The name of the input attribute in the shader.
     */
    void setInputAttributeDescription(uint32_t bufferBinding, uint32_t bufferOffset, const std::string& attributeName);

    // Getters for VkPipeline*StateCreateInfo data.
    VkPipelineViewportStateCreateInfo& getViewportStateCreateInfo() { return viewportStateInfo; }

protected:
    ShaderStagesPtr shaderStages;
    FramebufferPtr framebuffer;
    uint32_t subpassIndex = 0;
    VkRenderPassCreateInfo renderPassInfo = {};
    BlendMode currentBlendMode = BlendMode::OVERWRITE;

    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

    VkViewport viewport = {};
    VkRect2D scissor = {};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    VkPipelineViewportStateCreateInfo viewportStateInfo = {};
    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
    VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
};

class DLL_OBJECT GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline(Device* device, const GraphicsPipelineInfo& pipelineInfo);

    inline const FramebufferPtr& getFramebuffer() const { return framebuffer; }
    inline uint32_t getSubpassIndex() const { return subpassIndex; }

    // Used by RasterData to make deduce number of vertices from buffer byte size.
    inline const std::vector<VkVertexInputBindingDescription>& getVertexInputBindingDescriptions() const {
        return vertexInputBindingDescriptions;
    }
    inline const std::vector<VkVertexInputAttributeDescription>& getVertexBindingAttributeDescriptions() const {
        return vertexInputAttributeDescriptions;
    }

protected:
    FramebufferPtr framebuffer;
    uint32_t subpassIndex = 0;

    // Used by RasterData to make deduce number of vertices from buffer byte size.
    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
};

typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;

}}

#endif //SGL_GRAPHICSPIPELINE_HPP
