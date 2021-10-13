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

#ifndef GRAPHICS_OPENGL_SHADERMANAGER_HPP_
#define GRAPHICS_OPENGL_SHADERMANAGER_HPP_

#include <Graphics/Shader/ShaderManager.hpp>

namespace sgl {

class DLL_OBJECT ShaderManagerGL : public ShaderManagerInterface
{
public:
    ShaderManagerGL();
    ~ShaderManagerGL() override;

    ShaderProgramPtr createShaderProgram() override;
    ShaderPtr createShader(ShaderType sh) override;
    ShaderAttributesPtr createShaderAttributes(ShaderProgramPtr &shader) override;

    /// Make sure no shader is bound for rendering.
    void unbindShader() override;

    /**
     * Deletes all cached shaders in the ShaderManager. This is necessary e.g. when wanting to switch to a
     * different rendering technique with "addPreprocessorDefine" after already loading a certain shader.
     * Already loaded shaders will stay intact thanks to reference counting.
     */
    void invalidateShaderCache() override
    {
        assetMap.clear();
        effectSources.clear();
    }

    /// Invalidates all uniform, atomic counter and shader storage buffer bindings.
    void invalidateBindings();


    // --- Compute shader interface ---

    /// Array containing maximum work-group count in x,y,z that can be passed to glDispatchCompute.
    virtual const std::vector<int> &getMaxComputeWorkGroupCount();
    /// Array containing maximum local work-group size (defined in shader with layout qualifier).
    virtual const std::vector<int> &getMaxComputeWorkGroupSize();
    /// Maximum number of work group units of a local work group, e.g. 1024 local work items.
    virtual int getMaxWorkGroupInvocations();


    // --- Shader program resources ---

    // Binding points are shared by all shader programs (specified in shader by "layout(binding = x) ...").
    virtual void bindUniformBuffer(int binding, GeometryBufferPtr &geometryBuffer);
    virtual void bindAtomicCounterBuffer(int binding, GeometryBufferPtr &geometryBuffer);
    virtual void bindShaderStorageBuffer(int binding, GeometryBufferPtr &geometryBuffer);


protected:
    ShaderPtr loadAsset(ShaderInfo &shaderInfo);
    ShaderProgramPtr createShaderProgram(const std::vector<std::string> &shaderIDs, bool dumpTextDebug);

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

private:
    std::vector<int> maxComputeWorkGroupCount;
    std::vector<int> maxComputeWorkGroupSize;
    int maxWorkGroupInvocations;

    std::map<int, GeometryBufferPtr> uniformBuffers;
    std::map<int, GeometryBufferPtr> atomicCounterBuffers;
    std::map<int, GeometryBufferPtr> shaderStorageBuffers;
};

}

/*! GRAPHICS_OPENGL_SHADERMANAGER_HPP_ */
#endif
