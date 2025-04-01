/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#ifdef SUPPORT_GLFW
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#else
/*************************************************************************
 * The following API code is taken from GLFW 3.3.
 *------------------------------------------------------------------------
 * GLFW 3.3 - www.glfw.org
 * A library for OpenGL, window and input
 *------------------------------------------------------------------------
 * Copyright (c) 2002-2006 Marcus Geelnard
 * Copyright (c) 2006-2019 Camilla Löwy <elmindreda@glfw.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 *************************************************************************/
#define GLFW_TRUE 1
#define GLFW_FALSE 0
#define GLFW_CLIENT_API 0x00022001
#define GLFW_CONTEXT_VERSION_MAJOR 0x00022002
#define GLFW_CONTEXT_VERSION_MINOR 0x00022003
#define GLFW_NO_API 0
#define GLFW_OPENGL_API 0x00030001
#define GLFW_OPENGL_ES_API 0x00030002
#define GLFW_OPENGL_ANY_PROFILE 0
#define GLFW_OPENGL_CORE_PROFILE 0x00032001
#define GLFW_OPENGL_COMPAT_PROFILE 0x00032002
#define GLFW_OPENGL_DEBUG_CONTEXT 0x00022007
#define GLFW_OPENGL_PROFILE 0x00022008
#define GLFW_VISIBLE 0x00020004
#endif

#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Window.hpp>

#include "OffscreenContextGlfw.hpp"

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define dlsym GetProcAddress
#endif

struct GLFWmonitor;
typedef struct GLFWmonitor GLFWmonitor;
typedef void (*GLFWglproc)(void);
typedef int (*PFNGLFWINITPROC)();
typedef void (*PFNGLFWTERMINATEPROC)();
typedef const char* (*PFNGLFWGETVERSIONSTRINGPROC)();
typedef void (*PFNGLFWWINDOWHINTPROC)(int hint, int value);
typedef GLFWwindow* (*PFNGLFWCREATEWINDOWPROC)(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share);
typedef void (*PFNGLFWDESTROYWINDOWPROC)(GLFWwindow* window);
typedef void (*PFNGLFWMAKECONTEXTCURRENTPROC)(GLFWwindow* window);
typedef GLFWglproc (*PFNGLFWGETPROCADDRESSPROC)(const char* procname);

namespace sgl {

OffscreenContextGlfw::OffscreenContextGlfw(OffscreenContextGlfwParams params) : params(params) {}

struct OffscreenContextGlfwFunctionTable {
    PFNGLFWINITPROC glfwInit;
    PFNGLFWTERMINATEPROC glfwTerminate;
    PFNGLFWGETVERSIONSTRINGPROC glfwGetVersionString;
    PFNGLFWWINDOWHINTPROC glfwWindowHint;
    PFNGLFWCREATEWINDOWPROC glfwCreateWindow;
    PFNGLFWDESTROYWINDOWPROC glfwDestroyWindow;
    PFNGLFWMAKECONTEXTCURRENTPROC glfwMakeContextCurrent;
    PFNGLFWGETPROCADDRESSPROC glfwGetProcAddress;
};

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

bool OffscreenContextGlfw::loadFunctionTable() {
#ifndef SUPPORT_GLFW

#if defined(__linux__)
    glfwHandle = dlopen("libglfw.so.3", RTLD_NOW | RTLD_LOCAL);
    if (!glfwHandle) {
        sgl::Logfile::get()->writeError("OffscreenContextGlfw::initialize: Could not load libglfw.so.3.", false);
        return false;
    }
#elif defined(_WIN32)
    glfwHandle = LoadLibraryA("glfw3.dll");
    if (!glfwHandle) {
        sgl::Logfile::get()->writeError("OffscreenContextGlfw::initialize: Could not load glfw3.dll.", false);
        return false;
    }
#endif

    f->glfwInit = PFNGLFWINITPROC(dlsym(glfwHandle, TOSTRING(glfwInit)));
    f->glfwTerminate = PFNGLFWTERMINATEPROC(dlsym(glfwHandle, TOSTRING(glfwTerminate)));
    f->glfwGetVersionString = PFNGLFWGETVERSIONSTRINGPROC(dlsym(glfwHandle, TOSTRING(glfwGetVersionString)));
    f->glfwWindowHint = PFNGLFWWINDOWHINTPROC(dlsym(glfwHandle, TOSTRING(glfwWindowHint)));
    f->glfwCreateWindow = PFNGLFWCREATEWINDOWPROC(dlsym(glfwHandle, TOSTRING(glfwCreateWindow)));
    f->glfwDestroyWindow = PFNGLFWDESTROYWINDOWPROC(dlsym(glfwHandle, TOSTRING(glfwDestroyWindow)));
    f->glfwMakeContextCurrent = PFNGLFWMAKECONTEXTCURRENTPROC(dlsym(glfwHandle, TOSTRING(glfwMakeContextCurrent)));
    f->glfwGetProcAddress = PFNGLFWGETPROCADDRESSPROC(dlsym(glfwHandle, TOSTRING(glfwGetProcAddress)));

    if (!f->glfwInit
            || !f->glfwTerminate
            || !f->glfwGetVersionString
            || !f->glfwWindowHint
            || !f->glfwCreateWindow
            || !f->glfwDestroyWindow
            || !f->glfwMakeContextCurrent
            || !f->glfwGetProcAddress) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextGlfw::loadFunctionTable: "
                "At least one function pointer could not be loaded.", false);
        return false;
    }

