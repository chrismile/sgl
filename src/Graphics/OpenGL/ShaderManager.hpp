/*!
 * ShaderManager.hpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_SHADERMANAGER_HPP_
#define GRAPHICS_OPENGL_SHADERMANAGER_HPP_

#include <Graphics/Shader/ShaderManager.hpp>

namespace sgl {

class ShaderManagerGL : public ShaderManagerInterface
{
public:
    ShaderManagerGL();
    ~ShaderManagerGL();

    ShaderProgramPtr createShaderProgram();
    ShaderPtr createShader(ShaderType sh);
    ShaderAttributesPtr createShaderAttributes(ShaderProgramPtr &shader);

    /// Make sure no shader is bound for rendering.
    virtual void unbindShader();

    /**
     * Deletes all cached shaders in the ShaderManager. This is necessary e.g. when wanting to switch to a
     * different rendering technique with "addPreprocessorDefine" after already loading a certain shader.
     * Already loaded shaders will stay intact thanks to reference counting.
     */
    virtual void invalidateShaderCache()
    {
        assetMap.clear();
        effectSources.clear();
    }

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
    ShaderProgramPtr createShaderProgram(const std::list<std::string> &shaderIDs, bool dumpTextDebug);

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

    /// Directory in which to search for shaders (standard: Data/Shaders
    std::string pathPrefix;

    /// Maps shader name -> shader source, e.g. "Blur.Fragment" -> "void main() { ... }".
    std::map<std::string, std::string> effectSources;

    /// Maps file names without path to full file paths for "*.glsl" shader files,
    /// e.g. "Blur.glsl" -> "Data/Shaders/PostProcessing/Blur.glsl".
    std::map<std::string, std::string> shaderFileMap;

    // If a file named "GlobalDefines.glsl" is found: Appended to all shaders
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
