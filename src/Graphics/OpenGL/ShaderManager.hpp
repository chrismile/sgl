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

protected:
	ShaderPtr loadAsset(ShaderInfo &shaderInfo);
	ShaderProgramPtr createShaderProgram(const std::list<std::string> &shaderIDs);

	/// Internal loading
	std::string loadFileString(const std::string &shaderName);
	std::string getShaderString(const std::string &globalShaderName);

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
};

}

/*! GRAPHICS_OPENGL_SHADERMANAGER_HPP_ */
#endif
