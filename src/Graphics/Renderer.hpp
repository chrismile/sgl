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

#ifndef GRAPHICS_RENDERER_HPP_
#define GRAPHICS_RENDERER_HPP_

#include <functional>
#include <memory>

#ifdef USE_GLM
#include <glm/fwd.hpp>
#else
#include <Math/Geometry/fallback/fwd.hpp>
#endif

#include <Defs.hpp>
#include <Math/Geometry/AABB2.hpp>
#include <Math/Geometry/Point2.hpp>
#include <Graphics/Color.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <Graphics/Buffers/RBO.hpp>
#include <Graphics/Buffers/GeometryBuffer.hpp>
#include <Graphics/Mesh/Vertex.hpp>

namespace sgl {

class FramebufferObject;
typedef std::shared_ptr<FramebufferObject> FramebufferObjectPtr;
class RenderbufferObject;
typedef std::shared_ptr<RenderbufferObject> RenderbufferObjectPtr;
class Camera;
typedef std::shared_ptr<Camera> CameraPtr;
class ShaderProgram;
typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;
class ShaderAttributes;
typedef std::shared_ptr<ShaderAttributes> ShaderAttributesPtr;

enum BlendMode {
    BLEND_OVERWRITE, BLEND_ALPHA, BLEND_ADDITIVE, BLEND_SUBTRACTIVE, BLEND_MODULATIVE
};

enum DebugVerbosity {
    DEBUG_OUTPUT_CRITICAL_ONLY = 0,
    DEBUG_OUTPUT_MEDIUM_AND_ABOVE = 1,
    DEBUG_OUTPUT_LOW_AND_ABOVE = 2,
    DEBUG_OUTPUT_NOTIFICATION_AND_ABOVE = 3
};

#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif


/*! There are three classes/global objects in this engine that are responsible for creating
 *  and rendering graphics objects:
 *  1. ShaderManager: Load and create shader programs and shader attributes
 *  2. TextureManager: Create all sorts of textures that can be attached to shaders
 *  3. Renderer: Create framebuffer objects (FBOs), renderbuffer objects (RBOs) and geometry buffers
 *  Furthermore, Renderer is responsible for everything else that is mid-level rendering-related:
 *  Binding above-mentioned objects, changing rendering modes, blitting textures, and rendering itself.
 *  Only high-level functions are outsourced to other classes (e.g. font rendering).
 */

class DLL_OBJECT RendererInterface {
public:
    virtual ~RendererInterface() = default;

    /// Outputs e.g. "glGetError" (only necessary if no debug context was created)
    virtual void errorCheck()=0;
    // The functions below only work in an OpenGL debug context
    /// Sets a callback function that is called (synchronously) when an error in the OpenGL context occurs
    virtual void setErrorCallback(std::function<void()> callback)=0;
    virtual void callApplicationErrorCallback()=0;
    /// Set how much error reporting the program wants
    virtual void setDebugVerbosity(DebugVerbosity verbosity)=0;

    //! Creation functions
    virtual FramebufferObjectPtr createFBO()=0;
    virtual RenderbufferObjectPtr createRBO(int _width, int _height, RenderbufferType rboType, int _samples = 0)=0;
    virtual GeometryBufferPtr createGeometryBuffer(size_t size, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC)=0;
    virtual GeometryBufferPtr createGeometryBuffer(size_t size, void *data, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC)=0;

    //! Functions for managing viewports/render targets
    virtual void bindFBO(FramebufferObjectPtr _fbo, bool force = false)=0;
    virtual void unbindFBO(bool force = false)=0;
    virtual FramebufferObjectPtr getFBO()=0;
    virtual void clearFramebuffer(unsigned int buffers = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
            const Color& col = Color(0, 0, 0), float depth = 1.0f, unsigned short stencil = 0)=0;
    virtual void setCamera(CameraPtr _viewport, bool force = false)=0;
    virtual CameraPtr getCamera()=0;

    //! State changes
    virtual void bindTexture(const TexturePtr &tex, unsigned int textureUnit = 0)=0;
    virtual void setBlendMode(BlendMode mode)=0;
    virtual void setModelMatrix(const glm::mat4 &matrix)=0;
    virtual void setViewMatrix(const glm::mat4 &matrix)=0;
    virtual void setProjectionMatrix(const glm::mat4 &matrix)=0;
    virtual void setLineWidth(float width)=0;
    virtual void setPointSize(float size)=0;

    //! Stencil buffer
    virtual void enableStencilTest()=0;
    virtual void disableStencilTest()=0;
    virtual void setStencilMask(unsigned int mask)=0;
    virtual void clearStencilBuffer()=0;
    virtual void setStencilFunc(unsigned int func, int ref, unsigned int mask)=0;
    virtual void setStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass)=0;

    //! Rendering
    virtual void render(ShaderAttributesPtr &shaderAttributes)=0;
    //! Rendering with overwritten shader (e.g. for multi-pass rendering without calling copy()).
    virtual void render(ShaderAttributesPtr &shaderAttributes, ShaderProgramPtr &passShader)=0;

    //! For debugging purposes
    virtual void setPolygonMode(unsigned int polygonMode)=0;
    //! For debugging purposes
    virtual void enableWireframeMode(const Color &_wireframeColor = Color(255,255,255))=0;
    //! For debugging purposes
    virtual void disableWireframeMode()=0;

    //! Utility functions
    virtual void blitTexture(TexturePtr &tex, const AABB2 &renderRect, bool mirrored = false)=0;
    virtual void blitTexture(TexturePtr &tex, const AABB2 &renderRect, ShaderProgramPtr &shader, bool mirrored = false)=0;
    //! Just returns tex if not multisampled
    virtual TexturePtr resolveMultisampledTexture(TexturePtr &tex)=0;
    //! Texture needs GL_LINEAR filter for best results!
    virtual void blurTexture(TexturePtr &tex)=0;
    virtual TexturePtr getScaledTexture(TexturePtr &tex, Point2 newSize)=0;
    virtual void blitTextureFXAAAntialiased(TexturePtr &tex)=0;
    virtual std::vector<VertexTextured> createTexturedQuad(const AABB2 &renderRect, bool mirrored = false)=0;
};

DLL_OBJECT extern RendererInterface *Renderer;

}

/*! GRAPHICS_RENDERER_HPP_ */
#endif
