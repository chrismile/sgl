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

#include <cstring>

#include <X11/Xlib.h>
#include <GL/glx.h>

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Device.hpp>
#endif

#include "OffscreenContextGLX.hpp"

#include <dlfcn.h>
#include <unistd.h>

namespace sgl {

OffscreenContextGLX::OffscreenContextGLX(OffscreenContextGLXParams params) : params(params) {}

typedef Display* (*PFN_XOpenDisplay)(_Xconst char*);
typedef int (*PFN_XCloseDisplay)(Display*);
typedef GLXFBConfig* (*PFN_glXChooseFBConfig)(Display* dpy, int screen, const int* attribList, int* nitems);
typedef int (*PFN_glXGetFBConfigAttrib)(Display* dpy, GLXFBConfig config, int attribute, int* value);
typedef GLXContext (*PFN_glXCreateNewContext)(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct);
typedef void (*PFN_glXDestroyContext)(Display *dpy, GLXContext ctx);
typedef Bool (*PFN_glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
typedef Bool (*PFN_glXMakeContextCurrent)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
typedef GLXPbuffer (*PFN_glXCreatePbuffer)(Display *dpy, GLXFBConfig config, const int *attribList);
typedef void (*PFN_glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf);

struct OffscreenContextGLXFunctionTable {
    PFN_XOpenDisplay dyn_XOpenDisplay;
    PFN_XCloseDisplay dyn_XCloseDisplay;
    PFN_glXChooseFBConfig glXChooseFBConfig;
    PFN_glXGetFBConfigAttrib glXGetFBConfigAttrib;
    PFN_glXCreateNewContext glXCreateNewContext;
    PFN_glXDestroyContext glXDestroyContext;
    PFN_glXMakeCurrent glXMakeCurrent;
    PFN_glXMakeContextCurrent glXMakeContextCurrent;
    PFN_glXCreatePbuffer glXCreatePbuffer;
    PFN_glXDestroyPbuffer glXDestroyPbuffer;
    PFNGLXGETPROCADDRESSPROC glXGetProcAddress;
};

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

bool OffscreenContextGLX::loadFunctionTable() {
    x11Handle = dlopen("libX11.so", RTLD_NOW | RTLD_LOCAL);
    if (!x11Handle) {
        sgl::Logfile::get()->writeError("OffscreenContextGLX::loadFunctionTable: Could not load libX11.so.", false);
        return false;
    }
    glxHandle = dlopen("libGLX.so", RTLD_NOW | RTLD_LOCAL);
    if (!glxHandle) {
        glxHandle = dlopen("libGLX.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!glxHandle) {
            sgl::Logfile::get()->writeError("OffscreenContextGLX::loadFunctionTable: Could not load libGLX.so.", false);
            return false;
        }
    }

    f->dyn_XOpenDisplay = PFN_XOpenDisplay(dlsym(x11Handle, TOSTRING(XOpenDisplay)));
    f->dyn_XCloseDisplay = PFN_XCloseDisplay(dlsym(x11Handle, TOSTRING(XCloseDisplay)));
    f->glXChooseFBConfig = PFN_glXChooseFBConfig(dlsym(glxHandle, TOSTRING(glXChooseFBConfig)));
    f->glXGetFBConfigAttrib = PFN_glXGetFBConfigAttrib(dlsym(glxHandle, TOSTRING(glXGetFBConfigAttrib)));
    f->glXCreateNewContext = PFN_glXCreateNewContext(dlsym(glxHandle, TOSTRING(glXCreateNewContext)));
    f->glXDestroyContext = PFN_glXDestroyContext(dlsym(glxHandle, TOSTRING(glXDestroyContext)));
    f->glXMakeCurrent = PFN_glXMakeCurrent(dlsym(glxHandle, TOSTRING(glXMakeCurrent)));
    f->glXMakeContextCurrent = PFN_glXMakeContextCurrent(dlsym(glxHandle, TOSTRING(glXMakeContextCurrent)));
    f->glXCreatePbuffer = PFN_glXCreatePbuffer(dlsym(glxHandle, TOSTRING(glXCreatePbuffer)));
    f->glXDestroyPbuffer = PFN_glXDestroyPbuffer(dlsym(glxHandle, TOSTRING(glXDestroyPbuffer)));
    f->glXGetProcAddress = PFNGLXGETPROCADDRESSPROC(dlsym(glxHandle, TOSTRING(glXGetProcAddress)));

    if (!f->dyn_XOpenDisplay
            || !f->dyn_XCloseDisplay
            || !f->glXChooseFBConfig
            || !f->glXGetFBConfigAttrib
            || !f->glXCreateNewContext
            || !f->glXDestroyContext
            || !f->glXMakeCurrent
            || !f->glXMakeContextCurrent
            || !f->glXCreatePbuffer
            || !f->glXDestroyPbuffer
            || !f->glXGetProcAddress) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextGLX::loadFunctionTable: "
                "At least one function pointer could not be loaded.", false);
        return false;
    }

