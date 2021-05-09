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

#ifndef GRAPHICS_SHADER_SHADERMANAGER_HPP_
#define GRAPHICS_SHADER_SHADERMANAGER_HPP_

#include <vector>
#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <Utils/Convert.hpp>
#include <Utils/Singleton.hpp>
#include <Utils/File/FileManager.hpp>
#include <Graphics/Shader/Shader.hpp>

namespace sgl {

struct DLL_OBJECT ShaderInfo {
    std::string filename;
    ShaderType shaderType;
    bool operator <(const ShaderInfo &rhs) const {
        return filename < rhs.filename;
    }
};

class DLL_OBJECT ShaderManagerInterface : public FileManager<Shader, ShaderInfo>
{
public:
    /// Reference-counted loading
    /// If dumpTextDebug, the pre-processed source will be dumped on the command line.
    ShaderProgramPtr getShaderProgram(const std::vector<std::string> &shaderIDs, bool dumpTextDebug = false);
    ShaderPtr getShader(const char *shaderID, ShaderType shaderType);

    /// Create shader/program (manual loading)
    virtual ShaderPtr createShader(ShaderType sh)=0;
    virtual ShaderProgramPtr createShaderProgram()=0;
    virtual ShaderAttributesPtr createShaderAttributes(ShaderProgramPtr &shader)=0;

    /// Make sure no shader is bound for rendering.
    virtual void unbindShader()=0;

    /**
     * Used for adding preprocessor defines to all shader files before compiling.
     * This function is useful for e.g. switching at runtime between different different techniques.
     * The generated preprocessor statements are of the form "#define <token> <value>".
     */
    template<typename T>
    void addPreprocessorDefine(const std::string &token, const T &value)
    {
        preprocessorDefines[token] = toString(value);
    }
    std::string getPreprocessorDefine(const std::string &token)
    {
        return preprocessorDefines[token];
    }

    /// Removes a preprocessor #define token set by "addPreprocessorDefine"
    void removePreprocessorDefine(const std::string &token) {
        auto it = preprocessorDefines.find(token);
        if (it != preprocessorDefines.end()) {
            preprocessorDefines.erase(it);
        }
    }


    /**
     * Deletes all cached shaders in the ShaderManager. This is necessary e.g. when wanting to switch to a
     * different rendering technique with "addPreprocessorDefine" after already loading a certain shader.
     * Already loaded shaders will stay intact thanks to reference counting.
     */
    virtual void invalidateShaderCache()=0;


    // --- Compute shader interface ---

    /// Array containing maximum work-group count in x,y,z that can be passed to glDispatchCompute.
    virtual const std::vector<int> &getMaxComputeWorkGroupCount()=0;
    /// Array containing maximum local work-group size (defined in shader with layout qualifier).
    virtual const std::vector<int> &getMaxComputeWorkGroupSize()=0;
    /// Maximum number of work group units of a local work group, e.g. 1024 local work items.
    virtual int getMaxWorkGroupInvocations()=0;


    // --- Shader program resources ---

    virtual void bindUniformBuffer(int binding, GeometryBufferPtr &geometryBuffer)=0;
    virtual void bindAtomicCounterBuffer(int binding, GeometryBufferPtr &geometryBuffer)=0;
    virtual void bindShaderStorageBuffer(int binding, GeometryBufferPtr &geometryBuffer)=0;


protected:
    virtual ShaderPtr loadAsset(ShaderInfo &shaderInfo)=0;
    virtual ShaderProgramPtr createShaderProgram(const std::vector<std::string> &shaderIDs, bool dumpTextDebug)=0;

    /// A token-value map for user-provided preprocessor #define's
    std::map<std::string, std::string> preprocessorDefines;
};

DLL_OBJECT extern ShaderManagerInterface *ShaderManager;

}

/*! GRAPHICS_SHADER_SHADERMANAGER_HPP_ */
#endif
