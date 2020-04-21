/*
 * RendererGL.cpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include "RendererGL.hpp"
#include <Math/Geometry/Point2.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Scene/Camera.hpp>
#include <Graphics/Scene/RenderTarget.hpp>
#include <Graphics/Window.hpp>
#include "SystemGL.hpp"
#include "FBO.hpp"
#include "RBO.hpp"
#include "ShaderManager.hpp"
#include "ShaderAttributes.hpp"
#include "GeometryBuffer.hpp"
#include "Texture.hpp"

namespace sgl {

std::string getErrorSeverityString(GLenum severity)
{
    const std::map<GLenum, std::string> severityMap = {
            { GL_DEBUG_SEVERITY_HIGH,         "High" },
            { GL_DEBUG_SEVERITY_MEDIUM,       "Medium" },
            { GL_DEBUG_SEVERITY_LOW,          "Low" },
            { GL_DEBUG_SEVERITY_NOTIFICATION, "Notification" }
    };
    return severityMap.at(severity);
}

std::string getErrorSourceString(GLenum source)
{
    const std::map<GLenum, std::string> sourceMap = {
            { GL_DEBUG_SOURCE_API, "OpenGL API" },
            { GL_DEBUG_SOURCE_WINDOW_SYSTEM, "Window System" },
            { GL_DEBUG_SOURCE_SHADER_COMPILER, "Shader Compiler" },
            { GL_DEBUG_SOURCE_THIRD_PARTY, "Third Party" },
            { GL_DEBUG_SOURCE_APPLICATION, "Application" },
            { GL_DEBUG_SOURCE_OTHER, "Other" }
    };
    return sourceMap.at(source);
}

std::string getErrorTypeString(GLenum type)
{
    const std::map<GLenum, std::string> typeMap = {
            { GL_DEBUG_TYPE_ERROR, "API Error" },
            { GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "Deprecated Behavior" },
            { GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "Undefined Behavior" },
            { GL_DEBUG_TYPE_PORTABILITY, "Non-Portable Functionality" },
            { GL_DEBUG_TYPE_PERFORMANCE, "Bad Performance" },
            { GL_DEBUG_TYPE_MARKER, "Command Stream Annotation" },
            { GL_DEBUG_TYPE_PUSH_GROUP, "Group Pushing" },
            { GL_DEBUG_TYPE_POP_GROUP, "Group Popping" },
            { GL_DEBUG_TYPE_OTHER, "Other" }
    };
    return typeMap.at(type);
}

// Uses KHR_debug. For more information see https://www.khronos.org/opengl/wiki/Debug_Output.
void openglErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
        const void* userParam)
{
    Logfile::get()->writeError("OpenGL Error:");
    Logfile::get()->writeError("=============");
    Logfile::get()->writeError(std::string() + " Message ID: " + sgl::toString(id));
    Logfile::get()->writeError(std::string() + " Severity: " + getErrorSeverityString(severity));
    Logfile::get()->writeError(std::string() + " Type: " + getErrorTypeString(type));
    Logfile::get()->writeError(std::string() + " Source: " + getErrorSourceString(source));
    Logfile::get()->writeError(std::string() + " Message: " + message);
    Logfile::get()->writeError("");

    Renderer->callApplicationErrorCallback();
}

RendererGL::RendererGL()
{
    blendMode = BLEND_OVERWRITE;
    setBlendMode(BLEND_ALPHA);
    boundTextureID.resize(32, 0);

    modelMatrix = viewMatrix = projectionMatrix = mvpMatrix = matrixIdentity();
    lineWidth = 1.0f;
    pointSize = 1.0f;
    currentTextureUnit = 0;
    boundFBOID = 0;
    boundVAO = 0;
    boundShader = 0;
    wireframeMode = false;
    debugOutputExtEnabled = false;

    createMatrixBlock();

    if (FileUtils::get()->exists("Data/Shaders/FXAA.glsl"))
        fxaaShader = ShaderManager->getShaderProgram({"FXAA.Vertex", "FXAA.Fragment"});
    if (FileUtils::get()->exists("Data/Shaders/GaussianBlur.glsl"))
        blurShader = ShaderManager->getShaderProgram({"GaussianBlur.Vertex", "GaussianBlur.Fragment"});
    blitShader = ShaderManager->getShaderProgram({"Blit.Vertex", "Blit.Fragment"});
    resolveMSAAShader = ShaderManager->getShaderProgram({"ResolveMSAA.Vertex", "ResolveMSAA.Fragment"});
    solidShader = ShaderManager->getShaderProgram({"Mesh.Vertex.Plain", "Mesh.Fragment.Plain"});
    whiteShader = ShaderManager->getShaderProgram({"WhiteSolid.Vertex", "WhiteSolid.Fragment"});

    // https://www.khronos.org/opengl/wiki/Debug_Output
    if ((SystemGL::get()->isGLExtensionAvailable("ARB_debug_output")
            || SystemGL::get()->isGLExtensionAvailable("KHR_debug")
            || SystemGL::get()->openglVersionMinimum(4, 3))
            && AppSettings::get()->getMainWindow()->isDebugContext()) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        setDebugVerbosity(DEBUG_OUTPUT_MEDIUM_AND_ABOVE);
        glDebugMessageCallback((GLDEBUGPROC)openglErrorCallback, NULL);
        debugOutputExtEnabled = true;
    }
}

void RendererGL::setErrorCallback(std::function<void()> callback)
{
    applicationErrorCallback = callback;
}

void RendererGL::callApplicationErrorCallback()
{
    if (applicationErrorCallback) {
        applicationErrorCallback();
    }
}

void RendererGL::setDebugVerbosity(DebugVerbosity verbosity)
{
    GLboolean activeHigh = GL_TRUE, activeMedium = GL_FALSE, activeLow = GL_FALSE, activeNotification = GL_FALSE;
    if ((int)verbosity > 0) {
        activeMedium = GL_TRUE;
    }
    if ((int)verbosity > 1) {
        activeLow = GL_TRUE;
    }
    if ((int)verbosity > 2) {
        activeNotification = GL_TRUE;
    }
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, activeHigh);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, activeMedium);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, activeLow);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, activeNotification);
}

std::vector<std::string> getErrorMessages() {
    int numMessages = 10;

    GLint maxMessageLen = 0;
    glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &maxMessageLen);

    std::vector<GLchar> messageText(numMessages * maxMessageLen);
    std::vector<GLenum> sources(numMessages);
    std::vector<GLenum> types(numMessages);
    std::vector<GLuint> ids(numMessages);
    std::vector<GLenum> severities(numMessages);
    std::vector<GLsizei> lengths(numMessages);

    GLuint numFound = glGetDebugMessageLog(numMessages, messageText.capacity(), &sources[0],
            &types[0], &ids[0], &severities[0], &lengths[0], &messageText[0]);

    sources.resize(numFound);
    types.resize(numFound);
    severities.resize(numFound);
    ids.resize(numFound);
    lengths.resize(numFound);

    std::vector<std::string> messages;
    messages.reserve(numFound);

    std::vector<GLchar>::iterator currPos = messageText.begin();
    for(size_t i = 0; i < lengths.size(); ++i) {
        messages.push_back(std::string(currPos, currPos + lengths[i] - 1));
        currPos = currPos + lengths[i];
    }

    return messages;
}

void RendererGL::errorCheck()
{
    // Check for errors
    GLenum oglError = glGetError();
    if (oglError != GL_NO_ERROR) {
        Logfile::get()->writeError(std::string() + "OpenGL error: " + toString(oglError));

        auto messages = getErrorMessages();
        for (const std::string &msg : messages) {
            Logfile::get()->writeError(std::string() + "Error message: " + msg);
        }
    }
}

// Creation functions
FramebufferObjectPtr RendererGL::createFBO()
{
    if (SystemGL::get()->openglVersionMinimum(3,2)) {
        return FramebufferObjectPtr(new FramebufferObjectGL);
    }
    return FramebufferObjectPtr(new FramebufferObjectGL2);
}

RenderbufferObjectPtr RendererGL::createRBO(int _width, int _height, RenderbufferType rboType, int _samples /* = 0 */)
{
    return RenderbufferObjectPtr(new RenderbufferObjectGL(_width, _height, rboType, _samples));
}

