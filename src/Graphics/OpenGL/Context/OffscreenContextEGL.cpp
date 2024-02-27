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

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Device.hpp>
#endif

#include "OffscreenContextEGL.hpp"

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

namespace sgl {

OffscreenContextEGL::OffscreenContextEGL(OffscreenContextEGLParams params) : params(params) {}

// https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_persistent_id.txt
#ifndef EGL_EXT_device_persistent_id
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYDEVICEBINARYEXTPROC) (EGLDeviceEXT device, EGLint name, EGLint max_size, void *value, EGLint *size);
#define EGL_DEVICE_UUID_EXT 0x335C
#define EGL_DRIVER_UUID_EXT 0x335D
#endif

#ifndef EGL_EXT_device_query_name
#define EGL_RENDERER_EXT 0x335F
#endif

#ifndef EGL_EXT_device_drm
#define EGL_DRM_DEVICE_FILE_EXT 0x3233
#define EGL_DRM_MASTER_FD_EXT 0x333C
#endif

#ifndef EGL_EXT_device_drm_render_node
#define EGL_DRM_RENDER_NODE_FILE_EXT 0x3377
#endif

struct OffscreenContextEGLFunctionTable {
    PFNEGLGETPROCADDRESSPROC eglGetProcAddress;
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
    PFNEGLQUERYDEVICEBINARYEXTPROC eglQueryDeviceBinaryEXT;
};

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

bool OffscreenContextEGL::loadFunctionTable() {
#if defined(__linux__)
    eglHandle = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (!eglHandle) {
        eglHandle = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!eglHandle) {
            sgl::Logfile::get()->writeError("OffscreenContextEGL::initialize: Could not load libEGL.so.", false);
            return false;
        }
    }
#elif defined(_WIN32)
    eglHandle = LoadLibraryA("EGL.dll");
    if (!eglHandle) {
        sgl::Logfile::get()->writeError("OffscreenContextEGL::initialize: Could not load EGL.dll.", false);
        return false;
    }
#endif

    f->eglGetProcAddress = PFNEGLGETPROCADDRESSPROC(dlsym(eglHandle, TOSTRING(eglGetProcAddress)));
    f->eglGetError = PFNEGLGETERRORPROC(dlsym(eglHandle, TOSTRING(eglGetError)));
    f->eglQueryString = PFNEGLQUERYSTRINGPROC(dlsym(eglHandle, TOSTRING(eglQueryString)));
    f->eglGetDisplay = PFNEGLGETDISPLAYPROC(dlsym(eglHandle, TOSTRING(eglGetDisplay)));
    f->eglInitialize = PFNEGLINITIALIZEPROC(dlsym(eglHandle, TOSTRING(eglInitialize)));
    f->eglChooseConfig = PFNEGLCHOOSECONFIGPROC(dlsym(eglHandle, TOSTRING(eglChooseConfig)));
    f->eglCreatePbufferSurface = PFNEGLCREATEPBUFFERSURFACEPROC(dlsym(eglHandle, TOSTRING(eglCreatePbufferSurface)));
    f->eglBindAPI = PFNEGLBINDAPIPROC(dlsym(eglHandle, TOSTRING(eglBindAPI)));
    f->eglCreateContext = PFNEGLCREATECONTEXTPROC(dlsym(eglHandle, TOSTRING(eglCreateContext)));
    f->eglDestroySurface = PFNEGLDESTROYSURFACEPROC(dlsym(eglHandle, TOSTRING(eglDestroySurface)));
    f->eglDestroyContext = PFNEGLDESTROYCONTEXTPROC(dlsym(eglHandle, TOSTRING(eglDestroyContext)));
    f->eglTerminate = PFNEGLTERMINATEPROC(dlsym(eglHandle, TOSTRING(eglTerminate)));
    f->eglMakeCurrent = PFNEGLMAKECURRENTPROC(dlsym(eglHandle, TOSTRING(eglMakeCurrent)));

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
                "Error in OffscreenContextEGL::loadFunctionTable: "
                "At least one function pointer could not be loaded.", false);
        return false;
    }

    // EXT functions are optional.
    f->eglQueryDevicesEXT = PFNEGLQUERYDEVICESEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDevicesEXT)));
    f->eglQueryDeviceStringEXT = PFNEGLQUERYDEVICESTRINGEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDeviceStringEXT)));
    f->eglGetPlatformDisplayEXT = PFNEGLGETPLATFORMDISPLAYEXTPROC(f->eglGetProcAddress(TOSTRING(eglGetPlatformDisplayEXT)));
    f->eglQueryDeviceBinaryEXT = PFNEGLQUERYDEVICEBINARYEXTPROC(f->eglGetProcAddress(TOSTRING(eglQueryDeviceBinaryEXT)));

    return true;
}

