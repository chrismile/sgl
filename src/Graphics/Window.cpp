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

#include <cmath>

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#ifdef __linux__
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif
#endif

#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>

namespace sgl {

#ifdef SUPPORT_OPENGL
// Query the numbers of multisample samples possible (given a maximum number of desired samples).
int getMaxSamplesGLImpl(int desiredSamples) {
#ifdef __linux__
    typedef Display* (*PFN_XOpenDisplay)(_Xconst char*);
    typedef GLXFBConfig* (*PFN_glXChooseFBConfig)(Display* dpy, int screen, const int* attribList, int* nitems);
    typedef int (*PFN_glXGetFBConfigAttrib)(Display* dpy, GLXFBConfig config, int attribute, int* value);

    void* libX11so = dlopen("libX11.so", RTLD_NOW | RTLD_LOCAL);
    if (!libX11so) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load libX11.so!");
        return 1;
    }
    void* libGLXso = dlopen("libGLX.so", RTLD_NOW | RTLD_LOCAL);
    if (!libGLXso) {
        libGLXso = dlopen("libGLX.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!libGLXso) {
            sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load libGLX.so!");
            dlclose(libX11so);
            return 1;
        }
    }

    auto dyn_XOpenDisplay = PFN_XOpenDisplay(dlsym(libX11so, "XOpenDisplay"));
    if (!dyn_XOpenDisplay) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load function from libX11.so!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }

    auto dyn_glXChooseFBConfig = PFN_glXChooseFBConfig(dlsym(libGLXso, "glXChooseFBConfig"));
    auto dyn_glXGetFBConfigAttrib = PFN_glXGetFBConfigAttrib(dlsym(libGLXso, "glXGetFBConfigAttrib"));
    if (!dyn_glXChooseFBConfig || !dyn_glXGetFBConfigAttrib) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load functions from libGLX.so!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }


    const char* displayName = ":0";
    Display* display;
    if (!(display = dyn_XOpenDisplay(displayName))) {
        Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Couldn't open X11 display!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }
    int defscreen = DefaultScreen(display);

    int nitems;
    GLXFBConfig* fbcfg = dyn_glXChooseFBConfig(display, defscreen, NULL, &nitems);
    if (!fbcfg) {
        Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Couldn't get FB configs!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glXGetFBConfigAttrib.xml
    int maxSamples = 0;
    for (int i = 0; i < nitems; ++i) {
        int samples = 0;
        dyn_glXGetFBConfigAttrib(display, fbcfg[i], GLX_SAMPLES, &samples);
        if (samples > maxSamples) {
            maxSamples = samples;
        }
    }

    Logfile::get()->writeInfo("Maximum OpenGL multisamples (GLX): " + toString(maxSamples));

    dlclose(libGLXso);
    dlclose(libX11so);

    return std::min(maxSamples, desiredSamples);
#else
    return desiredSamples;
#endif
}
#endif

}