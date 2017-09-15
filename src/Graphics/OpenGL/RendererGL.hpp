/*
 * RendererGL.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_OPENGL_RENDERERGL_HPP_
#define GRAPHICS_OPENGL_RENDERERGL_HPP_

#include <vector>
#include <glm/glm.hpp>
#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Shader/Shader.hpp>

namespace sgl {

class ShaderProgramGL;

class RendererGL : public RendererInterface
{
public:
	RendererGL();
	virtual void errorCheck(); // Outputs e.g. "glGetError"

	// Creation functions
	virtual FramebufferObjectPtr createFBO();
	virtual RenderbufferObjectPtr createRBO(int _width, int _height, RenderbufferType rboType, int _samples = 0);
	virtual GeometryBufferPtr createGeometryBuffer(size_t size, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC);
	virtual GeometryBufferPtr createGeometryBuffer(size_t size, void *data, BufferType type = VERTEX_BUFFER, BufferUse bufferUse = BUFFER_STATIC);

	// Functions for managing viewports/render targets
	virtual void bindFBO(FramebufferObjectPtr _fbo, bool force = false);
	virtual void unbindFBO(bool force = false);
	virtual FramebufferObjectPtr getFBO();
	virtual void clearFramebuffer(unsigned int buffers = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, const Color& col = Color(0, 0, 0), float depth = 0.0f, unsigned short stencil = 0);
	virtual void setCamera(CameraPtr _viewport, bool force = false);
	virtual CameraPtr getCamera();

	// State changes
	virtual void bindTexture(TexturePtr &tex, unsigned int textureUnit = 0);
	virtual void setBlendMode(BlendMode mode);
	virtual void setModelMatrix(const glm::mat4 &matrix);
	virtual void setViewMatrix(const glm::mat4 &matrix);
	virtual void setProjectionMatrix(const glm::mat4 &matrix);
	virtual void setLineWidth(float width);
	virtual void setPointSize(float size);

	// Stencil buffer
	virtual void enableStencilTest();
	virtual void disableStencilTest();
	virtual void setStencilMask(unsigned int mask);
	virtual void clearStencilBuffer();
	virtual void setStencilFunc(unsigned int func, int ref, unsigned int mask);
	virtual void setStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass);

	// Rendering
	virtual void render(ShaderAttributesPtr &shaderAttributes);
	virtual void setPolygonMode(unsigned int polygonMode); // For debugging purposes
	virtual void enableWireframeMode(const Color &_wireframeColor = Color(255, 255, 255)); // For debugging purposes
	virtual void disableWireframeMode(); // For debugging purposes

	// Utility functions
	virtual void blitTexture(TexturePtr &tex, const AABB2 &renderRect);
	virtual void blitTexture(TexturePtr &tex, const AABB2 &renderRect, ShaderProgramPtr &shader);
	virtual TexturePtr resolveMultisampledTexture(TexturePtr &tex); // Just returns tex if not multisampled
	virtual void blurTexture(TexturePtr &tex); // Texture needs GL_LINEAR filter for best results!
	virtual TexturePtr getScaledTexture(TexturePtr &tex, Point2 newSize);
	virtual void blitTextureFXAAAntialiased(TexturePtr &tex);

	// OpenGL-specific calls
	void bindVAO(GLuint vao);
	GLuint getVAO();
	void useShaderProgram(ShaderProgramGL *shader);

public:
	glm::mat4 modelMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix, mvpMatrix;
	float lineWidth, pointSize;
	bool wireframeMode;
	bool debugOutputExtEnabled; // https://www.khronos.org/opengl/wiki/Debug_Output
	Color wireframeColor;
	BlendMode blendMode;
	FramebufferObjectPtr boundFBO;
	std::vector<GLuint> boundTextureID;
	GLuint currentTextureUnit;
	GLuint boundFBOID, boundVAO, boundShader;
	CameraPtr camera;
	ShaderProgramPtr fxaaShader, blurShader, blitShader, resolveMSAAShader, solidShader, whiteShader;
};

}

#endif /* GRAPHICS_OPENGL_RENDERERGL_HPP_ */
