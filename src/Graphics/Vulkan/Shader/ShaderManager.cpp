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

#include "../libs/volk/volk.h"

#include <Utils/Convert.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>

#include <Graphics/Vulkan/Utils/Instance.hpp>
#include "Internal/IncluderInterface.hpp"
#include "ShaderManager.hpp"

namespace sgl { namespace vk {

ShaderManagerVk::ShaderManagerVk(Device* device) : device(device) {
    shaderCompiler = new shaderc::Compiler;
    pathPrefix = sgl::AppSettings::get()->getDataDirectory() + "Shaders/";
    indexFiles(pathPrefix);

    // Was a file called "GlobalDefinesVulkan.glsl" found? If yes, store its content in the variable globalDefines.
    auto it = shaderFileMap.find("GlobalDefinesVulkan.glsl");
    if (it != shaderFileMap.end()) {
        std::ifstream file(it->second);
        if (!file.is_open()) {
            Logfile::get()->writeError(
                    "ShaderManagerVk::ShaderManagerVk: Unexpected error occured while loading "
                    "\"GlobalDefinesVulkan.glsl\".");
        }
        globalDefines = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    globalDefinesMvpMatrices =
            "layout (set = 1, binding = 0) uniform MatrixBlock {\n"
            "    mat4 mMatrix; // Model matrix\n"
            "    mat4 vMatrix; // View matrix\n"
            "    mat4 pMatrix; // Projection matrix\n"
            "    mat4 mvpMatrix; // Model-view-projection matrix\n"
            "};\n\n";
}

ShaderManagerVk::~ShaderManagerVk() {
    if (shaderCompiler) {
        delete shaderCompiler;
        shaderCompiler = nullptr;
    }
}


ShaderStagesPtr ShaderManagerVk::getShaderStages(const std::vector<std::string> &shaderIds, bool dumpTextDebug) {
    return createShaderStages(shaderIds, dumpTextDebug);
}

ShaderStagesPtr ShaderManagerVk::getShaderStages(
        const std::vector<std::string> &shaderIds, const std::map<std::string, std::string>& customPreprocessorDefines,
        bool dumpTextDebug) {
    tempPreprocessorDefines = customPreprocessorDefines;
    ShaderStagesPtr shaderStages = createShaderStages(shaderIds, dumpTextDebug);
    tempPreprocessorDefines.clear();
    return shaderStages;
}

ShaderModulePtr ShaderManagerVk::getShaderModule(const std::string& shaderId, ShaderModuleType shaderModuleType) {
    ShaderModuleInfo info;
    info.filename = shaderId;
    info.shaderModuleType = shaderModuleType;
    return FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);
}

ShaderModulePtr ShaderManagerVk::getShaderModule(
        const std::string& shaderId, ShaderModuleType shaderModuleType,
        const std::map<std::string, std::string>& customPreprocessorDefines) {
    tempPreprocessorDefines = customPreprocessorDefines;

    ShaderModuleInfo info;
    info.filename = shaderId;
    info.shaderModuleType = shaderModuleType;
    ShaderModulePtr shaderModule = FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);

