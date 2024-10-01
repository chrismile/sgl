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

#ifndef SGL_WEBGPU_RENDERPIPELINE_HPP
#define SGL_WEBGPU_RENDERPIPELINE_HPP

#include <memory>
#include <webgpu/webgpu.h>

#include "../Shader/Reflect/WGSLReflect.hpp"

namespace sgl { namespace webgpu {

class Device;
class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;
class Framebuffer;
typedef std::shared_ptr<Framebuffer> FramebufferPtr;

enum class BlendMode {
    // No blending.
    OVERWRITE,
    // Alpha blending.
    BACK_TO_FRONT_STRAIGHT_ALPHA, BACK_TO_FRONT_PREMUL_ALPHA, FRONT_TO_BACK_PREMUL_ALPHA,
    // Additive blending modes & multiplicative blending.
    BACK_ADDITIVE, ONE, BACK_SUBTRACTIVE, BACK_MULTIPLICATIVE,
    // Custom blend mode specified manually.
    CUSTOM
};

/// Cf. WGPUPrimitiveTopology
enum class PrimitiveTopology {
    POINT_LIST = WGPUPrimitiveTopology_PointList,
    LINE_LIST = WGPUPrimitiveTopology_LineList,
    LINE_STRIP = WGPUPrimitiveTopology_LineStrip,
    TRIANGLE_LIST = WGPUPrimitiveTopology_TriangleList,
    TRIANGLE_STRIP = WGPUPrimitiveTopology_TriangleStrip,
};

/// @see VkCullModeFlagBits
enum class CullMode {
    CULL_NONE = WGPUCullMode_None,
    CULL_FRONT = WGPUCullMode_Front,
    CULL_BACK = WGPUCullMode_Back,
    CULL_FRONT_AND_BACK = WGPUCullMode_Front | WGPUCullMode_Back,
};

class DLL_OBJECT RenderPipelineInfo {
    friend class RenderPipeline;

public:
    explicit RenderPipelineInfo(const ShaderStagesPtr& shaderStages);

    /**
     * Resets to standard settings.
     * - Primitive data: Triangle list.
     */
    void reset();

    void addConstantEntry(ShaderType shaderType, const std::string& key, double value);
    void removeConstantEntry(ShaderType shaderType, const std::string& key);

    /// Sets the specified framebuffer (REQUIRED).
    void setFramebuffer(const FramebufferPtr& framebuffer);

    // Color info.
    void setColorWriteEnabled(bool enableColorWrite, uint32_t colorAttachmentIndex = 0);
    void setBlendMode(BlendMode blendMode, uint32_t colorAttachmentIndex = 0);
    void setBlendModeCustom(
            WGPUBlendFactor srcColorBlendFactor, WGPUBlendFactor dstColorBlendFactor, WGPUBlendOperation colorBlendOp,
            WGPUBlendFactor srcAlphaBlendFactor, WGPUBlendFactor dstAlphaBlendFactor, WGPUBlendOperation alphaBlendOp,
            uint32_t colorAttachmentIndex = 0);
    inline BlendMode getBlendMode(uint32_t colorAttachmentIndex = 0) {
        return currentBlendModes.at(colorAttachmentIndex);
    }
    [[nodiscard]] inline bool getIsBlendEnabled(uint32_t colorAttachmentIndex = 0) const {
        return currentBlendModes.at(colorAttachmentIndex) != BlendMode::OVERWRITE;
    }

    /*
     * https://www.w3.org/TR/webgpu/#vertex-state
     * Primitive restart value for strips:
     * - uint16 -> 0xFFFF
     * - uint32 -> 0xFFFFFFFF
     */
    void setPrimitiveTopology(
            PrimitiveTopology primitiveTopology, WGPUIndexFormat stripIndexFormat = WGPUIndexFormat_Uint32);
    void setCullMode(CullMode cullMode);
    void setIsFrontFaceCcw(bool isFrontFaceCcw);

