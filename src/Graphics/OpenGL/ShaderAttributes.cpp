/*
 * ShaderAttributes.cpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include <Utils/File/Logfile.hpp>
#include <Utils/Timer.hpp>
#include <Utils/AppSettings.hpp>
#include <Math/Math.hpp>
#include <Graphics/Window.hpp>
#include "ShaderAttributes.hpp"
#include "RendererGL.hpp"
#include "Shader.hpp"
#include "GeometryBuffer.hpp"

namespace sgl {

int getComponentByteSize(VertexAttributeFormat format)
{
	if (format == ATTRIB_BYTE || format == ATTRIB_UNSIGNED_BYTE) {
		return sizeof(GLubyte);
	} else if (format == ATTRIB_SHORT || format == ATTRIB_UNSIGNED_SHORT) {
		return sizeof(GLushort);
	} else if (format == ATTRIB_INT || format == ATTRIB_UNSIGNED_INT) {
		return sizeof(GLuint);
	} else if (format == ATTRIB_HALF_FLOAT) {
		return sizeof(float)/2;
	} else if (format == ATTRIB_FLOAT || format == ATTRIB_FIXED) {
		return sizeof(float);
	} else if (format == ATTRIB_DOUBLE) {
		return sizeof(double);
	}
	return -1;
}


ShaderAttributesGL::ShaderAttributesGL(ShaderProgramPtr &_shader)
{
	shader = _shader;
	shaderGL = (ShaderProgramGL*)_shader.get();
	mMatrix = shader->getUniformLoc("mMatrix");
	vMatrix = shader->getUniformLoc("vMatrix");
	pMatrix = shader->getUniformLoc("pMatrix");
	mvpMatrix = shader->getUniformLoc("mvpMatrix");
	time = shader->getUniformLoc("time");
	resolution = shader->getUniformLoc("resolution");
}

void ShaderAttributesGL::setIndexGeometryBuffer(GeometryBufferPtr &geometryBuffer, VertexAttributeFormat format)
{
	indexBuffer = geometryBuffer;
	numIndices = geometryBuffer->getSize() / getComponentByteSize(format);
	indexFormat = format;
}

void ShaderAttributesGL::setModelViewProjectionMatrices(const glm::mat4 &m, const glm::mat4 &v, const glm::mat4 &p, const glm::mat4 &mvp)
{
	if (mMatrix >= 0) {
		shaderGL->setUniform(mMatrix, m);
	}
	if (vMatrix >= 0) {
		shaderGL->setUniform(vMatrix, v);
	}
	if (pMatrix >= 0) {
		shaderGL->setUniform(pMatrix, p);
	}
	if (mvpMatrix >= 0) {
		shaderGL->setUniform(mvpMatrix, mvp);
	}
	if (mvpMatrix >= 0) {
		shaderGL->setUniform(mvpMatrix, mvp);
	}
	if (time >= 0) {
		shaderGL->setUniform(time, Timer->getTimeInS());
	}
	if (resolution >= 0) {
		Window *window = AppSettings::get()->getMainWindow();
		shaderGL->setUniform(resolution, glm::vec2(window->getWidth(), window->getHeight()));
	}
}


// ---------------------------------- OpenGL 3 ----------------------------------

ShaderAttributesGL3::ShaderAttributesGL3(ShaderProgramPtr &_shader) : ShaderAttributesGL(_shader)
{
	RendererGL *rendererGL = static_cast<RendererGL*>(Renderer);
	glGenVertexArrays(1, &vaoID);
	rendererGL->bindVAO(vaoID);
	rendererGL->bindVAO(0);
}

ShaderAttributesGL3::~ShaderAttributesGL3()
{
	RendererGL *rendererGL = static_cast<RendererGL*>(Renderer);
	if (rendererGL->getVAO() == vaoID) {
		rendererGL->bindVAO(0);
	}
	glDeleteVertexArrays(1, &vaoID);
}

int vertexAttrFormatToIntSize(VertexAttributeFormat format) {
	switch (format) {
	case ATTRIB_BYTE:
	case ATTRIB_UNSIGNED_BYTE:
		return 1;
	case ATTRIB_SHORT:
	case ATTRIB_UNSIGNED_SHORT:
	case ATTRIB_HALF_FLOAT:
		return 2;
	case ATTRIB_INT:
	case ATTRIB_UNSIGNED_INT:
	case ATTRIB_FLOAT:
	case ATTRIB_FIXED:
		return 4;
	case ATTRIB_DOUBLE:
		return sizeof(double);
	default:
		Logfile::get()->writeError("ERROR: vertexAttrFormatToIntSize: Invalid format");
		return 0;
	}
}

ShaderAttributesPtr ShaderAttributesGL3::copy(ShaderProgramPtr &_shader, bool ignoreMissingAttrs /* = true */)
{
	ShaderAttributesGL3 *obj = new ShaderAttributesGL3(_shader);
	obj->vertexMode = this->vertexMode;
	obj->indexFormat = this->indexFormat;
	obj->numVertices = this->numVertices;
	obj->numIndices = this->numIndices;
	obj->instanceCount = this->instanceCount;
	if (this->indexBuffer) {
		obj->setIndexGeometryBuffer(this->indexBuffer, this->indexFormat);
	}
	for (AttributeData &attr : this->attributes) {
		int shaderLoc = glGetAttribLocation(obj->shaderGL->getShaderProgramID(), attr.attributeName.c_str());
		if (shaderLoc >= 0) {
			obj->addGeometryBuffer(attr.geometryBuffer, attr.attributeName.c_str(),
					(VertexAttributeFormat)attr.attributeType, attr.components, attr.offset, attr.stride);
		}
	}
	obj->attributes = this->attributes;
	obj->indexBuffer = this->indexBuffer;
	return ShaderAttributesPtr(obj);
}

