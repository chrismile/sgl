/*!
 * ShaderAttributes.hpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_
#define GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_

#include <Defs.hpp>
#include <Graphics/Buffers/GeometryBuffer.hpp>
#include <glm/glm.hpp>

namespace sgl {

class ShaderProgram;

enum VertexAttributeFormat {
	ATTRIB_BYTE = 0x1400, ATTRIB_UNSIGNED_BYTE = 0x1401,
	ATTRIB_SHORT = 0x1402, ATTRIB_UNSIGNED_SHORT = 0x1403,
	ATTRIB_INT = 0x1404, ATTRIB_UNSIGNED_INT = 0x1405,
	ATTRIB_HALF_FLOAT = 0x140B, ATTRIB_FLOAT = 0x1406,
	ATTRIB_DOUBLE = 0x140A, ATTRIB_FIXED = 0x140C
};

enum VertexMode : short {
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
	ShaderAttributes() : vertexMode(VERTEX_MODE_TRIANGLES), indexFormat(ATTRIB_UNSIGNED_SHORT),
		numVertices(0), numIndices(0), instanceCount(0) {}
	virtual ~ShaderAttributes() {}

	//! Creates copy of attributes with other shader
	virtual ShaderAttributesPtr copy(ShaderProgramPtr &_shader, bool ignoreMissingAttrs = true)=0;

	/*! Adds a geometry buffer to the shader attributes.
	 * Example: "format=ATTRIB_FLOAT,components=3" e.g. means vec3 data.
	 * \param geometryBuffer is the geometry buffer containing the data.
	 * \param attributeName is the name of the attribute in the shader the data shall be bound to
	 * \param format is the data type of a scalar value in the buffer (e.g. int, float, etc.)
	 * \param components is the size of our vector. If this value is > 4, it is handled as a matrix.
	 * \param offset is the starting position in byte of the attribute inside one buffer element
	 * \param stride is the offset in byte between two buffer elements (0 means no interleaving data)
	 * \param instancing is the number by which the instance count is increased in every frame.
	 *         A value of 0 means no instancing. Use Renderer->renderInstanced if this value is > 0.
	 *  NOTE: Instancing is only supported if OpenGL >= 3.3 or OpenGL ES >= 3.0! */
	virtual void addGeometryBuffer(GeometryBufferPtr &geometryBuffer, const char *attributeName,
			VertexAttributeFormat format, int components,
			int offset = 0, int stride = 0, int instancing = 0)=0;
	virtual void setIndexGeometryBuffer(GeometryBufferPtr &geometryBuffer, VertexAttributeFormat format)=0;
	virtual void bind()=0;
	/// Pass a shader attribute in case you want to do multi-pass rendering without a call to copy().
	/// NOTE: Expects the passed shader to have the same binding point like the shader attached (is not checked!).
	virtual void bind(ShaderProgramPtr passShader)=0;
	virtual ShaderProgram *getShaderProgram()=0;

	// Query information on the shader attributes
	inline void setVertexMode(VertexMode _vertexMode) { vertexMode = _vertexMode; }
	inline VertexMode getVertexMode() const { return vertexMode; }
	inline VertexAttributeFormat getIndexFormat() const { return indexFormat; }
	inline size_t getNumVertices() const { return numVertices; }
	inline size_t getNumIndices() const { return numIndices; }
	//! Gets and sets the number of instances to be rendered (standard: No instancing/0)
	inline size_t getInstanceCount() const { return instanceCount; }
	inline void setInstanceCount(int count) { instanceCount = count; }

protected:
	VertexMode vertexMode;
	VertexAttributeFormat indexFormat;
	size_t numVertices, numIndices;
	int instanceCount;
};

}

/*! GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_ */
#endif
