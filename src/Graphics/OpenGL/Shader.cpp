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

#include <GL/glew.h>
#include "Shader.hpp"
#include "SystemGL.hpp"
#include "RendererGL.hpp"
#include "Texture.hpp"
#include "ShaderAttributes.hpp"
#include "ShaderManager.hpp"
#include <Utils/File/Logfile.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sgl {

ShaderGL::ShaderGL(ShaderType _shaderType) : shaderType(_shaderType) {
    if (shaderType == VERTEX_SHADER) {
        shaderID = glCreateShader(GL_VERTEX_SHADER);
    } else if (shaderType == FRAGMENT_SHADER) {
        shaderID = glCreateShader(GL_FRAGMENT_SHADER);
    } else if (shaderType == GEOMETRY_SHADER) {
        shaderID = glCreateShader(GL_GEOMETRY_SHADER);
    } else if (shaderType == TESSELATION_EVALUATION_SHADER) {
        shaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
    } else if (shaderType == TESSELATION_CONTROL_SHADER) {
        shaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
    } else if (shaderType == COMPUTE_SHADER) {
        shaderID = glCreateShader(GL_COMPUTE_SHADER);
    }
}

ShaderGL::~ShaderGL() {
    glDeleteShader(shaderID);
}

void ShaderGL::setShaderText(const std::string &text) {
    // Upload the shader text to the graphics card
    const GLchar *stringPtrs[1];
    stringPtrs[0] = text.c_str();
    glShaderSource(shaderID, 1, stringPtrs, nullptr);
}

bool ShaderGL::compile() {
    glCompileShader(shaderID);
    GLint succes;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &succes);
    if (!succes) {
        GLint infoLogLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* infoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shaderID, infoLogLength, nullptr, infoLog);
        Logfile::get()->writeError(std::string() + "ERROR: ShaderGL::compile: Cannot compile shader! fileID: \"" + fileID + "\"");
        Logfile::get()->writeError(std::string() + "OpenGL Error: " + infoLog);
        delete[] infoLog;
        return false;
    }
    return true;
}

// Returns e.g. "Fragment Shader" for logging purposes
std::string ShaderGL::getShaderDebugType() {
    std::string type;
    if (shaderType == VERTEX_SHADER) {
        type = "Vertex Shader";
    } else if (shaderType == FRAGMENT_SHADER) {
        type = "Fragment Shader";
    } else if (shaderType == GEOMETRY_SHADER) {
        type = "Geometry Shader";
    } else if (shaderType == TESSELATION_EVALUATION_SHADER) {
        type = "Tesselation Evaluation Shader";
    } else if (shaderType == TESSELATION_CONTROL_SHADER) {
        type = "Tesselation Control Shader";
    } else if (shaderType == COMPUTE_SHADER) {
        type = "Compute Shader";
    }
    return type;
}




// ------------------------- Shader Program -------------------------

ShaderProgramGL::ShaderProgramGL() {
    shaderProgramID = glCreateProgram();
}

ShaderProgramGL::~ShaderProgramGL() {
    glDeleteProgram(shaderProgramID);
}


bool ShaderProgramGL::linkProgram() {
    // 1. Link the shader program
    glLinkProgram(shaderProgramID);
    GLint success = 0;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
    if (!success) {
        GLint infoLogLength;
        glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* infoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(shaderProgramID, infoLogLength, nullptr, infoLog);
        Logfile::get()->writeError("Error: Cannot link shader program!");
        Logfile::get()->writeError(std::string() + "OpenGL Error: " + infoLog);
        Logfile::get()->writeError("fileIDs of the linked shaders:");
        for (auto it = shaders.begin(); it != shaders.end(); ++it) {
            ShaderGL *shaderGL = (ShaderGL*)it->get();
            std::string type = shaderGL->getShaderDebugType();
            Logfile::get()->writeError(std::string() + "\"" + shaderGL->getFileID() + "\" (Type: " + type + ")");
        }
        delete[] infoLog;
        return false;
    }

    return true;
}

bool ShaderProgramGL::validateProgram() {
    // 2. Validation
    glValidateProgram(shaderProgramID);
    GLint success = 0;
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &success);
    if (!success) {
        GLint infoLogLength;
        glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar *infoLog = new GLchar[infoLogLength+1];
        glGetProgramInfoLog(shaderProgramID, infoLogLength, NULL, infoLog);
        Logfile::get()->writeError("Error in shader program validation!");
        Logfile::get()->writeError(std::string() + "OpenGL Error: " + infoLog);
        Logfile::get()->writeError("fileIDs of the linked shaders:");
        for (std::vector<ShaderPtr>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
            ShaderGL *shaderGL = (ShaderGL*)it->get();
            std::string type = shaderGL->getShaderDebugType();
            Logfile::get()->writeError(std::string() + "\"" + shaderGL->getFileID() + "\" (Type: " + type + ")");
        }
        delete[] infoLog;
        return false;
    }

    return true;
}

