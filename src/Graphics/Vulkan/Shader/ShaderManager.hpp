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

#include <Utils/File/FileManager.hpp>
#include "../libs/volk/volk.h"
#include "Shader.hpp"

namespace shaderc { class Compiler; }

namespace sgl { namespace vk {

struct DLL_OBJECT ShaderModuleInfo {
    std::string filename;
    ShaderModuleType shaderModuleType;
    bool operator <(const ShaderModuleInfo& rhs) const {
        return filename < rhs.filename;
    }
};

/// Wrapper for shaderc_optimization_level.
enum class ShaderOptimizationLevel {
    ZERO, //< No optimization ('-O0' when using glslc).
    SIZE, //< Optimize code size ('-Os' when using glslc).
    PERFORMANCE //< Optimize performance ('-O' when using glslc).
};

class DLL_OBJECT ShaderManagerVk : public FileManager<ShaderModule, ShaderModuleInfo> {
public:
    explicit ShaderManagerVk(Device* device);
    ~ShaderManagerVk() override;

    /// Reference-counted loading.
    /// If dumpTextDebug, the pre-processed source will be dumped on the command line.
    ShaderStagesPtr getShaderStages(const std::vector<std::string>& shaderIds, bool dumpTextDebug = false);
    ShaderStagesPtr getShaderStages(
            const std::vector<std::string>& shaderIds,
            const std::map<std::string, std::string>& customPreprocessorDefines, bool dumpTextDebug = false);
    ShaderModulePtr getShaderModule(const std::string& shaderId, ShaderModuleType shaderModuleType);
    ShaderModulePtr getShaderModule(
            const std::string& shaderId, ShaderModuleType shaderModuleType,
            const std::map<std::string, std::string>& customPreprocessorDefines);

    //virtual ShaderAttributesPtr createShaderAttributes(ShaderStagesPtr& shader)=0;

    /**
     * Used for adding preprocessor defines to all shader files before compiling.
     * This function is useful for e.g. switching at runtime between different different techniques.
     * The generated preprocessor statements are of the form "#define <token> <value>".
     */
    template<typename T>
    void addPreprocessorDefine(const std::string& token, const T& value) {
        preprocessorDefines[token] = toString(value);
    }
    void addPreprocessorDefine(const std::string& token) {
        preprocessorDefines[token] = "";
    }
    std::string getPreprocessorDefine(const std::string& token) {
        return preprocessorDefines[token];
    }

    /// Removes a preprocessor #define token set by "addPreprocessorDefine".
    void removePreprocessorDefine(const std::string& token) {
        auto it = preprocessorDefines.find(token);
        if (it != preprocessorDefines.end()) {
            preprocessorDefines.erase(it);
        }
    }

    /// Setting generateDebugInfo to true corresponds to the glslc flag '-g'.
    inline void setGenerateDebugInfo(bool _generateDebugInfo = true) { generateDebugInfo = _generateDebugInfo; }
    /// The different optimization levels correspond to the flags '-O0', '-Os' and '-O' when using glslc.
    inline void setOptimizationLevel(ShaderOptimizationLevel _shaderOptimizationLevel) {
        isOptimizationLevelSet = true;
        shaderOptimizationLevel = _shaderOptimizationLevel;
    }
    inline void resetOptimizationLevel() { isOptimizationLevelSet = false; }

    /**
     * Deletes all cached shaders in the ShaderManager. This is necessary, e.g., when wanting to switch to a
     * different rendering technique with "addPreprocessorDefine" after already having loaded a certain shader.
     * Already loaded shaders will stay intact thanks to reference counting.
     */
    virtual void invalidateShaderCache();

    // For use by IncluderInterface.
    [[nodiscard]] const std::map<std::string, std::string>& getShaderFileMap() const { return shaderFileMap; }
    [[nodiscard]] const std::string& getShaderPathPrefix() const { return pathPrefix; }

protected:
    ShaderModulePtr loadAsset(ShaderModuleInfo& shaderModuleInfo) override;
    ShaderStagesPtr createShaderStages(const std::vector<std::string>& shaderIds, bool dumpTextDebug);

    /// Internal loading
    std::string loadHeaderFileString(const std::string& shaderName, std::string& prependContent);
    std::string getHeaderName(const std::string& lineString);
    std::string getImportedShaderString(
            const std::string& moduleName, const std::string& parentModuleName, std::string& prependContent);
    std::string getShaderString(const std::string& globalShaderName);
    std::string getPreprocessorDefines(ShaderModuleType shaderModuleType);

    /**
     * Indexes all ".glsl" files in the directory pathPrefix (and its sub-directories recursively) to create
     * "shaderFileMap". Therefore, the application can easily include files with relative paths.
     */
    void indexFiles(const std::string& file);
    std::string getShaderFileName(const std::string& pureFilename);

    /// Directory in which to search for shaders (standard: Data/Shaders).
    std::string pathPrefix;

    /// Maps shader name -> shader source, e.g. "Blur.Fragment" -> "void main() { ... }".
    std::map<std::string, std::string> effectSources;
    std::map<std::string, std::string> effectSourcesRaw; ///< without prepended header.
    std::map<std::string, std::string> effectSourcesPrepend; ///< only prepended header.

    /// Maps file names without path to full file paths for "*.glsl" shader files,
    /// e.g. "Blur.glsl" -> "Data/Shaders/PostProcessing/Blur.glsl".
    std::map<std::string, std::string> shaderFileMap;
    int sourceStringNumber = 0;
    int recursionDepth = 0;

    // If a file named "GlobalDefinesVulkan.glsl" is found: Appended to all shaders.
    std::string globalDefines;
    // Global defines for vertex and geometry shaders.
    std::string globalDefinesMvpMatrices;

    /// A token-value map for user-provided preprocessor #define's
    std::map<std::string, std::string> preprocessorDefines;
    std::map<std::string, std::string> tempPreprocessorDefines; // Temporarily set when loading a shader.

    // Vulkan device.
    Device* device = nullptr;

    // Shader module compiler.
    shaderc::Compiler* shaderCompiler = nullptr;
    bool generateDebugInfo = false;
    bool isOptimizationLevelSet = false;
    ShaderOptimizationLevel shaderOptimizationLevel = ShaderOptimizationLevel::PERFORMANCE;
};

DLL_OBJECT extern ShaderManagerVk* ShaderManager;

}}

#endif //SGL_SHADERMANAGER_HPP
