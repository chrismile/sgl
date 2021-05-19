/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <unordered_map>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

#include <vulkan/vulkan.h>

#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>

#include "Internal/IncluderInterface.hpp"
#include "ShaderManager.hpp"

namespace sgl { namespace vk {

ShaderManager::ShaderManager(Device* device) : device(device)
{
    shaderCompiler = new shaderc::Compiler;
    pathPrefix = "./Data/Shaders/";
    indexFiles(pathPrefix);

    // Was a file called "GlobalDefines.glsl" found? If yes, store its content in "globalDefines".
    auto it = shaderFileMap.find("GlobalDefines.glsl");
    if (it != shaderFileMap.end()) {
        std::ifstream file(it->second);
        if (!file.is_open()) {
            Logfile::get()->writeError(
                    std::string() + "ShaderManager::ShaderManager: Unexpected error "
                                    "occured while loading \"GlobalDefines.glsl\".");
        }
        globalDefines = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
}

ShaderManager::~ShaderManager()
{
    if (shaderCompiler) {
        delete shaderCompiler;
        shaderCompiler = nullptr;
    }
}


ShaderStagesPtr ShaderManager::getShaderStages(const std::vector<std::string> &shaderIDs, bool dumpTextDebug)
{
    return createShaderStages(shaderIDs, dumpTextDebug);
}

ShaderModulePtr ShaderManager::getShaderModule(const std::string& shaderId, ShaderModuleType shaderType)
{
    ShaderModuleInfo info;
    info.filename = shaderId;
    info.shaderType = shaderType;
    return FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);
}


static bool dumpTextDebugStatic = false;
ShaderStagesPtr ShaderManager::createShaderStages(const std::vector<std::string>& shaderIDs, bool dumpTextDebug)
{
    dumpTextDebugStatic = dumpTextDebug;

    std::vector<ShaderModulePtr> shaderModules;
    for (const std::string &shaderID : shaderIDs) {
        std::string shaderID_lower = boost::algorithm::to_lower_copy(shaderID);
        ShaderModuleType shaderType = ShaderModuleType::VERTEX;
        if (boost::algorithm::ends_with(shaderID_lower.c_str(), "vertex")) {
            shaderType = ShaderModuleType::VERTEX;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "fragment")) {
            shaderType = ShaderModuleType::FRAGMENT;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "geometry")) {
            shaderType = ShaderModuleType::GEOMETRY;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "tesselationevaluation")) {
            shaderType = ShaderModuleType::TESSELATION_EVALUATION;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "tesselationcontrol")) {
            shaderType = ShaderModuleType::TESSELATION_CONTROL;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "compute")) {
            shaderType = ShaderModuleType::COMPUTE;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "raygen")) {
            shaderType = ShaderModuleType::RAYGEN;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "anyhit")) {
            shaderType = ShaderModuleType::ANY_HIT;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "closesthit")) {
            shaderType = ShaderModuleType::CLOSEST_HIT;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "miss")) {
            shaderType = ShaderModuleType::MISS;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "intersection")) {
            shaderType = ShaderModuleType::INTERSECTION;
        } else if (boost::algorithm::ends_with(shaderID_lower.c_str(), "callable")) {
            shaderType = ShaderModuleType::CALLABLE;
        } else {
            if (boost::algorithm::contains(shaderID_lower.c_str(), "vert")) {
                shaderType = ShaderModuleType::VERTEX;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "frag")) {
                shaderType = ShaderModuleType::FRAGMENT;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "geom")) {
                shaderType = ShaderModuleType::GEOMETRY;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "tess")) {
                if (boost::algorithm::contains(shaderID_lower.c_str(), "eval")) {
                    shaderType = ShaderModuleType::TESSELATION_EVALUATION;
                } else if (boost::algorithm::contains(shaderID_lower.c_str(), "control")) {
                    shaderType = ShaderModuleType::TESSELATION_CONTROL;
                }
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "comp")) {
                shaderType = ShaderModuleType::COMPUTE;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "raygen")) {
                shaderType = ShaderModuleType::RAYGEN;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "anyhit")) {
                shaderType = ShaderModuleType::ANY_HIT;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "closesthit")) {
                shaderType = ShaderModuleType::CLOSEST_HIT;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "miss")) {
                shaderType = ShaderModuleType::MISS;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "intersection")) {
                shaderType = ShaderModuleType::INTERSECTION;
            } else if (boost::algorithm::contains(shaderID_lower.c_str(), "callable")) {
                shaderType = ShaderModuleType::CALLABLE;
            } else {
                Logfile::get()->writeError(
                        std::string() + "ERROR: ShaderManager::createShaderProgram: "
                        + "Unknown shader type (id: \"" + shaderID + "\")");
            }
        }
        shaderModules.push_back(getShaderModule(shaderID, shaderType));
    }
    dumpTextDebugStatic = false;

    ShaderStagesPtr shaderProgram(new ShaderStages(shaderModules));
    return shaderProgram;
}