void ShaderProgramGL::attachShader(ShaderPtr shader) {
    ShaderGL *shaderGL = (ShaderGL*)shader.get();
    glAttachShader(shaderProgramID, shaderGL->getShaderID());
    shaders.push_back(shader);
}

void ShaderProgramGL::detachShader(ShaderPtr shader) {
    ShaderGL *shaderGL = (ShaderGL*)shader.get();
    glDetachShader(shaderProgramID, shaderGL->getShaderID());
    for (auto it = shaders.begin(); it != shaders.end(); ++it) {
        if (shader == *it) {
            shaders.erase(it);
            break;
        }
    }
}

void ShaderProgramGL::bind() {
    RendererGL *rendererGL = (RendererGL*)Renderer;
    rendererGL->useShaderProgram(this);
}


void ShaderProgramGL::dispatchCompute(int numGroupsX, int numGroupsY, int numGroupsZ) {
    this->bind();
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}



bool ShaderProgramGL::hasUniform(const char *name) {
    return getUniformLoc(name) >= 0;
}

int ShaderProgramGL::getUniformLoc(const char *name) {
    auto it = uniforms.find(name);
    if (it != uniforms.end()) {
        return it->second;
    }

    GLint uniformLoc = glGetUniformLocation(shaderProgramID, name);
    if (uniformLoc == -1) {
        return -1;
    }
    uniforms[name] = uniformLoc;
    return uniformLoc;
}

int ShaderProgramGL::getUniformLoc_error(const char *name) {
    int location = getUniformLoc(name);
    if (location == -1) {
        Logfile::get()->writeError(std::string() + "ERROR: ShaderProgramGL::setUniform: "
                + "No uniform variable called \"" + name + "\" in this shader program.");
    }
    return location;
}

bool ShaderProgramGL::setUniform(const char *name, int value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::ivec2 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::ivec3 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::ivec4 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, unsigned int value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::uvec2 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::uvec3 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::uvec4 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, bool value) {
    return setUniform(getUniformLoc_error(name), (int)value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::bvec2 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::bvec3 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::bvec4 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, float value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::vec2 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::vec3 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::vec4 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::mat4 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::mat3 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::mat3x4 &value) {
    return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const TexturePtr &value, int textureUnit /* = 0 */) {
    return setUniform(getUniformLoc_error(name), value, textureUnit);
}

bool ShaderProgramGL::setUniform(const char *name, const Color &value) {
    return setUniform(getUniformLoc_error(name), value);
}


bool ShaderProgramGL::setUniformArray(const char *name, const int *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const unsigned int *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const bool *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const float *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const glm::vec2 *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const glm::vec3 *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const glm::vec4 *value, size_t num) {
    return setUniformArray(getUniformLoc_error(name), value, num);
}



bool ShaderProgramGL::setUniform(int location, int value) {
    bind();
    glUniform1i(location, value);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::ivec2 &value) {
    bind();
    glUniform2i(location, value.x, value.y);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::ivec3 &value) {
    bind();
    glUniform3i(location, value.x, value.y, value.z);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::ivec4 &value) {
    bind();
    glUniform4i(location, value.x, value.y, value.z, value.w);
    return true;
}

bool ShaderProgramGL::setUniform(int location, unsigned int value) {
    bind();
    glUniform1ui(location, value);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::uvec2 &value) {
    bind();
    glUniform2ui(location, value.x, value.y);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::uvec3 &value) {
    bind();
    glUniform3ui(location, value.x, value.y, value.z);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::uvec4 &value) {
    bind();
    glUniform4ui(location, value.x, value.y, value.z, value.w);
    return true;
}

bool ShaderProgramGL::setUniform(int location, bool value) {
    bind();
    glUniform1i(location, value);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::bvec2 &value) {
    bind();
    glUniform2i(location, value.x, value.y);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::bvec3 &value) {
    bind();
    glUniform3i(location, value.x, value.y, value.z);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::bvec4 &value) {
    bind();
    glUniform4i(location, value.x, value.y, value.z, value.w);
    return true;
}

bool ShaderProgramGL::setUniform(int location, float value) {
    bind();
    glUniform1f(location, value);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::vec2 &value) {
    bind();
    glUniform2f(location, value.x, value.y);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::vec3 &value) {
    bind();
    glUniform3f(location, value.x, value.y, value.z);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::vec4 &value) {
    bind();
    glUniform4f(location, value.x, value.y, value.z, value.w);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::mat3 &value) {
    bind();
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(value));
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::mat3x4 &value) {
    bind();
    glUniformMatrix3x4fv(location, 1, false, glm::value_ptr(value));
    return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::mat4 &value) {
    bind();
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(value));
    return true;
}

bool ShaderProgramGL::setUniform(int location, const TexturePtr &value, int textureUnit /* = 0 */) {
    bind();
    Renderer->bindTexture(value, textureUnit);
    glUniform1i(location, textureUnit);
    return true;
}

