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

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <streambuf>

#include <GL/glew.h>

#include <Utils/Convert.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include "Shader.hpp"
#include "SystemGL.hpp"
#include "ShaderAttributes.hpp"
#include "ShaderManager.hpp"

namespace sgl {

ShaderManagerGL::ShaderManagerGL() {
    pathPrefix = sgl::AppSettings::get()->getDataDirectory() + "Shaders/";
    indexFiles(pathPrefix);

    // Was a file called "GlobalDefines.glsl" found? If yes, store its content in "globalDefines".
    auto it = shaderFileMap.find("GlobalDefines.glsl");
    if (it != shaderFileMap.end()) {
        std::ifstream file(it->second);
        if (!file.is_open()) {
            Logfile::get()->writeError(
                    std::string() + "ShaderManagerGL::ShaderManagerGL: Unexpected error "
                    "occured while loading \"GlobalDefines.glsl\".");
        }
        globalDefines = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    // Query compute shader capabilities
    maxComputeWorkGroupCount.resize(3);
    maxComputeWorkGroupSize.resize(3);
    for (int i = 0; i < 3; i++) {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, (&maxComputeWorkGroupCount.front()) + i);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, (&maxComputeWorkGroupSize.front()) + i);
    }
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
}

ShaderManagerGL::~ShaderManagerGL() {
}

const std::vector<int> &ShaderManagerGL::getMaxComputeWorkGroupCount() {
    return maxComputeWorkGroupCount;
}

const std::vector<int> &ShaderManagerGL::getMaxComputeWorkGroupSize() {
    return maxComputeWorkGroupSize;
}

int ShaderManagerGL::getMaxWorkGroupInvocations() {
    return maxWorkGroupInvocations;
}

void ShaderManagerGL::unbindShader() {
    glUseProgram(0);
}

void ShaderManagerGL::invalidateBindings() {
    uniformBuffers.clear();
    atomicCounterBuffers.clear();
    shaderStorageBuffers.clear();
}


void ShaderManagerGL::bindUniformBuffer(int binding, const GeometryBufferPtr &geometryBuffer) {
    GLuint bufferID = static_cast<GeometryBufferGL*>(geometryBuffer.get())->getBuffer();

    auto it = uniformBuffers.find(binding);
    if (uniformBuffers.find(binding) != uniformBuffers.end()
            && static_cast<GeometryBufferGL*>(it->second.get())->getBuffer() == bufferID
            && geometryBuffer.get() == it->second.get()) {
        // Already bound
        return;
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, binding, bufferID);
    uniformBuffers[binding] = geometryBuffer;
}

void ShaderManagerGL::bindAtomicCounterBuffer(int binding, const GeometryBufferPtr &geometryBuffer) {
    GLuint bufferID = static_cast<GeometryBufferGL*>(geometryBuffer.get())->getBuffer();

    auto it = atomicCounterBuffers.find(binding);
    if (atomicCounterBuffers.find(binding) != atomicCounterBuffers.end()
            && static_cast<GeometryBufferGL*>(it->second.get())->getBuffer() == bufferID
            && geometryBuffer.get() == it->second.get()) {
        // Already bound
        return;
    }

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, bufferID);
    atomicCounterBuffers[binding] = geometryBuffer;
}

void ShaderManagerGL::bindShaderStorageBuffer(int binding, const GeometryBufferPtr &geometryBuffer) {
    GLuint bufferID = static_cast<GeometryBufferGL*>(geometryBuffer.get())->getBuffer();

    auto it = shaderStorageBuffers.find(binding);
    if (shaderStorageBuffers.find(binding) != shaderStorageBuffers.end()
            && static_cast<GeometryBufferGL*>(it->second.get())->getBuffer() == bufferID
            && geometryBuffer.get() == it->second.get()) {
        // Already bound
        return;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, bufferID);
    shaderStorageBuffers[binding] = geometryBuffer;
}




