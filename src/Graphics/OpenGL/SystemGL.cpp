/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#include <GL/glew.h>
#include "SystemGL.hpp"
#include <Utils/File/Logfile.hpp>
#include <Utils/Convert.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <list>

namespace sgl {

SystemGL::SystemGL() {
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
    versionString = (const char*)glGetString(GL_VERSION);
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

bool SystemGL::isGLExtensionAvailable(const char *extensionName) {
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

void SystemGL::setPremulAlphaEnabled(bool enabled) {
    premulAlphaEnabled = enabled;
}

}
