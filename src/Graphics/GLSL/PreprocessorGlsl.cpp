/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#include <Utils/AppSettings.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include "PreprocessorGlsl.hpp"

namespace sgl {

PreprocessorGlsl::PreprocessorGlsl() {
}

void PreprocessorGlsl::resetLoad() {
    sourceStringNumber = 0;
    recursionDepth = 0;
}

void PreprocessorGlsl::loadGlobalDefinesFileIfExists(const std::string& id) {
    auto it = shaderFileMap.find("GlobalDefinesVulkan.glsl");
    if (it != shaderFileMap.end()) {
        std::ifstream file(it->second);
        if (!file.is_open()) {
            Logfile::get()->writeError(
                    "ShaderManagerVk::ShaderManagerVk: Unexpected error occured while loading "
                    "\"GlobalDefinesVulkan.glsl\".");
        }
        setGlobalDefines(
                std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
    }
}

std::string PreprocessorGlsl::getShaderFileName(const std::string &pureFilename) {
    auto it = shaderFileMap.find(pureFilename);
    if (it == shaderFileMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in PreprocessorGlsl::getShaderFileName: Unknown file name \"" + pureFilename + "\".");
        return "";
    }
    return it->second;
}

ShaderModuleTypeGlsl getShaderModuleTypeGlslFromString(const std::string& shaderId) {
    std::string shaderIdLower = sgl::toLowerCopy(shaderId);
    ShaderModuleTypeGlsl shaderModuleType = ShaderModuleTypeGlsl::UNKNOWN;
    if (sgl::endsWith(shaderIdLower, "vertex")) {
        shaderModuleType = ShaderModuleTypeGlsl::VERTEX;
    } else if (sgl::endsWith(shaderIdLower, "fragment")) {
        shaderModuleType = ShaderModuleTypeGlsl::FRAGMENT;
    } else if (sgl::endsWith(shaderIdLower, "geometry")) {
        shaderModuleType = ShaderModuleTypeGlsl::GEOMETRY;
    } else if (sgl::endsWith(shaderIdLower, "tesselationevaluation")) {
        shaderModuleType = ShaderModuleTypeGlsl::TESSELLATION_EVALUATION;
    } else if (sgl::endsWith(shaderIdLower, "tesselationcontrol")) {
        shaderModuleType = ShaderModuleTypeGlsl::TESSELLATION_CONTROL;
    } else if (sgl::endsWith(shaderIdLower, "compute")) {
        shaderModuleType = ShaderModuleTypeGlsl::COMPUTE;
    } else if (sgl::endsWith(shaderIdLower, "raygen")) {
        shaderModuleType = ShaderModuleTypeGlsl::RAYGEN;
    } else if (sgl::endsWith(shaderIdLower, "anyhit")) {
        shaderModuleType = ShaderModuleTypeGlsl::ANY_HIT;
    } else if (sgl::endsWith(shaderIdLower, "closesthit")) {
        shaderModuleType = ShaderModuleTypeGlsl::CLOSEST_HIT;
    } else if (sgl::endsWith(shaderIdLower, "miss")) {
        shaderModuleType = ShaderModuleTypeGlsl::MISS;
    } else if (sgl::endsWith(shaderIdLower, "intersection")) {
        shaderModuleType = ShaderModuleTypeGlsl::INTERSECTION;
    } else if (sgl::endsWith(shaderIdLower, "callable")) {
        shaderModuleType = ShaderModuleTypeGlsl::CALLABLE;
    } else if (sgl::endsWith(shaderIdLower, "tasknv")) {
        shaderModuleType = ShaderModuleTypeGlsl::TASK_NV;
    } else if (sgl::endsWith(shaderIdLower, "meshnv")) {
        shaderModuleType = ShaderModuleTypeGlsl::MESH_NV;
    }
#ifdef VK_EXT_mesh_shader
    else if (sgl::endsWith(shaderIdLower, "taskext")) {
        shaderModuleType = ShaderModuleTypeGlsl::TASK_EXT;
    }
    else if (sgl::endsWith(shaderIdLower, "meshext")) {
        shaderModuleType = ShaderModuleTypeGlsl::MESH_EXT;
    }
#endif
    else {
        if (sgl::stringContains(shaderIdLower, "vert")) {
            shaderModuleType = ShaderModuleTypeGlsl::VERTEX;
        } else if (sgl::stringContains(shaderIdLower, "frag")) {
            shaderModuleType = ShaderModuleTypeGlsl::FRAGMENT;
        } else if (sgl::stringContains(shaderIdLower, "geom")) {
            shaderModuleType = ShaderModuleTypeGlsl::GEOMETRY;
        } else if (sgl::stringContains(shaderIdLower, "tess")) {
            if (sgl::stringContains(shaderIdLower, "eval")) {
                shaderModuleType = ShaderModuleTypeGlsl::TESSELLATION_EVALUATION;
            } else if (sgl::stringContains(shaderIdLower, "control")) {
                shaderModuleType = ShaderModuleTypeGlsl::TESSELLATION_CONTROL;
            }
        } else if (sgl::stringContains(shaderIdLower, "comp")) {
            shaderModuleType = ShaderModuleTypeGlsl::COMPUTE;
        } else if (sgl::stringContains(shaderIdLower, "raygen")) {
            shaderModuleType = ShaderModuleTypeGlsl::RAYGEN;
        } else if (sgl::stringContains(shaderIdLower, "anyhit")) {
            shaderModuleType = ShaderModuleTypeGlsl::ANY_HIT;
        } else if (sgl::stringContains(shaderIdLower, "closesthit")) {
            shaderModuleType = ShaderModuleTypeGlsl::CLOSEST_HIT;
        } else if (sgl::stringContains(shaderIdLower, "miss")) {
            shaderModuleType = ShaderModuleTypeGlsl::MISS;
        } else if (sgl::stringContains(shaderIdLower, "intersection")) {
            shaderModuleType = ShaderModuleTypeGlsl::INTERSECTION;
        } else if (sgl::stringContains(shaderIdLower, "callable")) {
            shaderModuleType = ShaderModuleTypeGlsl::CALLABLE;
        } else if (sgl::stringContains(shaderIdLower, "tasknv")) {
            shaderModuleType = ShaderModuleTypeGlsl::TASK_NV;
        } else if (sgl::stringContains(shaderIdLower, "meshnv")) {
            shaderModuleType = ShaderModuleTypeGlsl::MESH_NV;
        }
#ifdef VK_EXT_mesh_shader
        else if (sgl::stringContains(shaderIdLower, "taskext")) {
            shaderModuleType = ShaderModuleTypeGlsl::TASK_EXT;
        }
        else if (sgl::stringContains(shaderIdLower, "meshext")) {
            shaderModuleType = ShaderModuleTypeGlsl::MESH_EXT;
        }
#endif
        //else {
        //    Logfile::get()->throwError(
        //            std::string() + "ERROR: getShaderModuleTypeGlslFromString: "
        //            + "Unknown shader type (id: \"" + shaderId + "\")");
        //    return ShaderModuleTypeGlsl::UNKNOWN;
        //}
    }
    return shaderModuleType;
}

std::string PreprocessorGlsl::loadHeaderFileString(const std::string& shaderName, std::string& prependContent) {
    return loadHeaderFileString(
            shaderName, sgl::FileUtils::get()->getPureFilename(shaderName), prependContent);
}

std::string PreprocessorGlsl::loadHeaderFileString(
        const std::string& shaderName, const std::string& headerName, std::string &prependContent) {
    std::ifstream file(shaderName.c_str());
    if (!file.is_open()) {
        Logfile::get()->throwError(
                std::string() + "Error in loadHeaderFileString: Couldn't open the file \"" + shaderName + "\".");
        return "";
    }
    //std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    sourceStringNumber++;
    std::string fileContent;
    if (useCppLineStyle) {
        fileContent = "#line 1 \"" + headerName + "\"\n";
    } else if (dumpTextDebugStatic) {
        fileContent = "#line 1 " + std::to_string(sourceStringNumber) + "\n";
    } else {
        fileContent = "#line 1\n";
    }

    // Support preprocessor for embedded headers
    bool hasUsedInclude = false;
    int preprocessorConditionalsDepth = 0;
    int lineNum = 1;
    std::string linestr, trimmedLinestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size() - 1) == '\r') {
            linestr = linestr.substr(0, linestr.size() - 1);
        }

        trimmedLinestr = sgl::stringTrimCopy(linestr);
        lineNum++;

        if (sgl::startsWith(trimmedLinestr, "#include")) {
            std::string headerNameSub = getHeaderName(linestr);
            std::string includedFileName = getShaderFileName(headerNameSub);
            std::string includedFileContent = loadHeaderFileString(includedFileName, headerNameSub, prependContent);
            fileContent += includedFileContent + "\n";
            if (useCppLineStyle) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
            } else if (dumpTextDebugStatic) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#import")) {
            std::string importedShaderModuleContent = getImportedShaderString(
                    getHeaderName(linestr), "", prependContent);
            fileContent += importedShaderModuleContent + "\n";
            if (useCppLineStyle) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
            } else if (dumpTextDebugStatic) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#extension") || sgl::startsWith(trimmedLinestr, "#version")) {
            prependContent += linestr + "\n";
            if (useCppLineStyle) {
                fileContent += std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
            } else if (dumpTextDebugStatic) {
                fileContent += "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += "#line " + toString(lineNum) + "\n";
            }
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#if")) {
            fileContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth++;
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#endif")) {
            fileContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth--;
            /*
              * Tests seem to indicate that #line statements are affected by #if/#ifdef.
              * Consequentially, to be conservative, a #line statement will be inserted after every #endif after an
              * include statement.
              */
            if (hasUsedInclude) {
                if (useCppLineStyle) {
                    fileContent +=
                            std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
                } else if (dumpTextDebugStatic) {
                    fileContent +=
                            std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
                } else {
                    fileContent += std::string() + "#line " + toString(lineNum) + "\n";
                }
            }
            if (preprocessorConditionalsDepth == 0 && hasUsedInclude) {
                hasUsedInclude = false;
            }
        } else {
            fileContent += std::string() + linestr + "\n";
        }
    }

    file.close();
    return fileContent;
}