GeometryBufferPtr RendererGL::createGeometryBuffer(size_t size, BufferType type /* = VERTEX_BUFFER */, BufferUse bufferUse /* = BUFFER_STATIC */)
{
    GeometryBufferPtr geomBuffer(new GeometryBufferGL(size, type, bufferUse));
    return geomBuffer;
}

GeometryBufferPtr RendererGL::createGeometryBuffer(size_t size, void *data, BufferType type /* = VERTEX_BUFFER */, BufferUse bufferUse /* = BUFFER_STATIC */)
{
    GeometryBufferPtr geomBuffer(new GeometryBufferGL(size, data, type, bufferUse));
    return geomBuffer;
}


// Functions for managing viewports/render targets
void RendererGL::bindFBO(FramebufferObjectPtr _fbo, bool force /* = false */)
{
    if (boundFBO.get() != _fbo.get() || force) {
        boundFBO = _fbo;
        if (_fbo.get() != NULL) {
            boundFBOID = _fbo->_bindInternal();
        } else {
            unbindFBO(true);
        }
    }
}

void RendererGL::unbindFBO(bool force /* = false */)
{
    if (boundFBO.get() != 0 || force) {
        boundFBO = FramebufferObjectPtr();
        boundFBOID = 0;
        if (SystemGL::get()->openglVersionMinimum(3,2)) {
            glBindFramebuffer(GL_FRAMEBUFFER, boundFBOID);
        } else {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, boundFBOID);
        }
    }
}

