/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Device.hpp>
#include "DeviceSelectionWGL.hpp"
#endif

#include "OffscreenContextWGL.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace sgl {

OffscreenContextWGL::OffscreenContextWGL(OffscreenContextWGLParams params) : params(params) {}

struct OffscreenContextWGLFunctionTable {
    /*PFNEGLGETPROCADDRESSPROC eglGetProcAddress;
    PFNEGLGETERRORPROC eglGetError;
    PFNEGLQUERYSTRINGPROC eglQueryString;
    PFNEGLGETDISPLAYPROC eglGetDisplay;
    PFNEGLINITIALIZEPROC eglInitialize;
    PFNEGLCHOOSECONFIGPROC eglChooseConfig;
    PFNEGLCREATEPBUFFERSURFACEPROC eglCreatePbufferSurface;
    PFNEGLBINDAPIPROC eglBindAPI;
    PFNEGLCREATECONTEXTPROC eglCreateContext;
    PFNEGLDESTROYSURFACEPROC eglDestroySurface;
    PFNEGLDESTROYCONTEXTPROC eglDestroyContext;
    PFNEGLTERMINATEPROC eglTerminate;
    PFNEGLMAKECURRENTPROC eglMakeCurrent;

    // EXT functions are optional.
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT;
    PFNEGLQUERYDEVICESTRINGEXTPROC eglQueryDeviceStringEXT;
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
    PFNEGLQUERYDEVICEBINARYEXTPROC eglQueryDeviceBinaryEXT;*/
};

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

bool OffscreenContextWGL::loadFunctionTable() {
    wglHandle = LoadLibraryA("opengl32.dll");
    if (!wglHandle) {
        sgl::Logfile::get()->writeError("OffscreenContextWGL::initialize: Could not load opengl32.dll.", false);
        return false;
    }

    /*f->eglGetProcAddress = PFNEGLGETPROCADDRESSPROC(dlsym(wglHandle, TOSTRING(eglGetProcAddress)));
    f->eglGetError = PFNEGLGETERRORPROC(dlsym(wglHandle, TOSTRING(eglGetError)));
    f->eglQueryString = PFNEGLQUERYSTRINGPROC(dlsym(wglHandle, TOSTRING(eglQueryString)));
    f->eglGetDisplay = PFNEGLGETDISPLAYPROC(dlsym(wglHandle, TOSTRING(eglGetDisplay)));
    f->eglInitialize = PFNEGLINITIALIZEPROC(dlsym(wglHandle, TOSTRING(eglInitialize)));
    f->eglChooseConfig = PFNEGLCHOOSECONFIGPROC(dlsym(wglHandle, TOSTRING(eglChooseConfig)));
    f->eglCreatePbufferSurface = PFNEGLCREATEPBUFFERSURFACEPROC(dlsym(wglHandle, TOSTRING(eglCreatePbufferSurface)));
    f->eglBindAPI = PFNEGLBINDAPIPROC(dlsym(wglHandle, TOSTRING(eglBindAPI)));
    f->eglCreateContext = PFNEGLCREATECONTEXTPROC(dlsym(wglHandle, TOSTRING(eglCreateContext)));
    f->eglDestroySurface = PFNEGLDESTROYSURFACEPROC(dlsym(wglHandle, TOSTRING(eglDestroySurface)));
    f->eglDestroyContext = PFNEGLDESTROYCONTEXTPROC(dlsym(wglHandle, TOSTRING(eglDestroyContext)));
    f->eglTerminate = PFNEGLTERMINATEPROC(dlsym(wglHandle, TOSTRING(eglTerminate)));
    f->eglMakeCurrent = PFNEGLMAKECURRENTPROC(dlsym(wglHandle, TOSTRING(eglMakeCurrent)));

    if (!f->eglGetProcAddress
            || !f->eglGetError
            || !f->eglQueryString
            || !f->eglGetDisplay
            || !f->eglInitialize
            || !f->eglChooseConfig
            || !f->eglCreatePbufferSurface
            || !f->eglBindAPI
            || !f->eglCreateContext
            || !f->eglDestroySurface
            || !f->eglDestroyContext
            || !f->eglTerminate
            || !f->eglMakeCurrent) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextWGL::loadFunctionTable: "
                "At least one function pointer could not be loaded.", false);
        return false;
    }

    // EXT functions are optional.
    f->eglQueryDevicesEXT = PFNEGLQUERYDEVICESEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDevicesEXT)));
    f->eglQueryDeviceStringEXT = PFNEGLQUERYDEVICESTRINGEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDeviceStringEXT)));
    f->eglGetPlatformDisplayEXT = PFNEGLGETPLATFORMDISPLAYEXTPROC(f->eglGetProcAddress(TOSTRING(eglGetPlatformDisplayEXT)));
    f->eglQueryDeviceBinaryEXT = PFNEGLQUERYDEVICEBINARYEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDeviceBinaryEXT)));*/

    return true;
}

bool OffscreenContextWGL::initialize() {
    f = new OffscreenContextWGLFunctionTable;
    if (!loadFunctionTable()) {
        return false;
    }

    if (!params.device) {
        params.useDefaultDisplay = true;
    }

    // TODO
    return false;

    isInitialized = true;
    return true;
}

OffscreenContextWGL::~OffscreenContextWGL() {
    // TODO
    /*if (eglSurface) {
        if (!f->eglDestroySurface(eglDisplay, eglSurface)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextWGL::~OffscreenContextWGL: eglDestroySurface failed.", true);
        }
        eglSurface = {};
    }
    if (eglContext) {
        if (!f->eglDestroyContext(eglDisplay, eglContext)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextWGL::~OffscreenContextWGL: eglDestroyContext failed.", true);
        }
        eglContext = {};
    }
    if (eglDisplay) {
        if (!f->eglTerminate(eglDisplay)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextWGL::~OffscreenContextWGL: eglTerminate failed.", true);
        }
        eglDisplay = {};
    }*/
    if (f) {
        delete f;
        f = nullptr;
    }
    if (wglHandle) {
        FreeLibrary(wglHandle);
        wglHandle = {};
    }
}

void OffscreenContextWGL::makeCurrent() {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError("Error in OffscreenContextWGL::makeCurrent: Context is not initialized.");
    }

    // TODO
    /*EGLBoolean retVal;
    if (!eglSurface) {
        retVal = f->eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext);
    } else {
        retVal = f->eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    }
    if (!retVal) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextWGL::makeCurrent: eglMakeCurrent failed.", true);
    }*/
}

void* OffscreenContextWGL::getFunctionPointer(const char* functionName) {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError(
                "Error in OffscreenContextWGL::getFunctionPointer: Context is not initialized.");
    }
    //return (void*)f->eglGetProcAddress(functionName);
    return nullptr; // TODO
}

}
