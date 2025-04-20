/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_WEBGPU_SHADERMANAGER_HPP
#define SGL_WEBGPU_SHADERMANAGER_HPP

#include <webgpu/webgpu.h>

#include <Utils/File/FileManager.hpp>
#include <Graphics/WebGPU/Utils/Device.hpp>

namespace sgl { class PreprocessorGlsl; }

namespace sgl { namespace webgpu {

class ShaderModule;
typedef std::shared_ptr<ShaderModule> ShaderModulePtr;
class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;

struct DLL_OBJECT ShaderModuleInfo {
    std::string filename;
    bool operator <(const ShaderModuleInfo& rhs) const {
        return filename < rhs.filename;
    }
};

enum class ShaderSource {
    WGSL, GLSL
};

class DLL_OBJECT ShaderManagerWgpu : public FileManager<ShaderModule, ShaderModuleInfo> {
public:
    explicit ShaderManagerWgpu(Device* device);
    ~ShaderManagerWgpu() override;

    /// Reference-counted loading.
    ShaderModulePtr getShaderModule(const std::string& shaderId);
    ShaderModulePtr getShaderModule(
            const std::string& shaderId, const std::map<std::string, std::string>& customPreprocessorDefines);
    ShaderModulePtr getShaderModule(
            const std::string& shaderId, const std::map<std::string, std::string>& customPreprocessorDefines,
            bool dumpTextDebug);
    ShaderStagesPtr getShaderStagesSingleSource(
            const std::string& shaderId, const std::vector<std::string>& entryPoints);
    ShaderStagesPtr getShaderStagesSingleSource(
            const std::string& shaderId, const std::vector<std::string>& entryPoints,
            const std::map<std::string, std::string>& customPreprocessorDefines);
    ShaderStagesPtr getShaderStagesSingleSource(
            const std::string& shaderId, const std::vector<std::string>& entryPoints,
            const std::map<std::string, std::string>& customPreprocessorDefines, bool dumpTextDebug);
    ShaderStagesPtr getShaderStagesMultiSource(
            const std::vector<std::string>& shaderIds);
    ShaderStagesPtr getShaderStagesMultiSource(
            const std::vector<std::string>& shaderIds, const std::vector<std::string>& entryPoints);
    ShaderStagesPtr getShaderStagesMultiSource(
            const std::vector<std::string>& shaderIds,
            const std::map<std::string, std::string>& customPreprocessorDefines);
    ShaderStagesPtr getShaderStagesMultiSource(
            const std::vector<std::string>& shaderIds, const std::vector<std::string>& entryPoints,
            const std::map<std::string, std::string>& customPreprocessorDefines);
    ShaderStagesPtr getShaderStagesMultiSource(
            const std::vector<std::string>& shaderIds, const std::vector<std::string>& entryPoints,
            const std::map<std::string, std::string>& customPreprocessorDefines, bool dumpTextDebug);

    /**
     * @see After indexFiles was called by the constructor, this function can be used to resolve a shader file path.
     */
    std::string getShaderFileName(const std::string& pureFilename);

    /**
     * Deletes all cached shaders in the ShaderManager. This is necessary, e.g., when wanting to switch to a
     * different rendering technique with "addPreprocessorDefine" after already having loaded a certain shader.
     * Already loaded shaders will stay intact thanks to reference counting.
     */
    virtual void invalidateShaderCache();

    // For use by IncluderInterface.
    [[nodiscard]] const std::map<std::string, std::string>& getShaderFileMap() const;
    [[nodiscard]] const std::string& getShaderPathPrefix() const { return pathPrefix; }

    // For use by device error callback.
    inline void onCompilationFailed(const std::string& messagePtr) { errorMessageExternal = messagePtr; }

private:
    Device* device = nullptr;
    PreprocessorGlsl* preprocessor = nullptr;

    ShaderModulePtr loadAsset(ShaderModuleInfo& shaderModuleInfo) override;

    /**
     * Indexes all ".wgsl" files in the directory pathPrefix (and its sub-directories recursively) to create
     * "shaderFileMap". Therefore, the application can easily include files with relative paths.
     */
    void indexFiles(std::map<std::string, std::string>& shaderFileMap, const std::string& file);

    /// Directory in which to search for shaders (standard: Data/Shaders).
    std::string pathPrefix;

    /// Internal loading for WGSL shaders.
    std::string getShaderStringWgsl(const std::string& globalShaderName);

    std::string errorMessageExternal;
};

DLL_OBJECT extern ShaderManagerWgpu* ShaderManager;

}}

#endif //SGL_WEBGPU_SHADERMANAGER_HPP