FramebufferObjectPtr RendererGL::getFBO()
{
    return boundFBO;
}

void RendererGL::clearFramebuffer(unsigned int buffers /* = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT */,
        const Color& col /* = Color(0, 0, 0) */, float depth /* = 0.0f */, unsigned short stencil /* = 0*/ )
{
    if ((buffers & GL_COLOR_BUFFER_BIT) != 0)
        glClearColor(col.getFloatR(), col.getFloatG(), col.getFloatB(), col.getFloatA());
    if ((buffers & GL_DEPTH_BUFFER_BIT) != 0)
        glClearDepth(depth);
    if ((buffers & GL_STENCIL_BUFFER_BIT) != 0)
        glClearStencil(stencil);
    glClear(buffers);
}

void RendererGL::setCamera(CameraPtr _camera, bool force)
{
    if (camera != _camera || force) {
        camera = _camera;
        glm::ivec4 ltwh =  camera->getViewportLTWH();
        glViewport(ltwh.x, ltwh.y, ltwh.z, ltwh.w); // left, top, width, height

        RenderTargetPtr target = camera->getRenderTarget();
        target->bindRenderTarget();
    }
}

CameraPtr RendererGL::getCamera()
{
    return camera;
}


// State changes
void RendererGL::bindTexture(TexturePtr &tex, unsigned int textureUnit /* = 0 */)
{
    TextureGL *textureGL = (TextureGL*)tex.get();

    // TODO: OpenGL reuses texture IDs after deletion. Remove bound texture ID!
    //if (boundTextureID.at(textureUnit) != textureGL->getTexture()) {
        boundTextureID.at(textureUnit) = textureGL->getTexture();

        if (currentTextureUnit != textureUnit) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            currentTextureUnit = textureUnit;
        }

        if (tex->getTextureType() == TEXTURE_3D) {
            glBindTexture(GL_TEXTURE_3D, textureGL->getTexture());
        } else if (tex->getTextureType() == TEXTURE_2D_ARRAY) {
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureGL->getTexture());
        } else if (tex->getTextureType() == TEXTURE_1D) {
            glBindTexture(GL_TEXTURE_1D, textureGL->getTexture());
        } else if (tex->getNumSamples() == 0) {
            glBindTexture(GL_TEXTURE_2D, textureGL->getTexture());
        } else {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureGL->getTexture());
        }

        if (tex->hasManualDepthStencilComponentMode()) {
            if (tex->hasDepthComponentMode()) {
                glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
            } else if (tex->hasStencilComponentMode()) {
                glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
            }
        }
    //}
}

