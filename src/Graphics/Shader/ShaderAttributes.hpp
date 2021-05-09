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

#ifndef GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_
#define GRAPHICS_SHADER_SHADERATTRIBUTES_HPP_

#include <memory>

#include <glm/glm.hpp>

#include <Defs.hpp>
#include <Graphics/Buffers/GeometryBuffer.hpp>

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

/**
 * Vertex attributes passed to the GPU may undergo conversion by the driver.
 * For more details see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
 */
enum VertexAttributeConversion {
    // Attribute values converted to floating-point type (single-precision).
    ATTRIB_CONVERSION_FLOAT,
    /*
     * Attribute values converted to floating point type and normalized to
     * range [-1,1] (for signed values) or [0,1] (for unsigned values).
     */
    ATTRIB_CONVERSION_FLOAT_NORMALIZED,
    // Attribute values converted to integer type.
    ATTRIB_CONVERSION_INT,
    // Attribute values converted to floating-point type (double-precision).
    ATTRIB_CONVERSION_DOUBLE
};


/*! Shader attributes are the heart of the graphics engine.
 * They manage the interaction between shaders and geometry.
 * First, you create a new instance with "ShaderManager->createShaderAttributes(shaderProgram)".
 * Then you can add geometry buffers containing all vertices, indices, etc.
 * Finally, you can use "Renderer->render(shaderAttributes)" to render your geometry in every frame. */

class ShaderAttributes;
typedef std::shared_ptr<ShaderAttributes> ShaderAttributesPtr;
class ShaderProgram;
typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;

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
     * \param instancing is the number by which the instance count is increased with every instance.
     *         A value of 0 means no instancing. Use Renderer->renderInstanced if this value is > 0.
     * \param attrConversion Should be changed for e.g. uint32_t colors that are accessed as denormalized vec4's.
     * \return True if geometry buffer was found, false otherwise.
     *  NOTE: Instancing is only supported if OpenGL >= 3.3 or OpenGL ES >= 3.0! */
    virtual bool addGeometryBuffer(GeometryBufferPtr &geometryBuffer, const char *attributeName,
                                   VertexAttributeFormat format, int components,
                                   int offset = 0, int stride = 0, int instancing = 0,
                                   VertexAttributeConversion attrConversion = ATTRIB_CONVERSION_FLOAT)=0;
    /// Same as "addGeometryBuffer", but no error message if attribute not existent in shader.
    virtual bool addGeometryBufferOptional(GeometryBufferPtr &geometryBuffer, const char *attributeName,
                                           VertexAttributeFormat format, int components,
                                           int offset = 0, int stride = 0, int instancing = 0,
                                           VertexAttributeConversion attrConversion = ATTRIB_CONVERSION_FLOAT)=0;
    /// Same as function above, but specifies layout binding position in vertex shader instead of attribute name.
    virtual void addGeometryBuffer(GeometryBufferPtr &geometryBuffer, int attributeLocation,
                                   VertexAttributeFormat format, int components,
                                   int offset = 0, int stride = 0, int instancing = 0,
                                   VertexAttributeConversion attrConversion = ATTRIB_CONVERSION_FLOAT)=0;
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