#else

    f->glfwInit = &glfwInit;
    f->glfwTerminate = &glfwTerminate;
    f->glfwGetVersionString = &glfwGetVersionString;
    f->glfwWindowHint = &glfwWindowHint;
    f->glfwCreateWindow = &glfwCreateWindow;
    f->glfwDestroyWindow = &glfwDestroyWindow;
    f->glfwMakeContextCurrent = &glfwMakeContextCurrent;
    f->glfwGetProcAddress = &glfwGetProcAddress;

#endif

    return true;
}

bool OffscreenContextGlfw::initialize() {
    f = new OffscreenContextGlfwFunctionTable;
    if (!loadFunctionTable()) {
        return false;
    }

    auto* window = sgl::AppSettings::get()->getMainWindow();
    isGlfwInitializedExternally = window && window->getBackend() == WindowBackend::GLFW_IMPL;
    if (!isGlfwInitializedExternally) {
        if(!f->glfwInit()){
            sgl::Logfile::get()->writeError("Error in OffscreenContextGlfw::initialize: glfwInit failed.", false);
            return false;
        }
    }
    glfwInitCalled = true;

    const char* glfwVersionString = f->glfwGetVersionString();
    sgl::Logfile::get()->write("GLFW version: " + std::string(glfwVersionString), sgl::BLUE);

    f->glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    f->glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, params.contextVersionMajor);
    f->glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, params.contextVersionMinor);
    f->glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    f->glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, params.useDebugContext ? 1 : 0);
    f->glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindow = f->glfwCreateWindow(params.pbufferWidth, params.pbufferHeight, "OffscreenGLFWWindow", nullptr, nullptr);
    f->glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!glfwWindow) {
        sgl::Logfile::get()->writeError("Error in OffscreenContextGlfw::initialize: glfwCreateWindow failed.", false);
        return false;
    }

    isInitialized = true;
    return true;
}

OffscreenContextGlfw::~OffscreenContextGlfw() {
    if (glfwWindow) {
        f->glfwDestroyWindow(glfwWindow);
        glfwWindow = nullptr;
    }
    if (!isGlfwInitializedExternally) {
        if(glfwInitCalled) {
            f->glfwTerminate();
        }
    }
    if (f) {
        delete f;
        f = nullptr;
    }
#ifndef SUPPORT_GLFW
    if (glfwHandle) {
#if defined(__linux__)
        dlclose(glfwHandle);
#elif defined(_WIN32)
        FreeLibrary(glfwHandle);
#endif
        glfwHandle = {};
    }
#endif
}

void OffscreenContextGlfw::makeCurrent() {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError("Error in OffscreenContextGlfw::makeCurrent: Context is not initialized.");
    }
    f->glfwMakeContextCurrent(glfwWindow);
}

void* OffscreenContextGlfw::getFunctionPointer(const char* functionName) {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError("Error in OffscreenContextGlfw::makeCurrent: Context is not initialized.");
    }
    return (void*)f->glfwGetProcAddress(functionName);
}

}