ShaderModulePtr ShaderManager::loadAsset(ShaderModuleInfo &shaderInfo)
{
    std::string id = shaderInfo.filename;
    std::string shaderString = getShaderString(id);

    if (dumpTextDebugStatic) {
        std::cout << "Shader dump (" << id << "):" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << shaderString << std::endl << std::endl;
    }

    shaderc::CompileOptions compileOptions;
    //compileOptions.AddMacroDefinition(a, b);
    IncluderInterface* includerInterface = new IncluderInterface();
    compileOptions.SetIncluder(std::unique_ptr<shaderc::CompileOptions::IncluderInterface>(includerInterface));

    const std::unordered_map<ShaderModuleType, shaderc_shader_kind> shaderKindLookupTable = {
            { ShaderModuleType::VERTEX,                 shaderc_vertex_shader },
            { ShaderModuleType::FRAGMENT,               shaderc_fragment_shader },
            { ShaderModuleType::COMPUTE,                shaderc_compute_shader },
            { ShaderModuleType::GEOMETRY,               shaderc_geometry_shader },
            { ShaderModuleType::TESSELATION_CONTROL,    shaderc_tess_control_shader },
            { ShaderModuleType::TESSELATION_EVALUATION, shaderc_tess_evaluation_shader },
            { ShaderModuleType::RAYGEN,                 shaderc_raygen_shader },
            { ShaderModuleType::ANY_HIT,                shaderc_anyhit_shader },
            { ShaderModuleType::CLOSEST_HIT,            shaderc_closesthit_shader },
            { ShaderModuleType::MISS,                   shaderc_miss_shader },
            { ShaderModuleType::INTERSECTION,           shaderc_intersection_shader },
            { ShaderModuleType::CALLABLE,               shaderc_callable_shader },
            { ShaderModuleType::TASK,                   shaderc_task_shader },
            { ShaderModuleType::MESH,                   shaderc_mesh_shader },
    };
    auto it = shaderKindLookupTable.find(shaderInfo.shaderType);
    if (it == shaderKindLookupTable.end()) {
        sgl::Logfile::get()->writeError("Error in ShaderManager::loadAsset: Invalid shader type.");
        return ShaderModulePtr();
    }
    shaderc_shader_kind shaderKind = it->second;
    shaderc::SpvCompilationResult compilationResult = shaderCompiler->CompileGlslToSpv(
            shaderString.c_str(), shaderString.size(), shaderKind, id.c_str(), compileOptions);

    if (compilationResult.GetNumErrors() != 0 || compilationResult.GetNumWarnings() != 0) {
        sgl::Logfile::get()->writeError(compilationResult.GetErrorMessage());
        if (compilationResult.GetNumErrors() != 0) {
            return ShaderModulePtr();
        }
    }

    std::vector<uint32_t> compilationResultWords(compilationResult.cbegin(), compilationResult.cend());
    ShaderModulePtr shaderModule(new ShaderModule(
            device, shaderInfo.filename, shaderInfo.shaderType, compilationResultWords));
    return shaderModule;
}