void ShaderAttributesGL3::addGeometryBuffer(GeometryBufferPtr &geometryBuffer,
		const char *attributeName, VertexAttributeFormat format, int components,
		int offset /* = 0 */, int stride /* = 0 */, int instancing /* = 0 */)
{
	RendererGL *rendererGL = static_cast<RendererGL*>(Renderer);
	attributes.push_back(AttributeData(geometryBuffer, attributeName, (GLuint)format, components,
			glGetAttribLocation(shaderGL->getShaderProgramID(), attributeName), offset, stride));

	rendererGL->bindVAO(vaoID);
	int shaderLoc = glGetAttribLocation(shaderGL->getShaderProgramID(), attributeName);
	if (shaderLoc < 0) {
		Logfile::get()->writeError(std::string() + "ERROR: ShaderAttributesGL3::addGeometryBuffer: "
				+ "shaderLoc < 0 (attributeName: \"" + attributeName + "\")");
	}
	GLuint dataType = (GLuint)format;
	glEnableVertexAttribArray(shaderLoc);
	geometryBuffer->bind();
	// OpenGL only allows a maximum of four components per attribute.
	// E.g. 4x4 matrices are internally split into four columns.
	int numColumns = ceilDiv(components, 4);
	int columnSize = vertexAttrFormatToIntSize(format); // In bytes
	for (int column = 0; column < numColumns; ++column) {
		glVertexAttribPointer(shaderLoc+column, components, dataType, GL_FALSE,
				stride, (void*)(intptr_t)(offset+columnSize*column));
		if (instancing > 0) {
			glVertexAttribDivisor(shaderLoc+column, instancing);
		}
	}
	rendererGL->bindVAO(0);

	// Compute the number of elements/vertices
	int componentByteSize = getComponentByteSize(format);
	int numElements = geometryBuffer->getSize();
	if (stride == 0) {
		numElements /= componentByteSize * components;
	} else {
		numElements /= stride;
	}

	// Update information about the vertices
	if (numVertices > 0 && (int)numVertices != numElements) {
		Logfile::get()->writeError("ERROR: ShaderAttributesGL3::addGeometryBuffer: "
			"Inconsistent number of vertex attribute elements!");
	}
	numVertices = numElements;
}

