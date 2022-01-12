/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#ifndef GRAPHICS_OPENGL_RENDERERGL_HPP_
#define GRAPHICS_OPENGL_RENDERERGL_HPP_

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Shader/Shader.hpp>

namespace sgl {

class ShaderProgramGL;

// Bound in all shaders to binding 0
struct DLL_OBJECT MatrixBlock
{
    glm::mat4 mMatrix; // Model matrix
    glm::mat4 vMatrix; // View matrix
    glm::mat4 pMatrix; // Projection matrix
    glm::mat4 mvpMatrix; // Model-view-projection matrix
};

class DLL_OBJECT RendererGL : public RendererInterface
{
public:
    RendererGL();

    //! Outputs e.g. "glGetError"
    void errorCheck() override;
    // The functions below only work in an OpenGL debug context
    /// Sets a callback function that is called (synchronously) when an error in the OpenGL context occurs
    void setErrorCallback(std::function<void()> callback) override;
    void callApplicationErrorCallback() override;
    /// Set how much error reporting the program wants
    void setDebugVerbosity(DebugVerbosity verbosity) override;

    //! Creation functions
    FramebufferObjectPtr createFBO() override;
    RenderbufferObjectPtr createRBO(
            int _width, int _height, RenderbufferType rboType, int _samples = 0) override;
    GeometryBufferPtr createGeometryBuffer(
            size_t size, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC) override;
    GeometryBufferPtr createGeometryBuffer(
            size_t size, void *data, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC) override;

    //! Functions for managing viewports/render targets
    void bindFBO(FramebufferObjectPtr _fbo, bool force = false) override;
    void unbindFBO(bool force = false) override;
    FramebufferObjectPtr getFBO() override;
    void clearFramebuffer(unsigned int buffers = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
            const Color& col = Color(0, 0, 0), float depth = 1.0f, unsigned short stencil = 0) override;
    void setCamera(CameraPtr _viewport, bool force = false) override;
    CameraPtr getCamera() override;

    //! State changes
    void bindTexture(const TexturePtr &tex, unsigned int textureUnit = 0) override;
    void setBlendMode(BlendMode mode) override;
    void setModelMatrix(const glm::mat4 &matrix) override;
    void setViewMatrix(const glm::mat4 &matrix) override;
    void setProjectionMatrix(const glm::mat4 &matrix) override;
    void setLineWidth(float width) override;
    void setPointSize(float size) override;

    //! Stencil buffer
    void enableStencilTest() override;
    void disableStencilTest() override;
    void setStencilMask(unsigned int mask) override;
    void clearStencilBuffer() override;
    void setStencilFunc(unsigned int func, int ref, unsigned int mask) override;
    void setStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass) override;

    //! Rendering
    void render(ShaderAttributesPtr &shaderAttributes) override;
    //! Rendering with overwritten shader (e.g. for multi-pass rendering without calling copy()).
    void render(ShaderAttributesPtr &shaderAttributes, ShaderProgramPtr &passShader) override;

    //! For debugging purposes
    void setPolygonMode(unsigned int polygonMode) override;
    //! For debugging purposes
    void enableWireframeMode(const Color &_wireframeColor = Color(255, 255, 255)) override;
    //! For debugging purposes
    void disableWireframeMode() override;

    // Utility functions
    void blitTexture(TexturePtr &tex, const AABB2 &renderRect, bool mirrored = false) override;
    void blitTexture(TexturePtr &tex, const AABB2 &renderRect, ShaderProgramPtr &shader, bool mirrored = false) override;
    //! Just returns tex if not multisampled
    TexturePtr resolveMultisampledTexture(TexturePtr &tex) override;
    //! Texture needs GL_LINEAR filter for best results!
    void blurTexture(TexturePtr &tex) override;
    TexturePtr getScaledTexture(TexturePtr &tex, Point2 newSize) override;
    void blitTextureFXAAAntialiased(TexturePtr &tex) override;
    std::vector<VertexTextured> createTexturedQuad(const AABB2 &renderRect, bool mirrored = false) override;

    // OpenGL-specific calls
    void bindVAO(GLuint vao);
    GLuint getVAO();
    void useShaderProgram(ShaderProgramGL *shader);
    void resetShaderProgram();

public:
    // OpenGL reuses deleted texture IDs -> "unbind" texture
    void unbindTexture(TexturePtr &tex, unsigned int textureUnit = 0);

    // Update matrix uniform buffer object
    void createMatrixBlock();
    void updateMatrixBlock();
    bool matrixBlockNeedsUpdate = true;
    MatrixBlock matrixBlock;
    sgl::GeometryBufferPtr matrixBlockBuffer;

    // For debugging purposes
    std::function<void()> applicationErrorCallback;
    //DebugVerbosity debugVerbosity;

    glm::mat4 modelMatrix, viewMatrix, projectionMatrix, mvpMatrix;
    float lineWidth, pointSize;
    bool wireframeMode;
    //! https://www.khronos.org/opengl/wiki/Debug_Output
    bool debugOutputExtEnabled;
    Color wireframeColor;
    BlendMode blendMode;
    FramebufferObjectPtr boundFBO;
    std::vector<GLuint> boundTextureID;
    GLuint currentTextureUnit = 0;
    GLuint boundFBOID = 0, boundVAO = 0, boundShader = 0;
    CameraPtr camera;

    // --- For post-processing effects ---
    ShaderProgramPtr fxaaShader, blurShader, blitShader, resolveMSAAShader, solidShader, whiteShader;
    void _setNormalizedViewProj();
    void _restoreViewProj();
    glm::mat4 oldProjMatrix, oldViewMatrix, oldModelMatrix;
    FramebufferObjectPtr oldFBO;
};

}

/*! GRAPHICS_OPENGL_RENDERERGL_HPP_ */
#endif
