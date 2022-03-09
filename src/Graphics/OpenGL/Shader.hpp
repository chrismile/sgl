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

#ifndef GRAPHICS_OPENGL_SHADER_HPP_
#define GRAPHICS_OPENGL_SHADER_HPP_

#include <list>
#include <map>

#include <GL/glew.h>

#include <Graphics/Shader/Shader.hpp>

namespace sgl {

class DLL_OBJECT ShaderGL : public Shader {
public:
    explicit ShaderGL(ShaderType _shaderType);
    ~ShaderGL() override;
    void setShaderText(const std::string &text) override;
    bool compile() override;

    /// Implementation dependent
    [[nodiscard]] inline GLuint getShaderID() const { return shaderID; }
    [[nodiscard]] inline GLuint getShaderType() const { return shaderType; }
    /// Returns e.g. "Fragment Shader" for logging purposes
    std::string getShaderDebugType();

private:
    GLuint shaderID;
    ShaderType shaderType;
};

typedef std::shared_ptr<Shader> ShaderPtr;

class DLL_OBJECT ShaderProgramGL : public ShaderProgram {
public:
    ShaderProgramGL();
    ~ShaderProgramGL() override;

    bool linkProgram() override;
    bool validateProgram() override;
    void attachShader(ShaderPtr shader) override;
    void detachShader(ShaderPtr shader) override;
    void bind() override;

    // Compute shader interface
    void dispatchCompute(int numGroupsX, int numGroupsY = 1, int numGroupsZ = 1) override;

    bool hasUniform(const char *name) override;
    int getUniformLoc(const char *name) override;
    bool setUniform(const char *name, int value) override;
    bool setUniform(const char *name, const glm::ivec2 &value) override;
    bool setUniform(const char *name, const glm::ivec3 &value) override;
    bool setUniform(const char *name, const glm::ivec4 &value) override;
    bool setUniform(const char *name, unsigned int value) override;
    bool setUniform(const char *name, const glm::uvec2 &value) override;
    bool setUniform(const char *name, const glm::uvec3 &value) override;
    bool setUniform(const char *name, const glm::uvec4 &value) override;
    bool setUniform(const char *name, bool value) override;
    bool setUniform(const char *name, const glm::bvec2 &value) override;
    bool setUniform(const char *name, const glm::bvec3 &value) override;
    bool setUniform(const char *name, const glm::bvec4 &value) override;
    bool setUniform(const char *name, float value) override;
    bool setUniform(const char *name, const glm::vec2 &value) override;
    bool setUniform(const char *name, const glm::vec3 &value) override;
    bool setUniform(const char *name, const glm::vec4 &value) override;
    bool setUniform(const char *name, const glm::mat3 &value) override;
    bool setUniform(const char *name, const glm::mat3x4 &value) override;
    bool setUniform(const char *name, const glm::mat4 &value) override;
    bool setUniform(const char *name, const TexturePtr &value, int textureUnit = 0) override;
    bool setUniform(const char *name, const Color &value) override;
    bool setUniformArray(const char *name, const int *value, size_t num) override;
    bool setUniformArray(const char *name, const unsigned int *value, size_t num) override;
    bool setUniformArray(const char *name, const bool *value, size_t num) override;
    bool setUniformArray(const char *name, const float *value, size_t num) override;
    bool setUniformArray(const char *name, const glm::vec2 *value, size_t num) override;
    bool setUniformArray(const char *name, const glm::vec3 *value, size_t num) override;
    bool setUniformArray(const char *name, const glm::vec4 *value, size_t num) override;

