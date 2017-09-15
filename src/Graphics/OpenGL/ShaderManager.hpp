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
};

}

/*! GRAPHICS_OPENGL_SHADERMANAGER_HPP_ */
#endif