    tempPreprocessorDefines.clear();
    return shaderModule;
}


ShaderModuleType getShaderModuleTypeFromString(const std::string& shaderId) {
    std::string shaderIdLower = boost::algorithm::to_lower_copy(shaderId);
    ShaderModuleType shaderModuleType = ShaderModuleType::VERTEX;
    if (boost::algorithm::ends_with(shaderIdLower.c_str(), "vertex")) {
        shaderModuleType = ShaderModuleType::VERTEX;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "fragment")) {
        shaderModuleType = ShaderModuleType::FRAGMENT;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "geometry")) {
        shaderModuleType = ShaderModuleType::GEOMETRY;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "tesselationevaluation")) {
        shaderModuleType = ShaderModuleType::TESSELATION_EVALUATION;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "tesselationcontrol")) {
        shaderModuleType = ShaderModuleType::TESSELATION_CONTROL;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "compute")) {
        shaderModuleType = ShaderModuleType::COMPUTE;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "raygen")) {
        shaderModuleType = ShaderModuleType::RAYGEN;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "anyhit")) {
        shaderModuleType = ShaderModuleType::ANY_HIT;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "closesthit")) {
        shaderModuleType = ShaderModuleType::CLOSEST_HIT;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "miss")) {
        shaderModuleType = ShaderModuleType::MISS;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "intersection")) {
        shaderModuleType = ShaderModuleType::INTERSECTION;
    } else if (boost::algorithm::ends_with(shaderIdLower.c_str(), "callable")) {
        shaderModuleType = ShaderModuleType::CALLABLE;
    } else {
        if (boost::algorithm::contains(shaderIdLower.c_str(), "vert")) {
            shaderModuleType = ShaderModuleType::VERTEX;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "frag")) {
            shaderModuleType = ShaderModuleType::FRAGMENT;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "geom")) {
            shaderModuleType = ShaderModuleType::GEOMETRY;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "tess")) {
            if (boost::algorithm::contains(shaderIdLower.c_str(), "eval")) {
                shaderModuleType = ShaderModuleType::TESSELATION_EVALUATION;
            } else if (boost::algorithm::contains(shaderIdLower.c_str(), "control")) {
                shaderModuleType = ShaderModuleType::TESSELATION_CONTROL;
            }
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "comp")) {
            shaderModuleType = ShaderModuleType::COMPUTE;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "raygen")) {
            shaderModuleType = ShaderModuleType::RAYGEN;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "anyhit")) {
            shaderModuleType = ShaderModuleType::ANY_HIT;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "closesthit")) {
            shaderModuleType = ShaderModuleType::CLOSEST_HIT;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "miss")) {
            shaderModuleType = ShaderModuleType::MISS;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "intersection")) {
            shaderModuleType = ShaderModuleType::INTERSECTION;
        } else if (boost::algorithm::contains(shaderIdLower.c_str(), "callable")) {
            shaderModuleType = ShaderModuleType::CALLABLE;
        } else {
            Logfile::get()->throwError(
                    std::string() + "ERROR: ShaderManagerVk::createShaderProgram: "
                    + "Unknown shader type (id: \"" + shaderId + "\")");
            return ShaderModuleType(0);
        }
    }
    return shaderModuleType;
}

static bool dumpTextDebugStatic = false;
ShaderStagesPtr ShaderManagerVk::createShaderStages(const std::vector<std::string>& shaderIds, bool dumpTextDebug) {
    dumpTextDebugStatic = dumpTextDebug;

    std::vector<ShaderModulePtr> shaderModules;
    for (const std::string &shaderId : shaderIds) {
        ShaderModuleType shaderModuleType = getShaderModuleTypeFromString(shaderId);
        ShaderModulePtr shaderModule = getShaderModule(shaderId, shaderModuleType);
        if (!shaderModule) {
            return ShaderStagesPtr();
        }
        shaderModules.push_back(shaderModule);
    }
    dumpTextDebugStatic = false;

    ShaderStagesPtr shaderProgram(new ShaderStages(device, shaderModules));
    return shaderProgram;
}

ShaderModulePtr ShaderManagerVk::loadAsset(ShaderModuleInfo& shaderInfo) {
    sourceStringNumber = 0;
    recursionDepth = 0;
    std::string id = shaderInfo.filename;
    std::string shaderString = getShaderString(id);

    if (dumpTextDebugStatic) {
        std::cout << "Shader dump (" << id << "):" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << shaderString << std::endl << std::endl;
    }

    shaderc::CompileOptions compileOptions;
    for (auto& it : preprocessorDefines) {
        compileOptions.AddMacroDefinition(it.first, it.second);
    }
    for (auto& it : tempPreprocessorDefines) {
        compileOptions.AddMacroDefinition(it.first, it.second);
    }
    auto includerInterface = new IncluderInterface();
    compileOptions.SetIncluder(std::unique_ptr<shaderc::CompileOptions::IncluderInterface>(includerInterface));

    if (device->getInstance()->getInstanceVulkanVersion() < VK_API_VERSION_1_1) {
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_0);
    } else if (device->getInstance()->getInstanceVulkanVersion() < VK_MAKE_API_VERSION(0, 1, 2, 0)
            || device->getApiVersion() < VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_3);
    } else {
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_5);
    }

    const std::unordered_map<ShaderModuleType, shaderc_shader_kind> shaderKindLookupTable = {
            { ShaderModuleType::VERTEX,                 shaderc_vertex_shader },
            { ShaderModuleType::FRAGMENT,               shaderc_fragment_shader },
            { ShaderModuleType::COMPUTE,                shaderc_compute_shader },
            { ShaderModuleType::GEOMETRY,               shaderc_geometry_shader },
            { ShaderModuleType::TESSELATION_CONTROL,    shaderc_tess_control_shader },
            { ShaderModuleType::TESSELATION_EVALUATION, shaderc_tess_evaluation_shader },
#if VK_VERSION_1_2 && VK_HEADER_VERSION >= 162
            { ShaderModuleType::RAYGEN,                 shaderc_raygen_shader },
            { ShaderModuleType::ANY_HIT,                shaderc_anyhit_shader },
            { ShaderModuleType::CLOSEST_HIT,            shaderc_closesthit_shader },
            { ShaderModuleType::MISS,                   shaderc_miss_shader },
            { ShaderModuleType::INTERSECTION,           shaderc_intersection_shader },
            { ShaderModuleType::CALLABLE,               shaderc_callable_shader },
            { ShaderModuleType::TASK,                   shaderc_task_shader },
            { ShaderModuleType::MESH,                   shaderc_mesh_shader },
#endif
    };
    auto it = shaderKindLookupTable.find(shaderInfo.shaderModuleType);
    if (it == shaderKindLookupTable.end()) {
        sgl::Logfile::get()->writeError("Error in ShaderManagerVk::loadAsset: Invalid shader type.");
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
            device, shaderInfo.filename, shaderInfo.shaderModuleType, compilationResultWords));
    return shaderModule;
}