std::string PreprocessorGlsl::getHeaderName(const std::string &lineString) {
    // Filename in quotes?
    auto startFilename = lineString.find('\"');
    auto endFilename = lineString.find_last_of('\"');
    if (startFilename != std::string::npos && endFilename != std::string::npos) {
        return lineString.substr(startFilename+1, endFilename-startFilename-1);
    } else {
        // Filename is user-specified #define directive?
        std::vector<std::string> line;
        sgl::splitStringWhitespace(lineString, line);
        if (line.size() < 2) {
            Logfile::get()->writeError("Error in PreprocessorGlsl::getHeaderFilename: Too few tokens.");
            return "";
        }

        auto it = preprocessorDefines.find(line.at(1));
        auto itTemp = tempPreprocessorDefines.find(line.at(1));
        if (itTemp != tempPreprocessorDefines.end()) {
            std::string::size_type startFilename = itTemp->second.find('\"');
            std::string::size_type endFilename = itTemp->second.find_last_of('\"');
            return itTemp->second.substr(startFilename+1, endFilename-startFilename-1);
        } else if (it != preprocessorDefines.end()) {
            std::string::size_type startFilename = it->second.find('\"');
            std::string::size_type endFilename = it->second.find_last_of('\"');
            return it->second.substr(startFilename+1, endFilename-startFilename-1);
        } else {
            Logfile::get()->writeError("Error in PreprocessorGlsl::getHeaderFilename: Invalid include directive.");
            Logfile::get()->writeError(std::string() + "Line string: " + lineString);
            return "";
        }
    }
}

