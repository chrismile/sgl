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

	// Internal loading
	std::string loadFileString(const std::string &shaderName);
	std::string getShaderString(const std::string &globalShaderName);

	// Directory in which to search for shaders (standard: Data/Shaders
	std::string pathPrefix;

	// Maps shader name -> shader source
	std::map<std::string, std::string> effectSources;
};

}

/*! GRAPHICS_OPENGL_SHADERMANAGER_HPP_ */
#endif