std::string ShaderManagerVk::loadHeaderFileString(const std::string &shaderName, std::string &prependContent) {
    std::ifstream file(shaderName.c_str());
    if (!file.is_open()) {
        Logfile::get()->throwError(
                std::string() + "Error in loadHeaderFileString: Couldn't open the file \"" + shaderName + "\".");
        return "";
    }
    //std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    sourceStringNumber++;
    std::string fileContent;
    if (dumpTextDebugStatic) {
        fileContent = "#line 1\n";
    } else {
        fileContent = "#line 1 " + std::to_string(sourceStringNumber) + "\n";
    }

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
            if (dumpTextDebugStatic) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else if (boost::starts_with(linestr, "#import")) {
            std::string importedShaderModuleContent = getImportedShaderString(
                    getHeaderName(linestr), "", prependContent);
            fileContent += importedShaderModuleContent + "\n";
            if (dumpTextDebugStatic) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else if (boost::starts_with(linestr, "#extension") || boost::starts_with(linestr, "#version")) {
            prependContent += linestr + "\n";
            if (dumpTextDebugStatic) {
                fileContent += "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += "#line " + toString(lineNum) + "\n";
            }
        } else {
            fileContent += std::string() + linestr + "\n";
        }
    }


    file.close();
    fileContent = fileContent;
    return fileContent;
}


std::string ShaderManagerVk::getHeaderName(const std::string &lineString) {
    // Filename in quotes?
    auto startFilename = lineString.find('\"');
    auto endFilename = lineString.find_last_of('\"');
    if (startFilename != std::string::npos && endFilename != std::string::npos) {
        return lineString.substr(startFilename+1, endFilename-startFilename-1);
    } else {
        // Filename is user-specified #define directive?
        std::vector<std::string> line;
        boost::algorithm::split(line, lineString, boost::is_any_of("\t "), boost::token_compress_on);
        if (line.size() < 2) {
            Logfile::get()->writeError("Error in ShaderManagerVk::getHeaderFilename: Too few tokens.");
            return "";
        }

        auto it = preprocessorDefines.find(line.at(1));
        if (it != preprocessorDefines.end()) {
            std::string::size_type startFilename = it->second.find('\"');
            std::string::size_type endFilename = it->second.find_last_of('\"');
            return it->second.substr(startFilename+1, endFilename-startFilename-1);
        } else {
            Logfile::get()->writeError("Error in ShaderManagerVk::getHeaderFilename: Invalid include directive.");
            Logfile::get()->writeError(std::string() + "Line string: " + lineString);
            return "";
        }
    }
}





void ShaderManagerVk::indexFiles(const std::string &file) {
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


std::string ShaderManagerVk::getShaderFileName(const std::string &pureFilename) {
    auto it = shaderFileMap.find(pureFilename);
    if (it == shaderFileMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerVk::getShaderFileName: Unknown file name \"" + pureFilename + "\".");
        return "";
    }
    return it->second;
}


std::string ShaderManagerVk::getPreprocessorDefines(ShaderModuleType shaderModuleType) {
    std::string preprocessorStatements;
    for (auto it = preprocessorDefines.begin(); it != preprocessorDefines.end(); it++) {
        preprocessorStatements += std::string() + "#define " + it->first + " " + it->second + "\n";
    }
    if (shaderModuleType == ShaderModuleType::VERTEX || shaderModuleType == ShaderModuleType::GEOMETRY) {
        preprocessorStatements += globalDefinesMvpMatrices;
    }
    preprocessorStatements += globalDefines;
    return preprocessorStatements;
}