void ShaderAttributesGL3::bind()
{
	shader->bind();
	RendererGL *rendererGL = static_cast<RendererGL*>(Renderer);
	rendererGL->bindVAO(vaoID);
	if (indexBuffer) {
		indexBuffer->bind();
	}
}



// ---------------------------------- OpenGL 2 ----------------------------------

ShaderAttributesGL2::ShaderAttributesGL2(ShaderProgramPtr &_shader) : ShaderAttributesGL(_shader)
{}

ShaderAttributesGL2::~ShaderAttributesGL2()
{}

ShaderAttributesPtr ShaderAttributesGL2::copy(ShaderProgramPtr &_shader, bool ignoreMissingAttrs /* = true */)
{
	ShaderAttributesGL3 *obj = new ShaderAttributesGL3(_shader);
	obj->vertexMode = this->vertexMode;
	obj->indexFormat = this->indexFormat;
	obj->numVertices = this->numVertices;
	obj->numIndices = this->numIndices;
	obj->instanceCount = this->instanceCount;
	obj->setIndexGeometryBuffer(this->indexBuffer, this->indexFormat);
	for (AttributeData &attr : this->attributes) {
		int shaderLoc = glGetAttribLocation(obj->shaderGL->getShaderProgramID(), attr.attributeName.c_str());
		if (shaderLoc >= 0) {
			obj->addGeometryBuffer(attr.geometryBuffer, attr.attributeName.c_str(),
					(VertexAttributeFormat)attr.attributeType, attr.components, attr.offset, attr.stride);
		}
	}
	obj->attributes = this->attributes;
	obj->indexBuffer = this->indexBuffer;
	return ShaderAttributesPtr(obj);
}

void ShaderAttributesGL2::addGeometryBuffer(GeometryBufferPtr &geometryBuffer,
		const char *attributeName, VertexAttributeFormat format, int components,
		int offset /* = 0 */, int stride /* = 0 */, int instancing /* = 0 */)
{
	if (instancing > 0) {
		Logfile::get()->writeError( "ERROR: ShaderAttributesGL2::addGeometryBuffer: OpenGL 2 does not support instancing.");
		return;
	}

	int shaderLoc = glGetAttribLocation(shaderGL->getShaderProgramID(), attributeName);
	if (shaderLoc < 0) {
		Logfile::get()->writeError(std::string() + "ERROR: ShaderAttributesGL2::addGeometryBuffer: "
				+ "shaderLoc < 0 (attributeName: \"" + attributeName + "\")");
		return;
	}
	attributes.push_back(AttributeData(geometryBuffer, attributeName,
			(GLuint)format, components, shaderLoc, offset, stride));

	// Compute the number of elements/vertices
	int componentByteSize = getComponentByteSize(format);
	int numElements = geometryBuffer->getSize();
	if (stride == 0) {
		numElements /= componentByteSize * components;
	} else {
		numElements /= stride;
	}

	// Update information about the vertices
	if (numVertices > 0 && (int)numVertices != numElements) {
		Logfile::get()->writeError("ERROR: ShaderAttributesGL3::addGeometryBuffer: "
			"Inconsistent number of vertex attribute elements!");
	}
	numVertices = numElements;
}

void ShaderAttributesGL2::bind()
{
	shader->bind();

	for (AttributeData &attributeData : attributes) {
		attributeData.geometryBuffer->bind();
		glEnableVertexAttribArray(attributeData.shaderLoc);
		glVertexAttribPointer(attributeData.shaderLoc, attributeData.components, attributeData.attributeType,
				GL_FALSE, attributeData.stride, (void*)(intptr_t)attributeData.offset);
	}

	if (indexBuffer) {
		indexBuffer->bind();
	}
}

}