/*!
 * ShaderManager.hpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_SHADER_SHADERMANAGER_HPP_
#define GRAPHICS_SHADER_SHADERMANAGER_HPP_

#include <vector>
#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <Utils/Singleton.hpp>
#include <Graphics/Shader/Shader.hpp>
#include <Utils/File/FileManager.hpp>

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
	//! Reference-counted loading
	ShaderProgramPtr getShaderProgram(const std::list<std::string> &shaderIDs);
	ShaderPtr getShader(const char *shaderID, ShaderType shaderType);

	//! Create shader/program (manual loading)
	virtual ShaderPtr createShader(ShaderType sh)=0;
	virtual ShaderProgramPtr createShaderProgram()=0;
	virtual ShaderAttributesPtr createShaderAttributes(ShaderProgramPtr &shader)=0;


	// --- Compute shader interface ---

	/// Array containing maximum work-group count in x,y,z that can be passed to glDispatchCompute.
	virtual const std::vector<int> &getMaxComputeWorkGroupCount()=0;
	/// Array containing maximum local work-group size (defined in shader with layout qualifier).
	virtual const std::vector<int> &getMaxComputeWorkGroupSize()=0;
	/// Maximum number of work group units of a local work group, e.g. 1024 local work items.
	virtual int getMaxWorkGroupInvocations()=0;

protected:
	virtual ShaderPtr loadAsset(ShaderInfo &shaderInfo)=0;
	virtual ShaderProgramPtr createShaderProgram(const std::list<std::string> &shaderIDs)=0;
};

extern ShaderManagerInterface *ShaderManager;

}

/*! GRAPHICS_SHADER_SHADERMANAGER_HPP_ */
#endif