static bool dumpTextDebugStatic = false;
ShaderProgramPtr ShaderManagerGL::createShaderProgram(const std::vector<std::string> &shaderIDs, bool dumpTextDebug) {
    ShaderProgramPtr shaderProgram = createShaderProgram();
    dumpTextDebugStatic = dumpTextDebug;

    for (const std::string &shaderID : shaderIDs) {
        ShaderPtr shader;
        std::string shaderID_lower = sgl::toLowerCopy(shaderID);
        ShaderType shaderType = VERTEX_SHADER;
        if (sgl::endsWith(shaderID_lower, "vertex")) {
            shaderType = VERTEX_SHADER;
        } else if (sgl::endsWith(shaderID_lower, "fragment")) {
            shaderType = FRAGMENT_SHADER;
        } else if (sgl::endsWith(shaderID_lower, "geometry")) {
            shaderType = GEOMETRY_SHADER;
        } else if (sgl::endsWith(shaderID_lower, "tesselationevaluation")) {
            shaderType = TESSELATION_EVALUATION_SHADER;
        } else if (sgl::endsWith(shaderID_lower, "tesselationcontrol")) {
            shaderType = TESSELATION_CONTROL_SHADER;
        } else if (sgl::endsWith(shaderID_lower, "compute")) {
            shaderType = COMPUTE_SHADER;
        } else {
            if (sgl::stringContains(shaderID_lower, "vert")) {
                shaderType = VERTEX_SHADER;
            } else if (sgl::stringContains(shaderID_lower, "frag")) {
                shaderType = FRAGMENT_SHADER;
            } else if (sgl::stringContains(shaderID_lower, "geom")) {
                shaderType = GEOMETRY_SHADER;
            } else if (sgl::stringContains(shaderID_lower, "tess")) {
                if (sgl::stringContains(shaderID_lower, "eval")) {
                    shaderType = TESSELATION_EVALUATION_SHADER;
                } else if (sgl::stringContains(shaderID_lower, "control")) {
                    shaderType = TESSELATION_CONTROL_SHADER;
                }
            } else if (sgl::stringContains(shaderID_lower, "comp")) {
                shaderType = COMPUTE_SHADER;
            } else {
                Logfile::get()->writeError(
                        std::string() + "ERROR: ShaderManagerGL::createShaderProgram: "
                        + "Unknown shader type (id: \"" + shaderID + "\")");
            }
        }
        shader = getShader(shaderID.c_str(), shaderType);
        shaderProgram->attachShader(shader);
    }
    dumpTextDebugStatic = false;
    shaderProgram->linkProgram();
    return shaderProgram;
}

ShaderPtr ShaderManagerGL::loadAsset(ShaderInfo &shaderInfo) {
    std::string id = shaderInfo.filename;
    std::string shaderString = getShaderString(id);

    if (dumpTextDebugStatic) {
        std::cout << "Shader dump (" << id << "):" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << shaderString << std::endl << std::endl;
    }

    ShaderGL *shaderGL = new ShaderGL(shaderInfo.shaderType);
    ShaderPtr shader(shaderGL);
    shaderGL->setShaderText(shaderString);
    shaderGL->setFileID(shaderInfo.filename.c_str());
    shaderGL->compile();
    return shader;
}

ShaderPtr ShaderManagerGL::createShader(ShaderType sh) {
    ShaderPtr shader(new ShaderGL(sh));
    return shader;
}

ShaderProgramPtr ShaderManagerGL::createShaderProgram() {
    ShaderProgramPtr shaderProg(new ShaderProgramGL());
    return shaderProg;
}

ShaderAttributesPtr ShaderManagerGL::createShaderAttributes(ShaderProgramPtr &shader) {
    if (SystemGL::get()->openglVersionMinimum(3,0)) {
        return ShaderAttributesPtr(new ShaderAttributesGL3(shader));
    }
    return ShaderAttributesPtr(new ShaderAttributesGL2(shader));
}



std::string ShaderManagerGL::loadHeaderFileString(const std::string &shaderName, std::string &prependContent) {
    std::ifstream file(shaderName.c_str());
    if (!file.is_open()) {
        Logfile::get()->writeError(std::string() + "Error in loadHeaderFileString: Couldn't open the file \""
                + shaderName + "\".");
        return "";
    }
    //std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string fileContent = "#line 1\n";

    // Support preprocessor for embedded headers
    std::string linestr;
    int lineNum = 1;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (linestr.size() > 0 && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        lineNum++;

        if (sgl::startsWith(linestr, "#include")) {
            std::string includedFileName = getShaderFileName(getHeaderName(linestr));
            std::string includedFileContent = loadHeaderFileString(includedFileName, prependContent);
            fileContent += includedFileContent + "\n";
            fileContent += std::string() + "#line " + toString(lineNum) + "\n";
        } else if (sgl::startsWith(linestr, "#extension") || sgl::startsWith(linestr, "#version")) {
            prependContent += linestr + "\n";
            fileContent = std::string() + fileContent + "#line " + toString(lineNum) + "\n";
        } else {
            fileContent += std::string() + linestr + "\n";
        }
    }


    file.close();
    fileContent = fileContent;
    return fileContent;
}


