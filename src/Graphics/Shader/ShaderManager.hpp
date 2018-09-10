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

#include <Utils/Convert.hpp>
#include <Utils/Singleton.hpp>
#include <Utils/File/FileManager.hpp>
#include <Graphics/Shader/Shader.hpp>

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
	/// Reference-counted loading
	ShaderProgramPtr getShaderProgram(const std::list<std::string> &shaderIDs);
	ShaderPtr getShader(const char *shaderID, ShaderType shaderType);

	/// Create shader/program (manual loading)
	virtual ShaderPtr createShader(ShaderType sh)=0;
	virtual ShaderProgramPtr createShaderProgram()=0;
	virtual ShaderAttributesPtr createShaderAttributes(ShaderProgramPtr &shader)=0;

	/**
	 * Used for adding preprocessor defines to all shader files before compiling.
	 * This function is useful for e.g. switching at runtime between different different techniques.
	 * The generated preprocessor statements are of the form "#define <token> <value>".
	 */
    template<typename T>
    void addPreprocessorDefine(const std::string &token, const T &value)
    {
        preprocessorDefines[token] = toString(value);
    }

    /// Removes a preprocessor #define token set by "addPreprocessorDefine"
    void removePreprocessorDefine(const std::string &token) {
        preprocessorDefines.erase(preprocessorDefines.find(token));
    }


	/**
	 * Deletes all cached shaders in the ShaderManager. This is necessary e.g. when wanting to switch to a
	 * different rendering technique with "addPreprocessorDefine" after already loading a certain shader.
	 * Already loaded shaders will stay intact thanks to reference counting.
	 */
    virtual void invalidateShaderCache()=0;


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

	/// A token-value map for user-provided preprocessor #define's
    std::map<std::string, std::string> preprocessorDefines;
};

extern ShaderManagerInterface *ShaderManager;

}

/*! GRAPHICS_SHADER_SHADERMANAGER_HPP_ */
#endif