void RendererGL::unbindTexture(TexturePtr &tex, unsigned int textureUnit /* = 0 */)
{
    TextureGL *textureGL = (TextureGL*)tex.get();

    if (boundTextureID.at(textureUnit) == textureGL->getTexture()) {
        boundTextureID.at(textureUnit) = 0;
    }
}

void RendererGL::setBlendMode(BlendMode mode)
{
    if (mode == blendMode)
        return;
    /*if (mode == blendMode || (SystemGL::get()->isPremulAphaEnabled()
            && ((mode == BLEND_ALPHA && blendMode == BLEND_ADDITIVE)
                    || (mode == BLEND_ADDITIVE && blendMode == BLEND_ALPHA))))
        return;*/

    if (SystemGL::get()->isPremulAphaEnabled()) {
        if (mode == BLEND_OVERWRITE) {
            // Disables blending of textures with the scene
            glDisable(GL_BLEND);
        } else if (mode == BLEND_ALPHA) {
            // This allows alpha blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);
            // TODO: glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);?
        } else if (mode == BLEND_ADDITIVE) {
            // This allows additive blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
        } else if (mode == BLEND_SUBTRACTIVE) {
            // This allows additive blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        } else if (mode == BLEND_MODULATIVE) {
            // This allows modulative blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            glBlendEquation(GL_FUNC_ADD);
        }
    } else {
        if (mode == BLEND_OVERWRITE) {
            // Disables blending of textures with the scene
            glDisable(GL_BLEND);
        } else if (mode == BLEND_ALPHA) {
            // This allows alpha blending of textures with the scene
            glEnable(GL_BLEND);
            //glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            //glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // PREMUL: TODO
            /*glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);*/

        } else if (mode == BLEND_ADDITIVE) {
            // This allows additive blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
        } else if (mode == BLEND_SUBTRACTIVE) {
            // This allows additive blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        } else if (mode == BLEND_MODULATIVE) {
            // This allows modulative blending of textures with the scene
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            glBlendEquation(GL_FUNC_ADD);
        }
    }

    // BLEND_OVERWRITE, BLEND_ALPHA, BLEND_ADDITIVE, BLEND_MODULATIVE
    blendMode = mode;
}

void RendererGL::setModelMatrix(const glm::mat4 &matrix)
{
    modelMatrix = matrix;
    matrixBlockNeedsUpdate = true;
}

void RendererGL::setViewMatrix(const glm::mat4 &matrix)
{
    viewMatrix = matrix;
    matrixBlockNeedsUpdate = true;
}

void RendererGL::setProjectionMatrix(const glm::mat4 &matrix)
{
    projectionMatrix = matrix;
    matrixBlockNeedsUpdate = true;
}

void RendererGL::setLineWidth(float width)
{
    if (width != lineWidth) {
        lineWidth = width;
        glLineWidth(width);
    }
}

void RendererGL::setPointSize(float size)
{
    if (size != pointSize) {
        pointSize = size;
        glPointSize(size);
    }
}


// Stencil buffer
void RendererGL::enableStencilTest()
{
    glEnable(GL_STENCIL_TEST);
}