    return true;
}

bool OffscreenContextGLX::initialize() {
    f = new OffscreenContextGLXFunctionTable;
    if (!loadFunctionTable()) {
        return false;
    }

    display = f->dyn_XOpenDisplay(nullptr);
    if (display == nullptr) {
        sgl::Logfile::get()->writeError("Error: XOpenDisplay failed.", false);
        return false;
    }
    int screen = DefaultScreen(display);

    int attribList[] = {
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_MAX_PBUFFER_WIDTH, params.pbufferWidth,
            GLX_MAX_PBUFFER_HEIGHT, params.pbufferHeight,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
            GLX_DEPTH_SIZE, 24,
            None
    };

    int fbconfigCount = 0;
    GLXFBConfig* fbconfig = f->glXChooseFBConfig(
            display, screen, attribList, &fbconfigCount);
    if (fbconfig == nullptr || fbconfigCount <= 0) {
        sgl::Logfile::get()->writeError("Error: glXChooseFBConfig failed.", false);
        return false;
    }

    GLXFBConfig config = fbconfig[0];

    if (params.createPBuffer) {
        const int attributeList[] = { GLX_PBUFFER_WIDTH, params.pbufferWidth, GLX_PBUFFER_HEIGHT, params.pbufferHeight, 0 };
        pbuffer = f->glXCreatePbuffer(display, config, attributeList);
        if (pbuffer == 0) {
            sgl::Logfile::get()->writeError("Error: glXCreatePbuffer failed.", false);
            return false;
        }
    }

    context = f->glXCreateNewContext(display, config, GLX_RGBA_TYPE, nullptr, GL_TRUE);
    if (!context) {
        sgl::Logfile::get()->writeError("Error: glXCreateNewContext failed.", false);
        return false;
    }

    isInitialized = true;
    return true;
}

OffscreenContextGLX::~OffscreenContextGLX() {
    if (context) {
        f->glXDestroyContext(display, context);
        context = {};
    }
    if (pbuffer) {
        f->glXDestroyPbuffer(display, pbuffer);
        pbuffer = {};
    }
    if (display) {
        if (f->dyn_XCloseDisplay(display) != 0) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextGLX::~OffscreenContextGLX: XCloseDisplay failed.", true);
        }
        display = {};
    }

    if (f) {
        delete f;
        f = nullptr;
    }
    if (x11Handle) {
        dlclose(x11Handle);
        x11Handle = {};
    }
    if (glxHandle) {
        dlclose(glxHandle);
        glxHandle = {};
    }
}

void OffscreenContextGLX::makeCurrent() {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError("Error in OffscreenContextGLX::makeCurrent: Context is not initialized.");
    }

    //if (!f->glXMakeContextCurrent(display, pbuffer, pbuffer, context)) {
    if (!f->glXMakeCurrent(display, 0, context)) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextGLX::makeCurrent: glXMakeCurrent failed.", true);
    }
}

void* OffscreenContextGLX::getFunctionPointer(const char* functionName) {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError(
                "Error in OffscreenContextGLX::getFunctionPointer: Context is not initialized.");
    }
    return (void*)f->glXGetProcAddress((const GLubyte*)functionName);
}

}