bool ShaderProgramGL::setUniform(int location, const Color &value) {
    bind();
    float color[] = { value.getFloatR(), value.getFloatG(), value.getFloatB(), value.getFloatA() };
    glUniform4fv(location, 1, color);
    return true;
}


bool ShaderProgramGL::setUniformArray(int location, const int *value, size_t num) {
    bind();
    glUniform1iv(location, GLsizei(num), value);
    return true;
}

bool ShaderProgramGL::setUniformArray(int location, const unsigned int *value, size_t num) {
    bind();
    glUniform1uiv(location, GLsizei(num), value);
    return true;
}


bool ShaderProgramGL::setUniformArray(int location, const bool *value, size_t num) {
    bind();
    glUniform1iv(location, GLsizei(num), (int*)value);
    return true;
}

bool ShaderProgramGL::setUniformArray(int location, const float *value, size_t num) {
    bind();
    glUniform1fv(location, GLsizei(num), value);
    return true;
}

bool ShaderProgramGL::setUniformArray(int location, const glm::vec2 *value, size_t num) {
    bind();
    glUniform2fv(location, GLsizei(num), (float*)value);
    return true;
}

bool ShaderProgramGL::setUniformArray(int location, const glm::vec3 *value, size_t num) {
    bind();
    glUniform3fv(location, GLsizei(num), (float*)value);
    return true;
}

bool ShaderProgramGL::setUniformArray(int location, const glm::vec4 *value, size_t num) {
    bind();
    glUniform4fv(location, GLsizei(num), (float*)value);
    return true;
}



void ShaderProgramGL::setUniformImageTexture(
        unsigned int unit, const TexturePtr& texture, unsigned int format /*= GL_RGBA8 */,
        unsigned int access /* = GL_READ_WRITE */, unsigned int level /* = 0 */,
        bool layered /* = false */, unsigned int layer /* = 0 */) {
    glBindImageTexture(
            unit, ((TextureGL*)texture.get())->getTexture(),
            GLint(level), layered, GLint(layer), access, format);
}



// OpenGL 3 Uniform Buffers & OpenGL 4 Shader Storage Buffers
bool ShaderProgramGL::setUniformBuffer(int binding, int location, const GeometryBufferPtr &geometryBuffer) {
    // Binding point is unique for _all_ shaders
    ShaderManager->bindUniformBuffer(binding, geometryBuffer);

    // Location is set per shader (by name, explicitly, or by layout modifier)
    glUniformBlockBinding(shaderProgramID, location, binding);

    return true;
}

bool ShaderProgramGL::setUniformBuffer(int binding, const char *name, const GeometryBufferPtr &geometryBuffer) {
    // Block index (aka location in the shader) can be queried by name in the shader
    unsigned int blockIndex = glGetUniformBlockIndex(shaderProgramID, name);
    if (blockIndex == GL_INVALID_INDEX) {
        Logfile::get()->writeError(std::string() + "ERROR: ShaderProgramGL::setUniformBuffer: "
                                   + "No uniform block called \"" + name + "\" in this shader program.");
    }
    return setUniformBuffer(binding, blockIndex, geometryBuffer);
}


bool ShaderProgramGL::setAtomicCounterBuffer(int binding, const GeometryBufferPtr &geometryBuffer) {
    // Binding point is unique for _all_ shaders
    ShaderManager->bindAtomicCounterBuffer(binding, geometryBuffer);

    // Set location to resource index per shader
    //glShaderStorageBlockBinding(shaderProgramID, location, binding);
    return true;
}

/*bool ShaderProgramGL::setAtomicCounterBuffer(int binding, const char *name, const GeometryBufferPtr &geometryBuffer) {
    // Resource index (aka location in the shader) can be queried by name in the shader
    unsigned int resourceIndex = glGetProgramResourceIndex(shaderProgramID, GL_ATOMIC_COUNTER_BUFFER, name);
    return setAtomicCounterBuffer(binding, resourceIndex, geometryBuffer);
}*/


bool ShaderProgramGL::setShaderStorageBuffer(int binding, int location, const GeometryBufferPtr &geometryBuffer) {
    // Binding point is unique for _all_ shaders
    ShaderManager->bindShaderStorageBuffer(binding, geometryBuffer);

    // Set location to resource index per shader
    glShaderStorageBlockBinding(shaderProgramID, location, binding);

    return true;
}

bool ShaderProgramGL::setShaderStorageBuffer(int binding, const char *name, const GeometryBufferPtr &geometryBuffer) {
    // Resource index (aka location in the shader) can be queried by name in the shader
    unsigned int resourceIndex = glGetProgramResourceIndex(shaderProgramID, GL_SHADER_STORAGE_BLOCK, name);
    if (resourceIndex == GL_INVALID_INDEX) {
        Logfile::get()->writeError(std::string() + "ERROR: ShaderProgramGL::setShaderStorageBuffer: "
                                   + "No shader storage buffer called \"" + name + "\" in this shader program.");
    }
    return setShaderStorageBuffer(binding, resourceIndex, geometryBuffer);
}

}
