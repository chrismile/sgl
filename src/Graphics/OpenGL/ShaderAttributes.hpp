/*!
 * ShaderAttributes.hpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_SHADERATTRIBUTES_HPP_
#define GRAPHICS_OPENGL_SHADERATTRIBUTES_HPP_

#include <list>
#include <vector>
#include <Graphics/Shader/ShaderAttributes.hpp>
#include "GeometryBuffer.hpp"
#include "Shader.hpp"

namespace sgl {

class ShaderProgramGL;
class ShaderProgram;

struct AttributeData {
	AttributeData(GeometryBufferPtr _geometryBuffer, const std::string &_attributeName, GLuint _attributeType,
			GLuint _components, int _shaderLoc, int _offset, int _stride)
		: geometryBuffer(_geometryBuffer), attributeName(_attributeName), attributeType(_attributeType),
		  components(_components), shaderLoc(_shaderLoc), offset(_offset), stride(_stride) {}
	GeometryBufferPtr geometryBuffer;
	std::string attributeName;
	unsigned int attributeType;
	unsigned int components;
	int shaderLoc;
	int offset;
	int stride;
};

//! Abstract class
class ShaderAttributesGL : public ShaderAttributes
{
friend class ShaderAttributesGL2;
friend class ShaderAttributesGL3;
public:
	ShaderAttributesGL(ShaderProgramPtr &_shader);
	~ShaderAttributesGL() {}

	void setIndexGeometryBuffer(GeometryBufferPtr &geometryBuffer, VertexAttributeFormat format);
	void setModelViewProjectionMatrices(const glm::mat4 &m, const glm::mat4 &v, const glm::mat4 &p, const glm::mat4 &mvp);
	ShaderProgram *getShaderProgram() { return shader.get(); }

protected:
	std::vector<AttributeData> attributes;
	GeometryBufferPtr indexBuffer;
	ShaderProgramPtr shader;
	ShaderProgramGL *shaderGL;
	//! Location of the transformation matrices
	int mMatrix, vMatrix, pMatrix, mvpMatrix, time, resolution;
};

/*! The OpenGL 3 implementation of shader attributes.
 * Vertex Array Objects are used to minimize the number of API calls. */
class ShaderAttributesGL3 : public ShaderAttributesGL
{
public:
	ShaderAttributesGL3(ShaderProgramPtr &_shader);
	~ShaderAttributesGL3();
	ShaderAttributesPtr copy(ShaderProgramPtr &_shader, bool ignoreMissingAttrs = true);

	/*! Adds a geometry buffer to the shader attributes.
	 * Example: "format=ATTRIB_FLOAT,components=3" e.g. means vec3 data.
	 * \param geometryBuffer is the geometry buffer containing the data.
	 * \param attributeName is the name of the attribute in the shader the data shall be bound to
	 * \param format is the data type of a scalar value in the buffer (e.g. int, float, etc.)
	 * \param components is the size of our vector. If this value is > 4, it is handled as a matrix.
	 * \param offset is the starting position in byte of the attribute inside one buffer element
	 * \param stride is the offset in byte between two buffer elements (0 means no interleaving data)
	 * \param instancing is the number by which the instance count is increased with every instance.
	 *         A value of 0 means no instancing. Use Renderer->renderInstanced if this value is > 0.
	 *  NOTE: Instancing is only supported if OpenGL >= 3.3 or OpenGL ES >= 3.0! */
	void addGeometryBuffer(GeometryBufferPtr &geometryBuffer, const char *attributeName,
			VertexAttributeFormat format, int components,
			int offset = 0, int stride = 0, int instancing = 0);
	void bind();

protected:
	unsigned int vaoID;
};

//! The old OpenGL 2 implementation binds the attributes manually.
class ShaderAttributesGL2 : public ShaderAttributesGL
{
public:
	ShaderAttributesGL2(ShaderProgramPtr &_shader);
	~ShaderAttributesGL2();
	ShaderAttributesPtr copy(ShaderProgramPtr &_shader, bool ignoreMissingAttrs = true);

	void addGeometryBuffer(GeometryBufferPtr &geometryBuffer, const char *attributeName,
			VertexAttributeFormat format, int components,
			int offset = 0, int stride = 0, int instancing = 0);
	void bind();
};

}

/*! GRAPHICS_OPENGL_SHADERATTRIBUTES_HPP_ */
#endif
