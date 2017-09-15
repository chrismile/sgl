/*!
 * ShaderAttributes.hpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_
#define GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_

#include <Defs.hpp>
#include <Graphics/Buffers/GeometryBuffer.hpp>
#include <glm/glm.hpp>

namespace sgl {

class ShaderProgram;

enum DLL_OBJECT VertexAttributeFormat {
	ATTRIB_BYTE = 0x1400, ATTRIB_UNSIGNED_BYTE = 0x1401,
	ATTRIB_SHORT = 0x1402, ATTRIB_UNSIGNED_SHORT = 0x1403,
	ATTRIB_INT = 0x1404, ATTRIB_UNSIGNED_INT = 0x1405,
	ATTRIB_HALF_FLOAT = 0x140B, ATTRIB_FLOAT = 0x1406,
	ATTRIB_DOUBLE = 0x140A, ATTRIB_FIXED = 0x140C
};

enum DLL_OBJECT VertexMode : short {
	VERTEX_MODE_POINTS = 0x0000, VERTEX_MODE_LINES = 0x0001,
	VERTEX_MODE_LINE_LOOP = 0x0002, VERTEX_MODE_LINE_STRIP = 0x0003,
	VERTEX_MODE_TRIANGLES = 0x0004, VERTEX_MODE_TRIANGLE_STRIP = 0x0005,
	VERTEX_MODE_TRIANGLE_FAN = 0x0006
};


/*! Shader attributes are the heart of the graphics engine.
 * They manage the interaction between shaders and geometry.
 * First, you create a new instance with "ShaderManager->createShaderAttributes(shaderProgram)".
 * Then you can add geometry buffers containing all vertices, indices, etc.
 * Finally, you can use "Renderer->render(shaderAttributes)" to render your geometry in every frame. */

class ShaderAttributes;
typedef boost::shared_ptr<ShaderAttributes> ShaderAttributesPtr;
class ShaderProgram;
typedef boost::shared_ptr<ShaderProgram> ShaderProgramPtr;

class DLL_OBJECT ShaderAttributes
{
	friend class ShaderProgram;
public:
	ShaderAttributes() : vertexMode(VERTEX_MODE_TRIANGLES), indexFormat(ATTRIB_UNSIGNED_SHORT), numVertices(0), numIndices(0) {}
	virtual ~ShaderAttributes() {}
	virtual ShaderAttributesPtr copy(ShaderProgramPtr &_shader, bool ignoreMissingAttrs = true)=0;

	/*! Adds a geometry buffer to the shader attributes.
	 * "offset" and "stride" optionally specify the location of the attributes in the buffer.
	 * [format=ATTRIB_FLOAT,components=3] e.g. means glm::vec3 data */
	virtual void addGeometryBuffer(GeometryBufferPtr &geometryBuffer, const char *attributeName,
			VertexAttributeFormat format, int components, int offset = 0, int stride = 0)=0;
	virtual void setIndexGeometryBuffer(GeometryBufferPtr &geometryBuffer, VertexAttributeFormat format)=0;
	virtual void bind()=0;
	virtual ShaderProgram *getShaderProgram()=0;

	// Query information on the shader attributes
	inline void setVertexMode(VertexMode _vertexMode) { vertexMode = _vertexMode; }
	inline VertexMode getVertexMode() const { return vertexMode; }
	inline VertexAttributeFormat getIndexFormat() const { return indexFormat; }
	inline size_t getNumVertices() const { return numVertices; }
	inline size_t getNumIndices() const { return numIndices; }

	// Set shared data in the renderer - done by Renderer
	virtual void setModelViewProjectionMatrices(const glm::mat4 &m, const glm::mat4 &v, const glm::mat4 &p, const glm::mat4 &mvp)=0;

protected:
	VertexMode vertexMode;
	VertexAttributeFormat indexFormat;
	size_t numVertices, numIndices;
};

}

/*! GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_ */
#endif
