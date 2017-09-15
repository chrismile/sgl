/*
 * ShaderManager.cpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph Neuhauser
 */

#include "ShaderManager.hpp"

namespace sgl {

ShaderProgramPtr ShaderManagerInterface::getShaderProgram(const std::list<std::string> &shaderIDs)
{
	return createShaderProgram(shaderIDs);
}

ShaderPtr ShaderManagerInterface::getShader(const char *shaderFilename, ShaderType shaderType)
{
	ShaderInfo info;
	info.filename = shaderFilename;
	info.shaderType = shaderType;
	return FileManager<Shader, ShaderInfo>::getAsset(info);
}

}