    bool setUniform(int location, int value) override;
    bool setUniform(int location, const glm::ivec2 &value) override;
    bool setUniform(int location, const glm::ivec3 &value) override;
    bool setUniform(int location, const glm::ivec4 &value) override;
    bool setUniform(int location, unsigned int value) override;
    bool setUniform(int location, const glm::uvec2 &value) override;
    bool setUniform(int location, const glm::uvec3 &value) override;
    bool setUniform(int location, const glm::uvec4 &value) override;
    bool setUniform(int location, bool value) override;
    bool setUniform(int location, const glm::bvec2 &value) override;
    bool setUniform(int location, const glm::bvec3 &value) override;
    bool setUniform(int location, const glm::bvec4 &value) override;
    bool setUniform(int location, float value) override;
    bool setUniform(int location, const glm::vec2 &value) override;
    bool setUniform(int location, const glm::vec3 &value) override;
    bool setUniform(int location, const glm::vec4 &value) override;
    bool setUniform(int location, const glm::mat3 &value) override;
    bool setUniform(int location, const glm::mat3x4 &value) override;
    bool setUniform(int location, const glm::mat4 &value) override;
    bool setUniform(int location, const TexturePtr &value, int textureUnit = 0) override;
    bool setUniform(int location, const Color &value) override;
    bool setUniformArray(int location, const int *value, size_t num) override;
    bool setUniformArray(int location, const unsigned int *value, size_t num) override;
    bool setUniformArray(int location, const bool *value, size_t num) override;
    bool setUniformArray(int location, const float *value, size_t num) override;
    bool setUniformArray(int location, const glm::vec2 *value, size_t num) override;
    bool setUniformArray(int location, const glm::vec3 *value, size_t num) override;
    bool setUniformArray(int location, const glm::vec4 *value, size_t num) override;


    // Image Load and Store

    /**
     * Binds a level of a texture to a uniform image unit in a shader.
     * For more details see: https://www.khronos.org/opengl/wiki/GLAPI/glBindImageTexture
     * @param unit: The binding in the shader to which the image should be attached.
     * @param texture: The texture to bind an image from.
     * @param format: The format used when performing formatted stores to the image.
     * @param access: GL_READ_ONLY, GL_WRITE_ONLY, or GL_READ_WRITE.
     * @param level: The level of a texture (usually of a mip-map) to be bound.
     * @param layered: When using a layered texture (e.g. GL_TEXTURE_2D_ARRAY) whether all layers should be bound.
     * @param layer: The layer to bind if "layered" is false.
     */
    void setUniformImageTexture(
            unsigned int unit, const TexturePtr& texture, unsigned int format = GL_RGBA8,
            unsigned int access = GL_READ_WRITE, unsigned int level = 0,
            bool layered = false, unsigned int layer = 0) override;


    // OpenGL 3 Uniform Buffers & OpenGL 4 Shader Storage Buffers

    /**
     * UBOs:
     * - Binding: A global slot for UBOs in the OpenGL context.
     * - Location (aka block index): The location of the referenced UBO within the shader.
     * Instead of location, one can also use the name of the UBO within the shader to reference it.
     * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
     */
    bool setUniformBuffer(int binding, int location, const GeometryBufferPtr &geometryBuffer) override;
    bool setUniformBuffer(int binding, const char *name, const GeometryBufferPtr &geometryBuffer) override;

    /**
     * Atomic counters (GL_ATOMIC_COUNTER_BUFFER)
     * https://www.khronos.org/opengl/wiki/Atomic_Counter
     * - Binding: A global slot for atomic counter buffers in the OpenGL context.
     * - Location: Not possible to specify. Oddly, only supported for uniform buffers and SSBOs in OpenGl specification.
     * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
     */
    bool setAtomicCounterBuffer(int binding, const GeometryBufferPtr &geometryBuffer) override;
    //bool setAtomicCounterBuffer(int binding, const char *name, const GeometryBufferPtr &geometryBuffer) override;

    /**
     * SSBOs:
     * - Binding: A global slot for SSBOs in the OpenGL context.
     * - Location (aka resource index): The location of the referenced SSBO within the shader.
     * Instead of location, one can also use the name of the SSBO within the shader to reference it.
     * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
     */
    bool setShaderStorageBuffer(int binding, int location, const GeometryBufferPtr &geometryBuffer) override;
    bool setShaderStorageBuffer(int binding, const char *name, const GeometryBufferPtr &geometryBuffer) override;

    [[nodiscard]] inline GLuint getShaderProgramID() const { return shaderProgramID; }

private:
    /// Prints an error message if the uniform doesn't exist
    int getUniformLoc_error(const char *name);
    std::map<std::string, int> uniforms;
    std::map<std::string, int> attributes;
    GLuint shaderProgramID;
};

}

/*! GRAPHICS_OPENGL_SHADER_HPP_ */
#endif
