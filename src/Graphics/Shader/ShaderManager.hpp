/*
 * ShaderManager.hpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_SHADER_SHADERMANAGER_HPP_
#define GRAPHICS_SHADER_SHADERMANAGER_HPP_

#include <Utils/Singleton.hpp>
#include <Graphics/Shader/Shader.hpp>
#include <Utils/File/FileManager.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <list>

namespace sgl {

struct ShaderInfo {
	std::string filename;
	ShaderType shaderType;
	bool operator <(const ShaderInfo &rhs) const {
		return filename < rhs.filename;
	}
};

class DLL_OBJECT ShaderManagerInterface : public FileManager<Shader, ShaderInfo>
{
public:
	// Reference-counted loading
	ShaderProgramPtr getShaderProgram(const std::list<std::string> &shaderIDs);
	ShaderPtr getShader(const char *shaderID, ShaderType shaderType);

	// Create shader/program (manual loading)
	virtual ShaderPtr createShader(ShaderType sh)=0;
	virtual ShaderProgramPtr createShaderProgram()=0;
	virtual ShaderAttributesPtr createShaderAttributes(ShaderProgramPtr &shader)=0;

protected:
	virtual ShaderPtr loadAsset(ShaderInfo &shaderInfo)=0;
	virtual ShaderProgramPtr createShaderProgram(const std::list<std::string> &shaderIDs)=0;
};

extern ShaderManagerInterface *ShaderManager;

}

#endif /* GRAPHICS_SHADER_SHADERMANAGER_HPP_ */
