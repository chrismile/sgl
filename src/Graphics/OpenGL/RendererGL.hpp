/*!
 * RendererGL.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
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
struct MatrixBlock
{
    glm::mat4 mMatrix; // Model matrix
    glm::mat4 vMatrix; // View matrix
    glm::mat4 pMatrix; // Projection matrix
    glm::mat4 mvpMatrix; // Model-view-projection matrix
};

class RendererGL : public RendererInterface
{
public:
	RendererGL();

	//! Outputs e.g. "glGetError"
	virtual void errorCheck();
	// The functions below only work in an OpenGL debug context
	/// Sets a callback function that is called (synchronously) when an error in the OpenGL context occurs
	virtual void setErrorCallback(std::function<void()> callback);
	virtual void callApplicationErrorCallback();
	/// Set how much error reporting the program wants
	virtual void setDebugVerbosity(DebugVerbosity verbosity);

	//! Creation functions
	virtual FramebufferObjectPtr createFBO();
	virtual RenderbufferObjectPtr createRBO(int _width, int _height, RenderbufferType rboType, int _samples = 0);
	virtual GeometryBufferPtr createGeometryBuffer(size_t size, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC);
	virtual GeometryBufferPtr createGeometryBuffer(size_t size, void *data, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC);

	//! Functions for managing viewports/render targets
	virtual void bindFBO(FramebufferObjectPtr _fbo, bool force = false);
	virtual void unbindFBO(bool force = false);
	virtual FramebufferObjectPtr getFBO();
	virtual void clearFramebuffer(unsigned int buffers = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			const Color& col = Color(0, 0, 0), float depth = 1.0f, unsigned short stencil = 0);
	virtual void setCamera(CameraPtr _viewport, bool force = false);
	virtual CameraPtr getCamera();

	//! State changes
	virtual void bindTexture(TexturePtr &tex, unsigned int textureUnit = 0);
	virtual void setBlendMode(BlendMode mode);
	virtual void setModelMatrix(const glm::mat4 &matrix);
	virtual void setViewMatrix(const glm::mat4 &matrix);
	virtual void setProjectionMatrix(const glm::mat4 &matrix);
	virtual void setLineWidth(float width);
	virtual void setPointSize(float size);

	//! Stencil buffer
	virtual void enableStencilTest();
	virtual void disableStencilTest();
	virtual void setStencilMask(unsigned int mask);
	virtual void clearStencilBuffer();
	virtual void setStencilFunc(unsigned int func, int ref, unsigned int mask);
	virtual void setStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass);

	//! Rendering
	virtual void render(ShaderAttributesPtr &shaderAttributes);
	//! Rendering with overwritten shader (e.g. for multi-pass rendering without calling copy()).
	virtual void render(ShaderAttributesPtr &shaderAttributes, ShaderProgramPtr &passShader);

	//! For debugging purposes
	virtual void setPolygonMode(unsigned int polygonMode);
	//! For debugging purposes
	virtual void enableWireframeMode(const Color &_wireframeColor = Color(255, 255, 255));
	//! For debugging purposes
	virtual void disableWireframeMode();

	// Utility functions
	virtual void blitTexture(TexturePtr &tex, const AABB2 &renderRect, bool mirrored = false);
	virtual void blitTexture(TexturePtr &tex, const AABB2 &renderRect, ShaderProgramPtr &shader, bool mirrored = false);
	//! Just returns tex if not multisampled
	virtual TexturePtr resolveMultisampledTexture(TexturePtr &tex);
	//! Texture needs GL_LINEAR filter for best results!
	virtual void blurTexture(TexturePtr &tex);
	virtual TexturePtr getScaledTexture(TexturePtr &tex, Point2 newSize);
	virtual void blitTextureFXAAAntialiased(TexturePtr &tex);

	// OpenGL-specific calls
	void bindVAO(GLuint vao);
	GLuint getVAO();
	void useShaderProgram(ShaderProgramGL *shader);

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
	GLuint currentTextureUnit;
	GLuint boundFBOID, boundVAO, boundShader;
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