void RendererGL::disableStencilTest()
{
    glDisable(GL_STENCIL_TEST);
}

void RendererGL::setStencilMask(unsigned int mask)
{
    glStencilMask(mask);
}

void RendererGL::clearStencilBuffer()
{
    glClear(GL_STENCIL_BUFFER_BIT);
}

void RendererGL::setStencilFunc(unsigned int func, int ref, unsigned int mask)
{
    glStencilFunc(func, ref, mask);
}

void RendererGL::setStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass)
{
    glStencilOp(sfail, dpfail, dppass);
}


// Rendering
void RendererGL::render(ShaderAttributesPtr &shaderAttributes)
{
    ShaderAttributesPtr attr = shaderAttributes;
    if (wireframeMode) {
        // Not the most performat solution, but wireframe mode is for debugging anyway
        attr = shaderAttributes->copy(solidShader);
    }

    attr->bind();
    updateMatrixBlock();
    //attr->setModelViewProjectionMatrices(modelMatrix, viewMatrix, projectionMatrix, mvpMatrix);

    if (attr->getNumIndices() > 0) {
        if (attr->getInstanceCount() == 0) {
            // Indices, no instancing
            /*glDrawRangeElements((GLuint)attr->getVertexMode(), 0, attr->getNumVertices()-1,
                    attr->getNumIndices(), attr->getIndexFormat(), NULL);*/
            glDrawElements((GLuint)attr->getVertexMode(), attr->getNumIndices(), attr->getIndexFormat(), NULL);
        } else {
            // Indices, instancing
            glDrawElementsInstanced((GLuint)attr->getVertexMode(), attr->getNumIndices(),
                    attr->getIndexFormat(), NULL, attr->getInstanceCount());
        }
    } else {
        if (attr->getInstanceCount() == 0) {
            // No indices, no instancing
            glDrawArrays((GLuint)attr->getVertexMode(), 0, attr->getNumVertices());
        } else {
            // No indices, instancing
            glDrawArraysInstanced((GLuint)attr->getVertexMode(), 0, attr->getNumVertices(), attr->getInstanceCount());
        }
    }
}

void RendererGL::render(ShaderAttributesPtr &shaderAttributes, ShaderProgramPtr &passShader)
{
    ShaderAttributesPtr attr = shaderAttributes;
    if (wireframeMode) {
        // Not the most performat solution, but wireframe mode is for debugging anyway
        attr = shaderAttributes->copy(solidShader);
    }

    attr->bind(passShader);
    updateMatrixBlock();
    //attr->setModelViewProjectionMatrices(modelMatrix, viewMatrix, projectionMatrix, mvpMatrix);

    if (attr->getNumIndices() > 0) {
        if (attr->getInstanceCount() == 0) {
            // Indices, no instancing
            /*glDrawRangeElements((GLuint)attr->getVertexMode(), 0, attr->getNumVertices()-1,
                    attr->getNumIndices(), attr->getIndexFormat(), NULL);*/
            glDrawElements((GLuint)attr->getVertexMode(), attr->getNumIndices(), attr->getIndexFormat(), NULL);
        } else {
            // Indices, instancing
            glDrawElementsInstanced((GLuint)attr->getVertexMode(), attr->getNumIndices(),
                                    attr->getIndexFormat(), NULL, attr->getInstanceCount());
        }
    } else {
        if (attr->getInstanceCount() == 0) {
            // No indices, no instancing
            glDrawArrays((GLuint)attr->getVertexMode(), 0, attr->getNumVertices());
        } else {
            // No indices, instancing
            glDrawArraysInstanced((GLuint)attr->getVertexMode(), 0, attr->getNumVertices(), attr->getInstanceCount());
        }
    }
}