std::string ShaderManager::loadHeaderFileString(const std::string &shaderName, std::string &prependContent) {
    std::ifstream file(shaderName.c_str());
    if (!file.is_open()) {
        Logfile::get()->writeError(std::string() + "Error in loadHeaderFileString: Couldn't open the file \""
                                   + shaderName + "\".");
        return "";
    }
    //std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string fileContent = "#line 1\n";

    // Support preprocessor for embedded headers
    std::string linestr;
    int lineNum = 1;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (linestr.size() > 0 && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        lineNum++;

        if (boost::starts_with(linestr, "#include")) {
            std::string includedFileName = getShaderFileName(getHeaderName(linestr));
            std::string includedFileContent = loadHeaderFileString(includedFileName, prependContent);
            fileContent += includedFileContent + "\n";
            fileContent += std::string() + "#line " + toString(lineNum) + "\n";
        } else if (boost::starts_with(linestr, "#extension") || boost::starts_with(linestr, "#version")) {
            prependContent += linestr + "\n";
            fileContent = std::string() + fileContent + "#line " + toString(lineNum) + "\n";
        } else {
            fileContent += std::string() + linestr + "\n";
        }
    }


    file.close();
    fileContent = fileContent;
    return fileContent;
}


std::string ShaderManager::getHeaderName(const std::string &lineString)
{
    // Filename in quotes?
    int startFilename = lineString.find("\"");
    int endFilename = lineString.find_last_of("\"");
    if (startFilename != std::string::npos && endFilename != std::string::npos) {
        return lineString.substr(startFilename+1, endFilename-startFilename-1);
    } else {
        // Filename is user-specified #define directive?
        std::vector<std::string> line;
        boost::algorithm::split(line, lineString, boost::is_any_of("\t "), boost::token_compress_on);
        if (line.size() < 2) {
            Logfile::get()->writeError("Error in ShaderManager::getHeaderFilename: Too few tokens.");
            return "";
        }

        auto it = preprocessorDefines.find(line.at(1));
        if (it != preprocessorDefines.end()) {
            int startFilename = it->second.find("\"");
            int endFilename = it->second.find_last_of("\"");
            return it->second.substr(startFilename+1, endFilename-startFilename-1);
        } else {
            Logfile::get()->writeError("Error in ShaderManager::getHeaderFilename: Invalid include directive.");
            Logfile::get()->writeError(std::string() + "Line string: " + lineString);
            return "";
        }
    }
}





void ShaderManager::indexFiles(const std::string &file) {
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


std::string ShaderManager::getShaderFileName(const std::string &pureFilename)
{
    auto it = shaderFileMap.find(pureFilename);
    if (it == shaderFileMap.end()) {
        sgl::Logfile::get()->writeError("Error in ShaderManager::getShaderFileName: Unknown file name \""
                                        + pureFilename + "\".");
        return "";
    }
    return it->second;
}


std::string ShaderManager::getPreprocessorDefines()
{
    std::string preprocessorStatements;
    for (auto it = preprocessorDefines.begin(); it != preprocessorDefines.end(); it++) {
        preprocessorStatements += std::string() + "#define " + it->first + " " + it->second + "\n";
    }
    return preprocessorStatements + globalDefines;
}


std::string ShaderManager::getShaderString(const std::string &globalShaderName) {
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
    std::string shaderContent = "#line 1\n";
    std::string prependContent = "";
    int lineNum = 1;
    std::string linestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (linestr.size() > 0 && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        lineNum++;

        if (boost::starts_with(linestr, "-- ")) {
            if (shaderContent.size() > 0 && shaderName.size() > 0) {
                shaderContent = prependContent + shaderContent;
                effectSources.insert(make_pair(shaderName, shaderContent));
            }

            shaderName = pureFilename + "." + linestr.substr(3);
            shaderContent = std::string() + getPreprocessorDefines() + "#line " + toString(lineNum) + "\n";
            prependContent = "";
        } else if (boost::starts_with(linestr, "#version") || boost::starts_with(linestr, "#extension")) {
            prependContent += linestr + "\n";
            shaderContent = std::string() + shaderContent + "#line " + toString(lineNum) + "\n";
        } else if (boost::starts_with(linestr, "#include")) {
            std::string includedFileName = getShaderFileName(getHeaderName(linestr));
            std::string includedFileContent = loadHeaderFileString(includedFileName, prependContent);
            shaderContent += includedFileContent + "\n";
            shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
        } else {
            shaderContent += std::string() + linestr + "\n";
        }
    }
    shaderContent = prependContent + shaderContent;
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

}}
