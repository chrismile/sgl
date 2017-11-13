/*!
 * Shader.hpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_SHADER_SHADER_HPP_
#define GRAPHICS_SHADER_SHADER_HPP_

#include <string>
#include <Defs.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/fwd.hpp>

namespace sgl {

enum DLL_OBJECT ShaderType {
	VERTEX_SHADER, FRAGMENT_SHADER, GEOMETRY_SHADER,
	TESSELATION_EVALUATION_SHADER, TESSELATION_CONTROL_SHADER, COMPUTE_SHADER
};

class ShaderAttributes;
typedef boost::shared_ptr<ShaderAttributes> ShaderAttributesPtr;
class Color;
class Texture;
typedef boost::shared_ptr<Texture> TexturePtr;

//! A single shader, e.g. fragment (pixel) shader, vertex shader, geometry shader, ...
class DLL_OBJECT Shader
{
public:
	Shader() {}
	virtual ~Shader() {}
	virtual void setShaderText(const std::string &text)=0;
	virtual bool compile()=0;
};

typedef boost::shared_ptr<Shader> ShaderPtr;

//! The shader program is the sum of the different shaders attached and linked together
class DLL_OBJECT ShaderProgram
{
public:
	ShaderProgram() {}
	virtual ~ShaderProgram() {}

	virtual void attachShader(ShaderPtr shader)=0;
	virtual void detachShader(ShaderPtr shader)=0;
	virtual bool linkProgram()=0;
	virtual bool validateProgram()=0;
	virtual void bind()=0;

	//! Uniform variables are shared between different executions of a shader program
	virtual bool hasUniform(const char *name)=0;
	virtual int getUniformLoc(const char *name)=0;
	virtual bool setUniform(const char *name, int value)=0;
	virtual bool setUniform(const char *name, bool value)=0;
	virtual bool setUniform(const char *name, float value)=0;
	virtual bool setUniform(const char *name, const glm::vec2 &value)=0;
	virtual bool setUniform(const char *name, const glm::vec3 &value)=0;
	virtual bool setUniform(const char *name, const glm::vec4 &value)=0;
	virtual bool setUniform(const char *name, const glm::mat3 &value)=0;
	virtual bool setUniform(const char *name, const glm::mat3x4 &value)=0;
	virtual bool setUniform(const char *name, const glm::mat4 &value)=0;
	virtual bool setUniform(const char *name, TexturePtr &value, int textureUnit = 0)=0;
	virtual bool setUniform(const char *name, const Color &value)=0;
	virtual bool setUniformArray(const char *name, const int *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const bool *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const float *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const glm::vec2 *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const glm::vec3 *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const glm::vec4 *value, size_t num)=0;

	virtual bool setUniform(int location, int value)=0;
	virtual bool setUniform(int location, float value)=0;
	virtual bool setUniform(int location, const glm::vec2 &value)=0;
	virtual bool setUniform(int location, const glm::vec3 &value)=0;
	virtual bool setUniform(int location, const glm::vec4 &value)=0;
	virtual bool setUniform(int location, const glm::mat3 &value)=0;
	virtual bool setUniform(int location, const glm::mat3x4 &value)=0;
	virtual bool setUniform(int location, const glm::mat4 &value)=0;
	virtual bool setUniform(int location, TexturePtr &value, int textureUnit = 0)=0;
	virtual bool setUniform(int location, const Color &value)=0;
	virtual bool setUniformArray(int location, const int *value, size_t num)=0;
	virtual bool setUniformArray(int location, const bool *value, size_t num)=0;
	virtual bool setUniformArray(int location, const float *value, size_t num)=0;
	virtual bool setUniformArray(int location, const glm::vec2 *value, size_t num)=0;
	virtual bool setUniformArray(int location, const glm::vec3 *value, size_t num)=0;
	virtual bool setUniformArray(int location, const glm::vec4 *value, size_t num)=0;
};

typedef boost::shared_ptr<ShaderProgram> ShaderProgramPtr;
typedef boost::weak_ptr<ShaderProgram> WeakShaderProgramPtr;

}

/*! GRAPHICS_SHADER_SHADER_HPP_ */
#endif