bool OffscreenContextEGL::initialize() {
    f = new OffscreenContextEGLFunctionTable;
    if (!loadFunctionTable()) {
        return false;
    }

    const char* extensionsNoDisplay = f->eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    std::string extensionsNoDisplayString(extensionsNoDisplay);
    sgl::Logfile::get()->write(
            "EGL extensions for EGL_NO_DISPLAY: " + extensionsNoDisplayString, BLUE);

    if (!params.device) {
        params.useDefaultDisplay = true;
    }

    // For some reason, Mesa 23.1 does not provide eglQueryDeviceBinaryEXT/EGL_EXT_device_persistent_id.

    if (params.device && (!f->eglQueryDevicesEXT || !f->eglQueryDeviceStringEXT
                          || !f->eglGetPlatformDisplayEXT || (!params.tryUseZinkIfAvailable && !f->eglQueryDeviceBinaryEXT))) {
        params.useDefaultDisplay = true;
        sgl::Logfile::get()->writeWarning(
                "Warning in OffscreenContextEGL::initialize: At least one EGL extension necessary for "
                "device querying is not available.", false);
    }

#ifdef SUPPORT_VULKAN
    if (!params.useDefaultDisplay) {
        EGLint numEglDevices = 0;
        if (!f->eglQueryDevicesEXT(0, nullptr, &numEglDevices)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::initialize: eglQueryDevicesEXT failed.", false);
            return false;
        }
        auto* eglDevices = new EGLDeviceEXT[numEglDevices];
        if (!f->eglQueryDevicesEXT(numEglDevices, eglDevices, &numEglDevices)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::initialize: eglQueryDevicesEXT failed.", false);
            return false;
        }
        if (numEglDevices == 0) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::initialize: eglQueryDevicesEXT returned no device.", false);
            return false;
        }

        // Get the Vulkan UUID data for the driver and device.
        VkPhysicalDeviceIDProperties physicalDeviceIdProperties = {};
        physicalDeviceIdProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {};
        physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physicalDeviceProperties2.pNext = &physicalDeviceIdProperties;
        vkGetPhysicalDeviceProperties2(params.device->getVkPhysicalDevice(), &physicalDeviceProperties2);

        int matchingDeviceIdx = -1;
        for (int i = 0; i < numEglDevices; i++) {
            const char* deviceExtensions = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_EXTENSIONS);
            if (!deviceExtensions) {
                sgl::Logfile::get()->writeError(
                        "Error in OffscreenContextEGL::initialize: eglQueryDeviceStringEXT failed.", false);
                return false;
            }
            std::string deviceExtensionsString(deviceExtensions);
            sgl::Logfile::get()->write("Device #" + std::to_string(i) + " Extensions: " + deviceExtensions, BLUE);
            std::vector<std::string> deviceExtensionsVector;
            sgl::splitStringWhitespace(deviceExtensionsString, deviceExtensionsVector);
            std::set<std::string> deviceExtensionsSet(deviceExtensionsVector.begin(), deviceExtensionsVector.end());

            if (deviceExtensionsSet.find("EGL_EXT_device_query_name") != deviceExtensionsSet.end()) {
                const char* deviceVendor = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_VENDOR);
                const char* deviceRenderer = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_RENDERER_EXT);
                sgl::Logfile::get()->write("Device #" + std::to_string(i) + " Vendor: " + deviceVendor, BLUE);
                sgl::Logfile::get()->write("Device #" + std::to_string(i) + " Renderer: " + deviceRenderer, BLUE);
            }

            if (deviceExtensionsSet.find("EGL_EXT_device_drm") != deviceExtensionsSet.end()) {
                const char* deviceDrmFile = f->eglQueryDeviceStringEXT(eglDevices[i], EGL_DRM_DEVICE_FILE_EXT);
                sgl::Logfile::get()->write("Device #" + std::to_string(i) + " DRM File: " + deviceDrmFile, BLUE);
            }

            if (deviceExtensionsSet.find("EGL_EXT_device_drm_render_node") != deviceExtensionsSet.end()) {
                const char* deviceDrmRenderNodeFile = f->eglQueryDeviceStringEXT(
                        eglDevices[i], EGL_DRM_RENDER_NODE_FILE_EXT);
                if (deviceDrmRenderNodeFile) {
                    sgl::Logfile::get()->write(
                            "Device #" + std::to_string(i) + " DRM Render Node File: " + deviceDrmRenderNodeFile, BLUE);
                }
            }

            if (deviceExtensionsSet.find("EGL_EXT_device_persistent_id") == deviceExtensionsSet.end()) {
                sgl::Logfile::get()->write(
                        "Discarding EGL device #" + std::to_string(i)
                        + " due to not supporting EGL_EXT_device_persistent_id.", BLUE);
                continue;
            }

            const size_t UUID_SIZE = VK_UUID_SIZE;
            uint8_t deviceUuid[VK_UUID_SIZE];
            EGLint uuidSize = 0;
            if (f->eglQueryDeviceBinaryEXT && !f->eglQueryDeviceBinaryEXT(
                    eglDevices[i], EGL_DEVICE_UUID_EXT, EGLint(VK_UUID_SIZE), deviceUuid, &uuidSize)) {
                EGLint errorCode = f->eglGetError();
                sgl::Logfile::get()->writeError(
                        "Error in OffscreenContextEGL::initialize: eglQueryDeviceBinaryEXT failed (error code: "
                        + std::to_string(errorCode) + ").", false);
                return false;
            }
            if (f->eglQueryDeviceBinaryEXT && strncmp(
                    (const char*)deviceUuid, (const char*)physicalDeviceIdProperties.deviceUUID, UUID_SIZE) == 0) {
                matchingDeviceIdx = i;
                //break;
            }
        }

        if (matchingDeviceIdx >= 0) {
            eglDisplay = f->eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevices[matchingDeviceIdx], nullptr);
            if (!eglDisplay) {
                EGLint errorCode = f->eglGetError();
                sgl::Logfile::get()->writeError(
                        "Error in OffscreenContextEGL::initialize: eglGetPlatformDisplayEXT failed (error code: "
                        + std::to_string(errorCode) + ").", false);
                return false;
            }
        } else {
            sgl::Logfile::get()->writeInfo(
                    "OffscreenContextEGL::initialize: Could not find matching device by UUID.");
            params.useDefaultDisplay = true;
        }

        delete[] eglDevices;
    }
