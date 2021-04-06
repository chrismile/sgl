/*
 * ShaderManager.cpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph Neuhauser
 */

#include "ShaderManager.hpp"

namespace sgl {

ShaderProgramPtr ShaderManagerInterface::getShaderProgram(const std::vector<std::string> &shaderIDs, bool dumpTextDebug)
{
    return createShaderProgram(shaderIDs, dumpTextDebug);
}

ShaderPtr ShaderManagerInterface::getShader(const char *shaderFilename, ShaderType shaderType)
{
    ShaderInfo info;
    info.filename = shaderFilename;
    info.shaderType = shaderType;
    return FileManager<Shader, ShaderInfo>::getAsset(info);
}

}
