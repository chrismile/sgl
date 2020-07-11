/*
 * SystemGL.cpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include "SystemGL.hpp"
#include <Utils/File/Logfile.hpp>
#include <Utils/Convert.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <list>

namespace sgl {

SystemGL::SystemGL()
{
    // Save OpenGL extensions in the variable "extensions"
    int n = 0;
    std::string extensionString;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (int i = 0; i < n; i++) {
        std::string extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        extensions.insert(extension);
        extensionString += extension;
        if (i + 1 < n) {
            extensionString += ", ";
        }
    }

    vendorString = (const char*)glGetString(GL_VENDOR);

    // Get OpenGL version (including GLSL)
    versionString = (char*)glGetString(GL_VERSION);
    shadingLanguageVersionString = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    majorVersionNumber = fromString<int>(std::string()+versionString.at(0));
    minorVersionNumber = fromString<int>(std::string()+versionString.at(2));
    majorShadingLanguageVersionNumber =  fromString<int>(std::string()+shadingLanguageVersionString.at(0));

    std::string tempVersionString;
    std::string::const_iterator it = shadingLanguageVersionString.begin(); ++it; ++it;
    while (it != shadingLanguageVersionString.end()) {
        tempVersionString += *it;
        ++it;
    }
    minorShadingLanguageVersionNumber = fromString<int>(tempVersionString);

    // Log information about OpenGL context
    Logfile::get()->write(std::string() + "OpenGL Version: " + (const char*)glGetString(GL_VERSION), BLUE);
    Logfile::get()->write(std::string() + "OpenGL Vendor: " + vendorString, BLUE);
    Logfile::get()->write(std::string() + "OpenGL Renderer: " + (const char*)glGetString(GL_RENDERER), BLUE);
    Logfile::get()->write(std::string() + "OpenGL Shading Language Version: " + (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION), BLUE);
    Logfile::get()->write(std::string() + "OpenGL Extensions: " + extensionString, BLUE);

    // Read out hardware limitations for line size, etc.
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);
    glGetFloatv(GL_LINE_WIDTH_RANGE, glLineSizeRange);
    glGetFloatv(GL_LINE_WIDTH_GRANULARITY, &glLineSizeIncrementStep);
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);

    if (!openglVersionMinimum(3, 1)) {
        Logfile::get()->writeError("FATAL ERROR: The minimum supported OpenGL version is OpenGL 3.1.");
    }

    premulAlphaEnabled = true;
}

bool SystemGL::isGLExtensionAvailable(const char *extensionName)
{
    auto it = extensions.find(extensionName);
    if (it != extensions.end())
        return true;
    return false;
}

// Returns whether the current OpenGL context supports the features of the passed OpenGL version
// You could for example call "openglVersionMinimum(3)" or "openglVersionMinimum(2, 1)"
bool SystemGL::openglVersionMinimum(int major, int minor /* = 0 */) {
    if (majorVersionNumber < major) {
        return false;
    } else if (majorVersionNumber == major && minorVersionNumber < minor) {
        return false;
    }
    return true;
}

void SystemGL::setPremulAlphaEnabled(bool enabled)
{
    premulAlphaEnabled = enabled;
}

}
