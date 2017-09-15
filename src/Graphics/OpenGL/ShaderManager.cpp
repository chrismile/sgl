/*
 * ShaderManager.cpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph
 */

#include <GL/glew.h>
#include "ShaderManager.hpp"
#include <Utils/File/Logfile.hpp>
#include "Shader.hpp"
#include "SystemGL.hpp"
#include "ShaderAttributes.hpp"
#include "glsw/glsw.h"
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

namespace sgl {

ShaderManagerGL::ShaderManagerGL()
{
	if (!glswInit()) {
		Logfile::get()->writeError("ERROR: ShaderManager::ShaderManager: !glswInit()");
	}
	glswSetPath("./Data/Shaders/", ".glsl");
}

ShaderManagerGL::~ShaderManagerGL()
{
	if (!glswShutdown()) {
		Logfile::get()->writeError("ERROR: ShaderManager::~ShaderManager: !glswShutdown()");
	}
}

ShaderProgramPtr ShaderManagerGL::createShaderProgram(const std::list<std::string> &shaderIDs)
{
	ShaderProgramPtr shaderProgram = createShaderProgram();
	for (const std::string &shaderID : shaderIDs) {
		ShaderPtr shader;
		std::string shaderID_lower = boost::algorithm::to_lower_copy(shaderID);
		if (boost::algorithm::contains(shaderID_lower.c_str(), "vert")) {
			shader = getShader(shaderID.c_str(), VERTEX_SHADER);
		} else if (boost::algorithm::contains(shaderID_lower.c_str(), "frag")) {
			shader = getShader(shaderID.c_str(), FRAGMENT_SHADER);
		} else if (boost::algorithm::contains(shaderID_lower.c_str(), "geom")) {
			shader = getShader(shaderID.c_str(), GEOMETRY_SHADER);
		} else if (boost::algorithm::contains(shaderID_lower.c_str(), "tess")) {
			if (boost::algorithm::contains(shaderID_lower.c_str(), "eval")) {
				shader = getShader(shaderID.c_str(), TESSELATION_EVALUATION_SHADER);
			} else if (boost::algorithm::contains(shaderID_lower.c_str(), "control")) {
				shader = getShader(shaderID.c_str(), TESSELATION_CONTROL_SHADER);
			}
		} else if (boost::algorithm::contains(shaderID_lower.c_str(), "comp")) {
			shader = getShader(shaderID.c_str(), COMPUTE_SHADER);
		} else {
			Logfile::get()->writeError(std::string() + "ERROR: ShaderManagerGL::createShaderProgram: "
					+ "Unknown shader type (id: \"" + shaderID + "\")");
		}
		shaderProgram->attachShader(shader);
	}
	shaderProgram->linkProgram();
	return shaderProgram;
}

ShaderPtr ShaderManagerGL::loadAsset(ShaderInfo &shaderInfo)
{
	std::string id = shaderInfo.filename;
	int gl = 2; // Standard: OpenGL 2
	if (boost::algorithm::ends_with(id.c_str(), ".GL3") || boost::algorithm::ends_with(id.c_str(), ".GL4")) {
		gl = boost::algorithm::ends_with(id.c_str(), ".GL3") ? 3 : 4;
		id.erase(id.size()-4, 4);
	}

	// Load the shader using GLSW
	const char *glswString = glswGetShader(id.c_str());
	if (!glswString) {
		const char *error = glswGetError();
		Logfile::get()->writeError(std::string() + "ERROR: ShaderManager::getShader: glswGetError: " + error);
		return ShaderPtr();
	}

	std::string shaderString;
	if (gl != 2) {
		shaderString = std::string() + "#version 150\n" + glswString;
	} else {
		shaderString = std::string() + "#version 120\n" + glswString;
	}

	ShaderGL *shaderGL = new ShaderGL(shaderInfo.shaderType);
	ShaderPtr shader(shaderGL);
	shaderGL->setShaderText(shaderString);
	shaderGL->setFileID(shaderInfo.filename.c_str());
	shaderGL->compile();
	return shader;
}

ShaderPtr ShaderManagerGL::createShader(ShaderType sh)
{
	ShaderPtr shader(new ShaderGL(sh));
	return shader;
}

ShaderProgramPtr ShaderManagerGL::createShaderProgram()
{
	ShaderProgramPtr shaderProg(new ShaderProgramGL());
	return shaderProg;
}

ShaderAttributesPtr ShaderManagerGL::createShaderAttributes(ShaderProgramPtr &shader)
{
	if (SystemGL::get()->openglVersionMinimum(3,0)) {
		return ShaderAttributesPtr(new ShaderAttributesGL3(shader));
	}
	return ShaderAttributesPtr(new ShaderAttributesGL2(shader));
}

}
