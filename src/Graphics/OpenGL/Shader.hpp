/*!
 * Shader.hpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_SHADER_HPP_
#define GRAPHICS_OPENGL_SHADER_HPP_

#include <Graphics/Shader/Shader.hpp>
#include <GL/glew.h>
#include <list>
#include <map>

namespace sgl {

class DLL_OBJECT ShaderGL : public Shader
{
public:
	ShaderGL(ShaderType _shaderType);
	~ShaderGL();
	void setShaderText(const std::string &text);
	bool compile();

	//! Implementation dependent
	inline GLuint getShaderID() const { return shaderID; }
	inline GLuint getShaderType() const { return shaderType; }
	inline const char *getFileID() const { return fileID.c_str(); }
	inline void setFileID(const char *_fileID) { fileID = _fileID; }
	//! Returns e.g. "Fragment Shader" for logging purposes
	std::string getShaderDebugType();

private:
	GLuint shaderID;
	ShaderType shaderType;
	std::string fileID;
};

typedef boost::shared_ptr<Shader> ShaderPtr;

class DLL_OBJECT ShaderProgramGL : public ShaderProgram
{
public:
	ShaderProgramGL();
	~ShaderProgramGL();

	bool linkProgram();
	void attachShader(ShaderPtr shader);
	void detachShader(ShaderPtr shader);
	void bind();

	bool hasUniform(const char *name);
	int getUniformLoc(const char *name);
	bool setUniform(const char *name, int value);
	bool setUniform(const char *name, float value);
	bool setUniform(const char *name, const glm::vec2 &value);
	bool setUniform(const char *name, const glm::vec3 &value);
	bool setUniform(const char *name, const glm::mat4 &value);
	bool setUniform(const char *name, TexturePtr &value, int textureUnit = 0);
	bool setUniform(const char *name, const Color &value);
	bool setUniform(int location, int value);
	bool setUniform(int location, float value);
	bool setUniform(int location, const glm::vec2 &value);
	bool setUniform(int location, const glm::vec3 &value);
	bool setUniform(int location, const glm::mat4 &value);
	bool setUniform(int location, TexturePtr &value, int textureUnit = 0);
	bool setUniform(int location, const Color &value);

	inline GLuint getShaderProgramID() { return shaderProgramID; }

private:
	//! Prints an error message if the uniform doesn't exist
	int getUniformLoc_error(const char *name);
	std::map<std::string, int> uniforms;
	std::map<std::string, int> attributes;
	GLuint shaderProgramID;
	std::list<ShaderPtr> shaders;
};

}

/*! GRAPHICS_OPENGL_SHADER_HPP_ */
#endif