std::string ShaderManagerGL::getHeaderName(const std::string &lineString) {
    // Filename in quotes?
    auto startFilename = lineString.find("\"");
    auto endFilename = lineString.find_last_of("\"");
    if (startFilename != std::string::npos && endFilename != std::string::npos) {
        return lineString.substr(startFilename+1, endFilename-startFilename-1);
    } else {
        // Filename is user-specified #define directive?
        std::vector<std::string> line;
        sgl::splitStringWhitespace(lineString, line);
        if (line.size() < 2) {
            Logfile::get()->writeError("Error in ShaderManagerGL::getHeaderFilename: Too few tokens.");
            return "";
        }

        auto it = preprocessorDefines.find(line.at(1));
        if (it != preprocessorDefines.end()) {
            auto startFilename = it->second.find("\"");
            auto endFilename = it->second.find_last_of("\"");
            return it->second.substr(startFilename+1, endFilename-startFilename-1);
        } else {
            Logfile::get()->writeError("Error in ShaderManagerGL::getHeaderFilename: Invalid include directive.");
            Logfile::get()->writeError(std::string() + "Line string: " + lineString);
            return "";
        }
    }
}





void ShaderManagerGL::indexFiles(const std::string &file) {
    if (FileUtils::get()->isDirectory(file)) {
        // Scan content of directory
        std::vector<std::string> elements = FileUtils::get()->getFilesInDirectoryVector(file);
        for (std::string &childFile : elements) {
            indexFiles(childFile);
        }
    } else if (FileUtils::get()->hasExtension(file.c_str(), ".glsl")) {
        // File to index. "fileName" is name without path.
        std::string fileName = FileUtils::get()->getPureFilename(file);
        shaderFileMap.insert(make_pair(fileName, file));
    }
}


std::string ShaderManagerGL::getShaderFileName(const std::string &pureFilename) {
    auto it = shaderFileMap.find(pureFilename);
    if (it == shaderFileMap.end()) {
        sgl::Logfile::get()->writeError("Error in ShaderManagerGL::getShaderFileName: Unknown file name \""
                + pureFilename + "\".");
        return "";
    }
    return it->second;
}


std::string ShaderManagerGL::getPreprocessorDefines() {
    std::string preprocessorStatements;
    for (auto it = preprocessorDefines.begin(); it != preprocessorDefines.end(); it++) {
        preprocessorStatements += std::string() + "#define " + it->first + " " + it->second + "\n";
    }
    return preprocessorStatements + globalDefines;
}


std::string ShaderManagerGL::getShaderString(const std::string &globalShaderName) {
    auto it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    auto filenameEnd = globalShaderName.find(".");
    std::string pureFilename = globalShaderName.substr(0, filenameEnd);
    std::string shaderFilename = getShaderFileName(pureFilename + ".glsl");
    std::string shaderInternalID = globalShaderName.substr(filenameEnd+1);

    std::ifstream file(shaderFilename.c_str());
    if (!file.is_open()) {
        Logfile::get()->writeError(std::string() + "Error in getShader: Couldn't open the file \""
                + shaderFilename + "\".");
    }

    std::string shaderName;
    std::string shaderContent = "#line 1\n";
    std::string prependContent;
    int lineNum = 1;
    std::string linestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        lineNum++;

        if (sgl::startsWith(linestr, "-- ")) {
            if (!shaderContent.empty() && !shaderName.empty()) {
                shaderContent = prependContent + shaderContent;
                effectSources.insert(make_pair(shaderName, shaderContent));
            }

            shaderName = pureFilename + "." + linestr.substr(3);
            shaderContent = std::string() + getPreprocessorDefines() + "#line " + toString(lineNum) + "\n";
            prependContent = "";
        } else if (sgl::startsWith(linestr, "#version") || sgl::startsWith(linestr, "#extension")) {
            prependContent += linestr + "\n";
            shaderContent = std::string() + shaderContent + "#line " + toString(lineNum) + "\n";
        } else if (sgl::startsWith(linestr, "#include")) {
            std::string includedFileName = getShaderFileName(getHeaderName(linestr));
            std::string includedFileContent = loadHeaderFileString(includedFileName, prependContent);
            shaderContent += includedFileContent + "\n";
            shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
        } else {
            shaderContent += std::string() + linestr + "\n";
        }
    }
    shaderContent = prependContent + shaderContent;
    file.close();

    if (!shaderName.empty()) {
        effectSources.insert(make_pair(shaderName, shaderContent));
    } else {
        effectSources.insert(make_pair(pureFilename + ".glsl", shaderContent));
    }

    it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    Logfile::get()->writeError(std::string() + "Error in getShader: Couldn't find the shader \""
            + globalShaderName + "\".");
    return "";
}

}