#else
    params.useDefaultDisplay = true;
#endif

    /*
     * TODO: The 'offscreen' backend of SDL2 calls eglGetPlatformDisplayEXT for all devices until it finds one where
     * eglInitialize does not fail. It continues its search in two cases (cf. SDL_egl.c):
     * - eglGetPlatformDisplayEXT returns EGL_NO_DISPLAY.
     * - eglInitialize does not return EGL_TRUE. In this case, eglTerminate needs to be called on the display.
     * This might be a good fallback for the case where EGL_EXT_device_persistent_id is not available.
     * For now, 'eglGetDisplay(EGL_DEFAULT_DISPLAY)' will hopefully prove to be a good fallback, but it should be
     * investigated if this could fail for hybrid and multi GPU systems.
     */

    if (params.useDefaultDisplay) {
        eglDisplay = f->eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (!eglDisplay) {
            sgl::Logfile::get()->writeError("Error in OffscreenContextEGL::initialize: eglGetDisplay failed.", false);
            return false;
        }
    }

    EGLint major, minor;
    if (!f->eglInitialize(eglDisplay, &major, &minor)) {
        sgl::Logfile::get()->writeError("Error in OffscreenContextEGL::initialize: eglInitialize failed.", false);
        return false;
    }
    sgl::Logfile::get()->writeInfo(
            "OffscreenContextEGL::initialize: EGL version "
            + std::to_string(major) + "." + std::to_string(minor) + ".");

    const char* displayVendor = f->eglQueryString(eglDisplay, EGL_VENDOR);
    sgl::Logfile::get()->write(std::string() + "EGL Display Vendor: " + displayVendor, BLUE);

    EGLint numConfigs;
    EGLConfig eglConfig;
    bool resultEglChooseConfig;
    if (params.createPBuffer) {
        constexpr EGLint configAttributes[] = {
                EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_DEPTH_SIZE, 8,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                EGL_NONE
        };
        resultEglChooseConfig = f->eglChooseConfig(eglDisplay, configAttributes, &eglConfig, 1, &numConfigs);
    } else {
        constexpr EGLint configAttributes[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                EGL_NONE
        };
        resultEglChooseConfig = f->eglChooseConfig(eglDisplay, configAttributes, &eglConfig, 1, &numConfigs);
    }
    if (numConfigs <= 0) {
        sgl::Logfile::get()->writeError("Error in OffscreenContextEGL::initialize: eglChooseConfig returned 0.", false);
        return false;
    }
    if (!resultEglChooseConfig) {
        sgl::Logfile::get()->writeError("Error in OffscreenContextEGL::initialize: eglChooseConfig failed.", false);
        return false;
    }

    if (params.createPBuffer) {
        static const EGLint pbufferAttributes[] = {
                EGL_WIDTH, params.pbufferWidth,
                EGL_HEIGHT, params.pbufferHeight,
                EGL_NONE,
        };
        eglSurface = f->eglCreatePbufferSurface(eglDisplay, eglConfig, pbufferAttributes);
        if (!eglSurface) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::initialize: eglCreatePbufferSurface failed.", false);
            return false;
        }
    }

    if (!f->eglBindAPI(EGL_OPENGL_API)) {
        sgl::Logfile::get()->writeError("Error in OffscreenContextEGL::initialize: eglBindAPI failed.", false);
        return false;
    }

    //constexpr EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    eglContext = f->eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, nullptr);
    if (!eglContext) {
        EGLint errorCode = f->eglGetError();
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextEGL::initialize: eglCreateContext failed (error code: "
                + std::to_string(errorCode) + ").", false);
        return false;
    }

    isInitialized = true;
    return true;
}