std::string ShaderManagerVk::getImportedShaderString(
        const std::string& moduleName, const std::string& parentModuleName, std::string& prependContent) {
    recursionDepth++;
    if (recursionDepth > 1) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerVk::getImportedShaderString: Nested/recursive imports are not supported.");
    }

    if (moduleName.empty()) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerVk::getImportedShaderString: Empty import statement in module \""
                + parentModuleName + "\".");
    }

    std::string absoluteModuleName;
    if (moduleName.front() == '.') {
        // Relative mode.
        absoluteModuleName = parentModuleName + moduleName;
    } else {
        // Absolute mode.
        absoluteModuleName = moduleName;
    }

    std::string::size_type filenameEnd = absoluteModuleName.find('.');
    std::string pureFilename = absoluteModuleName.substr(0, filenameEnd);

    std::string moduleContentString;
    if (pureFilename != parentModuleName) {
        getShaderString(absoluteModuleName);
    }

    // Only allow importing previously defined modules for now.
    auto itRaw = effectSourcesRaw.find(absoluteModuleName);
    auto itPrepend = effectSourcesPrepend.find(absoluteModuleName);
    if (itRaw != effectSourcesRaw.end() || itPrepend != effectSourcesPrepend.end()) {
        moduleContentString = itRaw->second;
        prependContent += itPrepend->second;
    } else {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerVk::getImportedShaderString: The module \"" + absoluteModuleName
                + "\" couldn't be found. Hint: Only modules occurring in a file before the importing module can be "
                  "imported.");
    }

    recursionDepth--;
    return moduleContentString;
}

std::string ShaderManagerVk::getShaderString(const std::string &globalShaderName) {
    auto it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    std::string::size_type filenameEnd = globalShaderName.find('.');
    std::string pureFilename = globalShaderName.substr(0, filenameEnd);
    std::string shaderFilename = getShaderFileName(pureFilename + ".glsl");
    std::string shaderInternalId = globalShaderName.substr(filenameEnd + 1);

    std::ifstream file(shaderFilename.c_str());
    if (!file.is_open()) {
        Logfile::get()->throwError(
                std::string() + "Error in getShader: Couldn't open the file \"" + shaderFilename + "\".");
    }

    int oldSourceStringNumber = sourceStringNumber;

    std::string shaderContent;
    if (dumpTextDebugStatic) {
        shaderContent = "#line 1\n";
    } else {
        shaderContent = "#line 1 " + std::to_string(sourceStringNumber) + "\n";
    }

    std::string shaderName;
    std::string prependContent;
    int lineNum = 1;
    std::string linestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        lineNum++;

        if (boost::starts_with(linestr, "-- ")) {
            if (!shaderContent.empty() && !shaderName.empty()) {
                effectSourcesRaw.insert(make_pair(shaderName, shaderContent));
                effectSourcesPrepend.insert(make_pair(shaderName, prependContent));
                shaderContent = prependContent + shaderContent;
                effectSources.insert(make_pair(shaderName, shaderContent));
            }

            sourceStringNumber = oldSourceStringNumber;
            shaderName = pureFilename + "." + linestr.substr(3);
            ShaderModuleType shaderModuleType = getShaderModuleTypeFromString(shaderName);
            if (dumpTextDebugStatic) {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + "\n";
            }
            prependContent = "";
        } else if (boost::starts_with(linestr, "#version") || boost::starts_with(linestr, "#extension")) {
            prependContent += linestr + "\n";
            if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else if (boost::starts_with(linestr, "#include")) {
            std::string includedFileName = getShaderFileName(getHeaderName(linestr));
            std::string includedFileContent = loadHeaderFileString(includedFileName, prependContent);
            shaderContent += includedFileContent + "\n";
            if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else if (boost::starts_with(linestr, "#import")) {
            std::string importedShaderModuleContent = getImportedShaderString(
                    getHeaderName(linestr), pureFilename, prependContent);
            shaderContent += importedShaderModuleContent + "\n";
            if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else {
            shaderContent += std::string() + linestr + "\n";
        }
    }
    shaderContent = prependContent + shaderContent;
    file.close();

    sourceStringNumber = oldSourceStringNumber;

    if (!shaderName.empty()) {
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

void ShaderManagerVk::invalidateShaderCache() {
    assetMap.clear();
    effectSources.clear();
}

}}