void RendererGL::createMatrixBlock()
{
    matrixBlockBuffer = this->createGeometryBuffer(sizeof(MatrixBlock), &matrixBlock, UNIFORM_BUFFER, BUFFER_STREAM);

    // Binding point is unique for _all_ shaders
    ShaderManager->bindUniformBuffer(0, matrixBlockBuffer);
    //glBindBufferBase(GL_UNIFORM_BUFFER, 0, static_cast<GeometryBufferGL*>(matrixBlockBuffer.get())->getBuffer());
}

void RendererGL::updateMatrixBlock()
{
    if (matrixBlockNeedsUpdate) {
        mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
        matrixBlock.mMatrix = modelMatrix;
        matrixBlock.vMatrix = viewMatrix;
        matrixBlock.pMatrix = projectionMatrix;
        matrixBlock.mvpMatrix = mvpMatrix;
        matrixBlockBuffer->subData(0, sizeof(MatrixBlock), &matrixBlock);
        matrixBlockNeedsUpdate = false;
    }
}




void RendererGL::setPolygonMode(unsigned int polygonMode) // For debugging purposes
{
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
}

void RendererGL::enableWireframeMode(const Color &_wireframeColor)
{
    wireframeMode = true;
    wireframeColor = _wireframeColor;
    solidShader->setUniform("color", wireframeColor);
    setPolygonMode(GL_LINE);
}

void RendererGL::disableWireframeMode()
{
    wireframeMode = false;
    setPolygonMode(GL_FILL);
}


// Utility functions
void RendererGL::blitTexture(TexturePtr &tex, const AABB2 &renderRect, bool mirrored)
{
    if (tex->getNumSamples() > 0) {
        blitTexture(tex, renderRect, resolveMSAAShader, mirrored);
    } else {
        blitTexture(tex, renderRect, blitShader, mirrored);
    }
}

std::vector<VertexTextured> RendererGL::createTexturedQuad(const AABB2 &renderRect, bool mirrored /* = false */)
{
    glm::vec2 min = renderRect.getMinimum();
    glm::vec2 max = renderRect.getMaximum();
    if (!mirrored) {
        return std::vector<VertexTextured>{
                VertexTextured(glm::vec3(max.x,max.y,0), glm::vec2(1, 1)),
                VertexTextured(glm::vec3(min.x,min.y,0), glm::vec2(0, 0)),
                VertexTextured(glm::vec3(max.x,min.y,0), glm::vec2(1, 0)),
                VertexTextured(glm::vec3(min.x,min.y,0), glm::vec2(0, 0)),
                VertexTextured(glm::vec3(max.x,max.y,0), glm::vec2(1, 1)),
                VertexTextured(glm::vec3(min.x,max.y,0), glm::vec2(0, 1))};
    } else {
        return std::vector<VertexTextured>{
                VertexTextured(glm::vec3(max.x,max.y,0), glm::vec2(1, 0)),
                VertexTextured(glm::vec3(min.x,min.y,0), glm::vec2(0, 1)),
                VertexTextured(glm::vec3(max.x,min.y,0), glm::vec2(1, 1)),
                VertexTextured(glm::vec3(min.x,min.y,0), glm::vec2(0, 1)),
                VertexTextured(glm::vec3(max.x,max.y,0), glm::vec2(1, 0)),
                VertexTextured(glm::vec3(min.x,max.y,0), glm::vec2(0, 0))};
    }
}

