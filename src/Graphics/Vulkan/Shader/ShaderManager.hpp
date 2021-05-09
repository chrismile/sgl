/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#ifndef SGL_SHADERMANAGER_HPP
#define SGL_SHADERMANAGER_HPP

#include <string>
#include <vector>
#include <map>

#include <vulkan/vulkan.h>

#include <Utils/File/FileManager.hpp>
#include "Shader.hpp"

namespace sgl { namespace vk {

struct DLL_OBJECT ShaderModuleInfo {
    std::string filename;
    ShaderModuleType shaderType;
    bool operator <(const ShaderModuleInfo &rhs) const {
        return filename < rhs.filename;
    }
};

class DLL_OBJECT ShaderManager : public FileManager<ShaderModule, ShaderModuleInfo> {
public:
    /// Reference-counted loading
    /// If dumpTextDebug, the pre-processed source will be dumped on the command line.
    ShaderStagesPtr getShaderStages(const std::vector<std::string> &shaderIds, bool dumpTextDebug = false);
    ShaderModulePtr getShaderModule(const std::string& shaderId, ShaderModuleType shaderModuleType);

    /// Create shader/program (manual loading)
    virtual ShaderModulePtr createShader(ShaderModuleType shaderModuleType)=0;
    virtual ShaderStagesPtr createShaderStages()=0;
    //virtual ShaderAttributesPtr createShaderAttributes(ShaderStagesPtr &shader)=0;

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

    // For use by IncluderInterface.
    const std::map<std::string, std::string>& getShaderFileMap() const { return shaderFileMap; }
    const std::string& getShaderPathPrefix() const { return pathPrefix; }

protected:
    ShaderModulePtr loadAsset(ShaderModuleInfo &shaderModuleInfo);
    ShaderStagesPtr createShaderStages(const std::vector<std::string> &shaderIds, bool dumpTextDebug);

    /// Internal loading
    std::string loadHeaderFileString(const std::string &shaderName, std::string &prependContent);
    std::string getHeaderName(const std::string &lineString);
    std::string getShaderString(const std::string &globalShaderName);
    std::string getPreprocessorDefines();

    /**
     * Indexes all ".glsl" files in the directory pathPrefix (and its sub-directories recursively) to create
     * "shaderFileMap". Therefore, the application can easily include files with relative paths.
     */
    void indexFiles(const std::string &file);
    std::string getShaderFileName(const std::string &pureFilename);

    /// Directory in which to search for shaders (standard: Data/Shaders).
    std::string pathPrefix;

    /// Maps shader name -> shader source, e.g. "Blur.Fragment" -> "void main() { ... }".
    std::map<std::string, std::string> effectSources;

    /// Maps file names without path to full file paths for "*.glsl" shader files,
    /// e.g. "Blur.glsl" -> "Data/Shaders/PostProcessing/Blur.glsl".
    std::map<std::string, std::string> shaderFileMap;

    // If a file named "GlobalDefines.glsl" is found: Appended to all shaders.
    std::string globalDefines;

    /// A token-value map for user-provided preprocessor #define's
    std::map<std::string, std::string> preprocessorDefines;
};

}}

#endif //SGL_SHADERMANAGER_HPP
