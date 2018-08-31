/*
 * ShaderManager.cpp
 *
 *  Created on: 07.02.2015
 *      Author: Christoph Neuhauser
 */

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <streambuf>
#include <sstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

#include <GL/glew.h>
//#include "glsw/glsw.h"

#include "ShaderManager.hpp"
#include <Utils/File/Logfile.hpp>
#include <Utils/Convert.hpp>
#include <Utils/File/FileUtils.hpp>
#include "Shader.hpp"
#include "SystemGL.hpp"
#include "ShaderAttributes.hpp"

namespace sgl {

ShaderManagerGL::ShaderManagerGL()
{
	pathPrefix = "./Data/Shaders/";
	indexFiles(pathPrefix);

	// Query compute shader capabilities
	maxComputeWorkGroupCount.resize(3);
	maxComputeWorkGroupSize.resize(3);
	for (int i = 0; i < 3; i++) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, (&maxComputeWorkGroupCount.front()) + i);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, (&maxComputeWorkGroupSize.front()) + i);
	}
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
}

ShaderManagerGL::~ShaderManagerGL()
{
}

const std::vector<int> &ShaderManagerGL::getMaxComputeWorkGroupCount()
{
	return maxComputeWorkGroupCount;
}

const std::vector<int> &ShaderManagerGL::getMaxComputeWorkGroupSize()
{
	return maxComputeWorkGroupSize;
}

int ShaderManagerGL::getMaxWorkGroupInvocations()
{
	return maxWorkGroupInvocations;
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
	std::string shaderString = getShaderString(id);

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



std::string ShaderManagerGL::loadFileString(const std::string &shaderName) {
	std::ifstream file(shaderName.c_str());
	if (!file.is_open()) {
		Logfile::get()->writeError(std::string() + "Error in loadFileString: Couldn't open the file \""
				+ shaderName + "\".");
	}
	std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return fileContent;
}


void ShaderManagerGL::indexFiles(const std::string &file) {
	if (FileUtils::get()->isDirectory(file)) {
		// Scan content of directory
		std::vector<std::string> elements = FileUtils::get()->getFilesInDirectoryVector(file);
		for (std::string &childFile : elements) {
			indexFiles(childFile);
		}
	} else if (FileUtils::get()->hasExtension(file.c_str(), ".glsl")) {
		// File to index. "fileName" is name without path.
		std::string fileName = FileUtils::get()->getPureFilename(file);
		shaderFileMap.insert(make_pair(fileName, file));
	}
}


std::string ShaderManagerGL::getShaderFileName(const std::string &pureFilename)
{
	auto it = shaderFileMap.find(pureFilename);
	if (it == shaderFileMap.end()) {
		sgl::Logfile::get()->writeError("Error in ShaderManagerGL::getShaderFileName: Unknown file name \""
				+ pureFilename + "\".");
		return "";
	}
	return it->second;
}

std::string ShaderManagerGL::getShaderString(const std::string &globalShaderName) {
	auto it = effectSources.find(globalShaderName);
	if (it != effectSources.end()) {
		return it->second;
	}

	int filenameEnd = globalShaderName.find(".");
	std::string pureFilename = globalShaderName.substr(0, filenameEnd);
	std::string shaderFilename = getShaderFileName(pureFilename + ".glsl");
	std::string shaderInternalID = globalShaderName.substr(filenameEnd+1);

	std::ifstream file(shaderFilename.c_str());
	if (!file.is_open()) {
		Logfile::get()->writeError(std::string() + "Error in getShader: Couldn't open the file \""
				+ shaderFilename + "\".");
	}

	std::string shaderName = "";
	std::string shaderContent = "";
	int lineNum = 2;
	std::string linestr;
	while (getline(file, linestr)) {
		// Remove \r if line ending is \r\n
		if (linestr.size() > 0 && linestr.at(linestr.size()-1) == '\r') {
			linestr = linestr.substr(0, linestr.size()-1);
		}

		if (boost::starts_with(linestr, "-- ")) {
			if (shaderContent.size() > 0) {
				if (shaderName.size() > 0) {
					effectSources.insert(make_pair(shaderName, shaderContent));
				}
			}

			shaderName = pureFilename + "." + linestr.substr(3);
			shaderContent = std::string() + "#line " + toString(lineNum) + "\n";
		} else if (boost::starts_with(linestr, "#version")) {
			shaderContent = std::string() + linestr + "\n" + shaderContent + "\n";
		} else if (boost::starts_with(linestr, "#include")) {
			int startFilename = linestr.find("\"");
			int endFilename = linestr.find_last_of("\"");
			std::string includedFileName = getShaderFileName(
					linestr.substr(startFilename+1, endFilename-startFilename-1));
			std::string includedFileContent = loadFileString(includedFileName);
			shaderContent += includedFileContent + "\n";
			shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
		} else {
			shaderContent += std::string() + linestr + "\n";
		}

		lineNum++;
	}
	file.close();

	if (shaderName.size() > 0) {
		effectSources.insert(make_pair(shaderName, shaderContent));
	} else {
		effectSources.insert(make_pair(pureFilename + ".glsl", shaderContent));
	}

	it = effectSources.find(globalShaderName);
	if (it != effectSources.end()) {
		return it->second;
	}

	Logfile::get()->writeError(std::string() + "Error in getShader: Couldn't find the shader \""
			+ globalShaderName + "\".");
	return "";
}

}