void RendererGL::blitTexture(TexturePtr &tex, const AABB2 &renderRect, ShaderProgramPtr &shader, bool mirrored)
{
    // Set-up the vertex data of the rectangle
    std::vector<VertexTextured> fullscreenQuad(createTexturedQuad(renderRect, mirrored));

    // Feed the shader with the data and render the quad
    int stride = sizeof(VertexTextured);
    GeometryBufferPtr geomBuffer(new GeometryBufferGL(sizeof(VertexTextured)*fullscreenQuad.size(), &fullscreenQuad.front()));
    ShaderAttributesPtr shaderAttributes = ShaderManager->createShaderAttributes(shader);
    shaderAttributes->addGeometryBuffer(geomBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
    shaderAttributes->addGeometryBuffer(geomBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);
    shaderAttributes->getShaderProgram()->setUniform("texture", tex);
    if (tex->getNumSamples() > 0) {
        shaderAttributes->getShaderProgram()->setUniform("numSamples", tex->getNumSamples());
    }
    render(shaderAttributes);
}

void RendererGL::_setNormalizedViewProj() {
    // Set a new temporal MV and P matrix to render a fullscreen quad
    oldProjMatrix = projectionMatrix;
    oldViewMatrix = viewMatrix;
    oldModelMatrix = modelMatrix;
    glm::mat4 newProjMat(matrixOrthogonalProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
    setProjectionMatrix(newProjMat);
    setViewMatrix(matrixIdentity());
    setModelMatrix(matrixIdentity());
    oldFBO = boundFBO;
}

void RendererGL::_restoreViewProj() {
    // Reset the matrices
    setProjectionMatrix(oldProjMatrix);
    setViewMatrix(oldViewMatrix);
    setModelMatrix(oldModelMatrix);
    bindFBO(oldFBO);
    oldFBO = FramebufferObjectPtr();
}


//#define RESOLVE_BLIT_FBO
TexturePtr RendererGL::resolveMultisampledTexture(TexturePtr &tex) // Just returns tex if not multisampled
{
    if (tex->getNumSamples() <= 0) {
        return tex;
    }

#ifdef RESOLVE_BLIT_FBO
    TexturePtr resolvedTexture = TextureManager->createEmptyTexture(tex->getW(), tex->getH(),
            TextureSettings(tex->getMinificationFilter(), tex->getMagnificationFilter(), tex->getWrapS(), tex->getWrapT()));
    FramebufferObjectPtr msaaFBO = createFBO();
    FramebufferObjectPtr resolvedFBO = createFBO();
    msaaFBO->bindTexture(tex);
    resolvedFBO->bindTexture(resolvedTexture);
    int w = tex->getW(), h = tex->getH();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO->getID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedFBO->getID());
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#else
    // OR:
    TexturePtr resolvedTexture = TextureManager->createEmptyTexture(tex->getW(), tex->getH(),
            TextureSettings(tex->getMinificationFilter(), tex->getMagnificationFilter(), tex->getWrapS(), tex->getWrapT()));
    FramebufferObjectPtr fbo = createFBO();
    fbo->bindTexture(resolvedTexture);
    _setNormalizedViewProj();

    // Set-up the vertex data of the rectangle
    std::vector<VertexTextured> fullscreenQuad(createTexturedQuad(AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f))));

    // Feed the shader with the rendering data
    int stride = sizeof(VertexTextured);
    GeometryBufferPtr geomBuffer(new GeometryBufferGL(sizeof(VertexTextured)*fullscreenQuad.size(), &fullscreenQuad.front()));
    ShaderAttributesPtr shaderAttributes = ShaderManager->createShaderAttributes(resolveMSAAShader);
    shaderAttributes->addGeometryBuffer(geomBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
    shaderAttributes->addGeometryBuffer(geomBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);
    shaderAttributes->getShaderProgram()->setUniform("texture", tex);
    shaderAttributes->getShaderProgram()->setUniform("numSamples", tex->getNumSamples());

    // Now resolve the texture
    bindFBO(fbo);
    render(shaderAttributes);
    _restoreViewProj();

#endif

    return resolvedTexture;
}

