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
#include <unordered_map>

#include <Utils/File/FileManager.hpp>
#include "../libs/volk/volk.h"
#include "Shader.hpp"

#ifdef SUPPORT_SHADERC_BACKEND
namespace shaderc { class Compiler; }
#endif

namespace sgl { class PreprocessorGlsl; }

namespace sgl { namespace vk {

struct DLL_OBJECT ShaderModuleInfo {
    std::string filename;
    ShaderModuleType shaderModuleType;
    bool operator <(const ShaderModuleInfo& rhs) const {
        return filename < rhs.filename;
    }
};

/**
 * Both shaderc and glslang can be used as shader compiler backends.
 */
enum class ShaderCompilerBackend {
    SHADERC, GLSLANG
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
    void setShaderCompilerBackend(ShaderCompilerBackend backend);

    /// Reference-counted loading.
    /// If dumpTextDebug, the pre-processed source will be dumped on the command line.
    ShaderStagesPtr getShaderStages(const std::vector<std::string>& shaderIds, bool dumpTextDebug = false);
    ShaderStagesPtr getShaderStages(
            const std::vector<std::string>& shaderIds, uint32_t subgroupSize, bool dumpTextDebug = false);
    ShaderStagesPtr getShaderStages(
            const std::vector<std::string>& shaderIds,
            const std::map<std::string, std::string>& customPreprocessorDefines, bool dumpTextDebug = false);
    ShaderStagesPtr getShaderStages(
            const std::vector<std::string>& shaderIds,
            const std::map<std::string, std::string>& customPreprocessorDefines,
            uint32_t subgroupSize, bool dumpTextDebug = false);
    ShaderStagesPtr getShaderStagesWithSettings(
            const std::vector<std::string>& shaderIds,
            const std::map<std::string, std::string>& customPreprocessorDefines,
            const std::vector<ShaderStageSettings>& settings, bool dumpTextDebug = false);
    ShaderModulePtr getShaderModule(const std::string& shaderId, ShaderModuleType shaderModuleType);
    ShaderModulePtr getShaderModule(
            const std::string& shaderId, ShaderModuleType shaderModuleType,
            const std::map<std::string, std::string>& customPreprocessorDefines);

    /// Cached compilation of compute shaders.
    ShaderStagesPtr compileComputeShaderFromStringCached(const std::string& shaderId, const std::string& shaderString);

    //virtual ShaderAttributesPtr createShaderAttributes(ShaderStagesPtr& shader)=0;

    /**
     * Used for adding preprocessor defines to all shader files before compiling.
     * This function is useful for e.g. switching at runtime between different different techniques.
     * The generated preprocessor statements are of the form "#define <token> <value>".
     */
    template<typename T>
    void addPreprocessorDefine(const std::string& token, const T& value) {
        addPreprocessorDefine(token, toString(value));
    }
    void addPreprocessorDefine(const std::string& token, const std::string& value);
    void addPreprocessorDefine(const std::string& token);
    std::string getPreprocessorDefine(const std::string& token);
    /// Removes a preprocessor #define token set by "addPreprocessorDefine".
    void removePreprocessorDefine(const std::string& token);

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
    [[nodiscard]] const std::map<std::string, std::string>& getShaderFileMap() const;
    [[nodiscard]] const std::string& getShaderPathPrefix() const { return pathPrefix; }

    /**
     * @see After indexFiles was called by the constructor, this function can be used to resolve a shader file path.
     */
    std::string getShaderFileName(const std::string& pureFilename);

    std::string loadHeaderFileString(const std::string& shaderName, std::string& prependContent);
    std::string loadHeaderFileString(
            const std::string& shaderName, const std::string& headerName, std::string& prependContent);

protected:
    ShaderModulePtr loadAsset(ShaderModuleInfo& shaderModuleInfo) override;
#ifdef SUPPORT_SHADERC_BACKEND
    ShaderModulePtr loadAssetShaderc(
            ShaderModuleInfo& shaderInfo, const std::string& id, const std::string& shaderString);
#endif
#ifdef SUPPORT_GLSLANG_BACKEND
    ShaderModulePtr loadAssetGlslang(
            ShaderModuleInfo& shaderInfo, const std::string& id, const std::string& shaderString);
#endif
    ShaderStagesPtr createShaderStages(const std::vector<std::string>& shaderIds, bool dumpTextDebug);
    ShaderStagesPtr createShaderStages(
            const std::vector<std::string>& shaderIds, const std::vector<ShaderStageSettings>& shaderStageSettings,
            bool dumpTextDebug);

    /**
     * Indexes all ".glsl" files in the directory pathPrefix (and its sub-directories recursively) to create
     * "shaderFileMap". Therefore, the application can easily include files with relative paths.
     */
    void indexFiles(std::map<std::string, std::string>& shaderFileMap, const std::string& file);

    // If a file named "GlobalDefinesVulkan.glsl" is found: Appended to all shaders.
    //std::string globalDefines;
    // Global defines for vertex and geometry shaders.
    //std::string globalDefinesMvpMatrices;

    /// A token-value map for user-provided preprocessor #define's
    //std::map<std::string, std::string> preprocessorDefines;
    //std::map<std::string, std::string> tempPreprocessorDefines; // Temporarily set when loading a shader.

    // Vulkan device.
    Device* device = nullptr;

    /// Directory in which to search for shaders (standard: Data/Shaders).
    std::string pathPrefix;
    PreprocessorGlsl* preprocessor = nullptr;

    // Shader module compiler.
#ifdef SUPPORT_SHADERC_BACKEND
    ShaderCompilerBackend shaderCompilerBackend = ShaderCompilerBackend::SHADERC;
#else
    ShaderCompilerBackend shaderCompilerBackend = ShaderCompilerBackend::GLSLANG;
#endif
    bool generateDebugInfo = false;
    bool isOptimizationLevelSet = false;
    bool isFirstShaderCompilation = true;
    ShaderOptimizationLevel shaderOptimizationLevel = ShaderOptimizationLevel::PERFORMANCE;

#ifdef SUPPORT_SHADERC_BACKEND
    shaderc::Compiler* shaderCompiler = nullptr;
#endif

    /// @see compileComputeShaderFromStringCached
    std::unordered_map<std::string, ShaderStagesPtr> cachedShadersLoadedFromDirectString;
};

DLL_OBJECT extern ShaderManagerVk* ShaderManager;

}}

#endif //SGL_SHADERMANAGER_HPP