OffscreenContextEGL::~OffscreenContextEGL() {
    if (eglSurface) {
        if (!f->eglDestroySurface(eglDisplay, eglSurface)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::~OffscreenContextEGL: eglDestroySurface failed.", true);
        }
        eglSurface = {};
    }
    if (eglContext) {
        if (!f->eglDestroyContext(eglDisplay, eglContext)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::~OffscreenContextEGL: eglDestroyContext failed.", true);
        }
        eglContext = {};
    }
    if (eglDisplay) {
        if (!f->eglTerminate(eglDisplay)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextEGL::~OffscreenContextEGL: eglTerminate failed.", true);
        }
        eglDisplay = {};
    }
    if (f) {
        delete f;
        f = nullptr;
    }
    if (eglHandle) {
#if defined(__linux__)
        dlclose(eglHandle);
#elif defined(_WIN32)
        FreeLibrary(eglHandle);
#endif
        eglHandle = {};
    }
}

void OffscreenContextEGL::makeCurrent() {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError("Error in OffscreenContextEGL::makeCurrent: Context is not initialized.");
    }

    EGLBoolean retVal;
    if (!eglSurface) {
        retVal = f->eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext);
    } else {
        retVal = f->eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    }
    if (!retVal) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextEGL::makeCurrent: eglMakeCurrent failed.", true);
    }
}

void* OffscreenContextEGL::getFunctionPointer(const char* functionName) {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError(
                "Error in OffscreenContextEGL::getFunctionPointer: Context is not initialized.");
    }
    return (void*)f->eglGetProcAddress(functionName);
}

}