void RendererGL::blurTexture(TexturePtr &tex)
{
    // Create a framebuffer and a temporal texture for blurring
    FramebufferObjectPtr blurFramebuffer = createFBO();
    TexturePtr tempBlurTexture = TextureManager->createEmptyTexture(tex->getW(), tex->getH(),
            TextureSettings(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER));

    // Set a new temporal MV and P matrix to render a fullscreen quad
    _setNormalizedViewProj();

    // Set-up the vertex data of the rectangle
    std::vector<VertexTextured> fullscreenQuad(createTexturedQuad(AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f))));

    // Feed the shader with the data and render the quad
    int stride = sizeof(VertexTextured);
    GeometryBufferPtr geomBuffer(new GeometryBufferGL(sizeof(VertexTextured)*fullscreenQuad.size(), &fullscreenQuad.front()));
    ShaderAttributesPtr shaderAttributes = ShaderManager->createShaderAttributes(blurShader);
    shaderAttributes->addGeometryBuffer(geomBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
    shaderAttributes->addGeometryBuffer(geomBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);
    shaderAttributes->getShaderProgram()->setUniform("texture", tex);
    shaderAttributes->getShaderProgram()->setUniform("texSize", glm::vec2(tex->getW(), tex->getH()));

    // Perform a horizontal and a vertical blur
    blurFramebuffer->bindTexture(tempBlurTexture);
    bindFBO(blurFramebuffer);
    shaderAttributes->getShaderProgram()->setUniform("horzBlur", true);
    render(shaderAttributes);

    blurFramebuffer->bindTexture(tex);
    bindFBO(blurFramebuffer, true);
    shaderAttributes->getShaderProgram()->setUniform("texture", tempBlurTexture);
    shaderAttributes->getShaderProgram()->setUniform("horzBlur", false);
    render(shaderAttributes);

    // Reset the matrices
    _restoreViewProj();
}

TexturePtr RendererGL::getScaledTexture(TexturePtr &tex, Point2 newSize)
{
    // Create a framebuffer and the storage for the scaled texture
    FramebufferObjectPtr framebuffer = createFBO();
    TexturePtr scaledTexture = TextureManager->createEmptyTexture(newSize.x, newSize.y, TextureSettings(
            tex->getMinificationFilter(), tex->getMagnificationFilter(), tex->getWrapS(), tex->getWrapT()));

    // Set a new temporal MV and P matrix to render a fullscreen quad
    _setNormalizedViewProj();

    // Create a scaled copy of the texture
    framebuffer->bindTexture(scaledTexture);
    bindFBO(framebuffer);
    blitTexture(tex, AABB2(glm::vec2(-1,-1), glm::vec2(1,1)));

    // Reset the matrices
    _restoreViewProj();

    return scaledTexture;
}

void RendererGL::blitTextureFXAAAntialiased(TexturePtr &tex)
{
    // Set a new temporal MV and P matrix to render a fullscreen quad
    _setNormalizedViewProj();

    // Set the attributes of the shader
    //fxaaShader->setUniform("m_Texture", tex);
    fxaaShader->setUniform("g_Resolution", glm::vec2(tex->getW(), tex->getH()));
    fxaaShader->setUniform("m_SubPixelShift", 1.0f / 4.0f);
    fxaaShader->setUniform("m_ReduceMul", 0.0f * 1.0f / 8.0f);
    //fxaaShader->setUniform("m_VxOffset", 0.0f);
    fxaaShader->setUniform("m_SpanMax", 16.0f);

    // Create a scaled copy of the texture
    blitTexture(tex, AABB2(glm::vec2(-1,-1), glm::vec2(1,1)), fxaaShader);

    // Reset the matrices
    _restoreViewProj();
}


// OpenGL-specific calls
void RendererGL::bindVAO(GLuint vao)
{
    if (vao != boundVAO) {
        boundVAO = vao;
        glBindVertexArray(vao);
    }
}

GLuint RendererGL::getVAO()
{
    return boundVAO;
}

void RendererGL::useShaderProgram(ShaderProgramGL *shader)
{
    unsigned int shaderID = shader ? shader->getShaderProgramID() : 0;
    if (shaderID != boundShader) {
        boundShader = shaderID;
        glUseProgram(shaderID);
    }
}

}