std::string PreprocessorGlsl::getPreprocessorDefines(ShaderModuleTypeGlsl shaderModuleType) {
    std::string preprocessorStatements;
    for (auto it = preprocessorDefines.begin(); it != preprocessorDefines.end(); it++) {
        preprocessorStatements += std::string() + "#define " + it->first + " " + it->second + "\n";
    }
    if (shaderModuleType == ShaderModuleTypeGlsl::VERTEX || shaderModuleType == ShaderModuleTypeGlsl::GEOMETRY
            || shaderModuleType == ShaderModuleTypeGlsl::FRAGMENT || shaderModuleType == ShaderModuleTypeGlsl::MESH_NV
            || shaderModuleType == ShaderModuleTypeGlsl::MESH_EXT) {
        preprocessorStatements += globalDefinesMvpMatrices;
    }
    preprocessorStatements += globalDefines;
    return preprocessorStatements;
}

std::string PreprocessorGlsl::getImportedShaderString(
        const std::string& moduleName, const std::string& parentModuleName, std::string& prependContent) {
    recursionDepth++;
    if (recursionDepth > 1) {
        sgl::Logfile::get()->throwError(
                "Error in PreprocessorGlsl::getImportedShaderString: Nested/recursive imports are not supported.");
    }

    if (moduleName.empty()) {
        sgl::Logfile::get()->throwError(
                "Error in PreprocessorGlsl::getImportedShaderString: Empty import statement in module \""
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
        if (sgl::startsWith(itPrepend->second, "#version")) {
            auto newLinePos = std::min(itPrepend->second.find("\n") + 1, itPrepend->second.size());
            prependContent = itPrepend->second.substr(0, newLinePos) + prependContent + itPrepend->second.substr(newLinePos, itPrepend->second.size());
        } else {
            prependContent += itPrepend->second;
        }
    } else {
        sgl::Logfile::get()->throwError(
                "Error in PreprocessorGlsl::getImportedShaderString: The module \"" + absoluteModuleName
                + "\" couldn't be found. Hint: Only modules occurring in a file before the importing module can be "
                  "imported.");
    }

    recursionDepth--;
    return moduleContentString;
}

void PreprocessorGlsl::addExtensions(std::string& prependContent, const std::map<std::string, std::string>& defines) {
    auto extensionsIt = defines.find("__extensions");
    if (extensionsIt != defines.end()) {
        std::vector<std::string> extensions;
        sgl::splitString2(extensionsIt->second, extensions, ';', ',');
        for (const std::string& extension : extensions) {
            prependContent += "#extension " + extension + " : require\n";
        }
    }
}

std::string PreprocessorGlsl::getShaderString(const std::string &globalShaderName) {
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
    bool hasUsedInclude = false;

    std::string shaderContent;
    if (useCppLineStyle) {
        shaderContent = "#line 1 \"" + globalShaderName + "\"\n";
    } else if (dumpTextDebugStatic) {
        shaderContent = "#line 1 " + std::to_string(sourceStringNumber) + "\n";
    } else {
        shaderContent = "#line 1\n";
    }

    std::string extensionsString;
    addExtensions(extensionsString, preprocessorDefines);
    addExtensions(extensionsString, tempPreprocessorDefines);
    std::string shaderName;
    std::string prependContent;
    if (useCppLineStyle) {
        prependContent += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
    }

    int preprocessorConditionalsDepth = 0;
    int lineNum = 1;
    std::string linestr, trimmedLinestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        trimmedLinestr = sgl::stringTrimCopy(linestr);
        lineNum++;

        if (sgl::startsWith(linestr, "-- ")) {
            if (!shaderContent.empty() && !shaderName.empty()) {
                effectSourcesRaw.insert(make_pair(shaderName, shaderContent));
                effectSourcesPrepend.insert(make_pair(shaderName, prependContent));
                shaderContent = prependContent + shaderContent;
                effectSources.insert(make_pair(shaderName, shaderContent));
            }

            sourceStringNumber = oldSourceStringNumber;
            shaderName = pureFilename + "." + linestr.substr(3);
            ShaderModuleTypeGlsl shaderModuleType = getShaderModuleTypeGlslFromString(shaderName);
            if (useCppLineStyle) {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + "\n";
            }
            if (shaderModuleType == ShaderModuleTypeGlsl::FRAGMENT
                    || extensionsString != "#extension GL_ARB_fragment_shader_interlock : require\n") {
                prependContent = extensionsString;
            } else {
                prependContent.clear();
            }
            if (useCppLineStyle) {
                prependContent += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
            }
        } else if (sgl::startsWith(trimmedLinestr, "#version") || sgl::startsWith(trimmedLinestr, "#extension")) {
            if (sgl::startsWith(linestr, "#version")) {
                prependContent = linestr + "\n" + prependContent;
            } else {
                prependContent += linestr + "\n";
            }
            if (useCppLineStyle) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else if (sgl::startsWith(trimmedLinestr, "#include")) {
            std::string headerName = getHeaderName(linestr);
            std::string includedFileName = getShaderFileName(headerName);
            std::string includedFileContent = loadHeaderFileString(includedFileName, headerName, prependContent);
            shaderContent += includedFileContent + "\n";
            if (useCppLineStyle) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#import")) {
            std::string importedShaderModuleContent = getImportedShaderString(
                    getHeaderName(linestr), pureFilename, prependContent);
            shaderContent += importedShaderModuleContent + "\n";
            if (useCppLineStyle) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#codefrag")) {
            const std::string& codeFragmentName = getHeaderName(linestr);
            auto codeFragIt = tempPreprocessorDefines.find(codeFragmentName);
            if (codeFragIt != tempPreprocessorDefines.end()) {
                if (useCppLineStyle) {
                    shaderContent +=
                            std::string() + "#line 1 \"" + codeFragmentName + "\"\n";
                } else {
                    shaderContent += "#line 1\n";
                }
                shaderContent += codeFragIt->second + "\n";
                if (useCppLineStyle) {
                    shaderContent +=
                            std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
                } else {
                    shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
                }
                tempPreprocessorDefines.erase(codeFragmentName);
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#if")) {
            shaderContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth++;
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#endif")) {
            shaderContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth--;
            /*
              * Tests seem to indicate that #line statements are affected by #if/#ifdef.
              * Consequentially, to be conservative, a #line statement will be inserted after every #endif after an
              * include statement.
              */
            if (hasUsedInclude) {
                if (useCppLineStyle) {
                    shaderContent +=
                            std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
                } else if (dumpTextDebugStatic) {
                    shaderContent +=
                            std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
                } else {
                    shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
                }
            }
            if (preprocessorConditionalsDepth == 0 && hasUsedInclude) {
                hasUsedInclude = false;
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

    Logfile::get()->writeError(
            std::string() + "Error in getShader: Couldn't find the shader \"" + globalShaderName + "\".");
    return "";
}

void PreprocessorGlsl::invalidateShaderCache() {
    effectSources.clear();
    effectSourcesRaw.clear();
    effectSourcesPrepend.clear();
}

}