    /**
     * In Vulkan, the coordinate origin is usually at the top left corner of the viewport.
     * In Vulkan >= 1.1 (or when using VK_KHR_maintenance1), it is possible to move it to the bottom left.
     */
    [[nodiscard]] inline bool getUseCoordinateOriginBottomLeft() const { return coordinateOriginBottomLeft; }

    // Depth-stencil info.
    void setDepthTestEnabled(bool enableDepthTest);
    void setDepthWriteEnabled(bool enableDepthWrite);
    void setDepthCompareFunction(WGPUCompareFunction compareFunction);
    [[nodiscard]] inline bool getDepthWriteEnabled() const { return depthStencilState.depthWriteEnabled; }
    [[nodiscard]] inline WGPUCompareFunction getDepthCompareFunction() const { return depthStencilState.depthCompare; }

    /**
     * E.g., if we have struct Vertex { vec3 vertexPosition; float vertexAttribute; };
     * - addVertexBufferBinding(0, sizeof(Vertex));
     * @param binding The binding point of the vertex buffer.
     * @param stride The stride of one vertex in bytes.
     * @param stepMode The step mode (either 'vertex' or 'instance').
     */
    void setVertexBufferBinding(
            uint32_t binding, uint32_t stride, WGPUVertexStepMode stepMode = WGPUVertexStepMode_Vertex);

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

    void setVertexBufferBindingByLocationIndex(
            const std::string& attributeName, uint32_t stride, WGPUVertexStepMode stepMode = WGPUVertexStepMode_Vertex);

    void setVertexBufferBindingByLocationIndexOptional(
            const std::string& attributeName, uint32_t stride, WGPUVertexStepMode stepMode = WGPUVertexStepMode_Vertex);

protected:
    void _resizeColorTargets(size_t newCount);

    ShaderStagesPtr shaderStages;
    std::unordered_map<ShaderType, std::map<std::string, double>> constantEntriesMap;
    FramebufferPtr framebuffer;
    std::vector<BlendMode> currentBlendModes;

    std::vector<std::vector<WGPUVertexAttribute>> vertexBuffersAttributes;
    std::vector<WGPUVertexBufferLayout> vertexBufferLayouts;

    bool coordinateOriginBottomLeft = false;

    WGPUPrimitiveState primitiveState = {};
    WGPUMultisampleState multisampleState = {};
    WGPUDepthStencilState depthStencilState = {};
    std::vector<WGPUBlendState> blendStates;
    std::vector<WGPUColorTargetState> colorTargetStates;
};

class DLL_OBJECT RenderPipeline {
public:
    RenderPipeline(Device* device, const RenderPipelineInfo& pipelineInfo);
    ~RenderPipeline();

    [[nodiscard]] inline Device* getDevice() { return device; }
    [[nodiscard]] inline ShaderStagesPtr& getShaderStages() { return shaderStages; }
    [[nodiscard]] inline const ShaderStagesPtr& getShaderStages() const { return shaderStages; }

    [[nodiscard]] inline const FramebufferPtr& getFramebuffer() const { return framebuffer; }
    // Framebuffer must be compatible with the render pass.
    inline void setCompatibleFramebuffer(const FramebufferPtr& _framebuffer) { framebuffer = _framebuffer; }
    [[nodiscard]] inline bool getDepthWriteEnabled() const { return depthWriteEnabled; }
    [[nodiscard]] inline bool getStencilWriteEnabled() const { return stencilWriteEnabled; }

    [[nodiscard]] inline const std::vector<uint32_t>& getVertexBufferStrides() { return vertexBufferStrides; }

    [[nodiscard]] inline WGPURenderPipeline getWGPURenderPipeline() { return pipeline; }

protected:
    Device* device;
    ShaderStagesPtr shaderStages;
    WGPUPipelineLayout pipelineLayout{};
    WGPURenderPipeline pipeline = {};
    FramebufferPtr framebuffer;
    bool depthWriteEnabled = true;
    bool stencilWriteEnabled = true;
    std::vector<uint32_t> vertexBufferStrides;
};

typedef std::shared_ptr<RenderPipeline> RenderPipelinePtr;

}}

#endif //SGL_WEBGPU_RENDERPIPELINE_HPP
