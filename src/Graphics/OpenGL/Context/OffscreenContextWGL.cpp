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

// BEGIN D3D12
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("Error: D3D12 called failed.");
    }
}
// END D3D12

typedef BOOL ( WINAPI *PFN_EnumDisplayDevicesA )( LPCSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEA lpDisplayDevice, DWORD dwFlags );

#ifndef WINBOOL
#define WINBOOL BOOL
#endif
typedef HGLRC ( WINAPI *PFN_wglCreateContext )( HDC );
typedef WINBOOL ( WINAPI *PFN_wglDeleteContext )( HGLRC );
typedef WINBOOL ( WINAPI *PFN_wglMakeCurrent )( HDC, HGLRC );
typedef PROC ( WINAPI *PFN_wglGetProcAddress )( LPCSTR );
typedef const char *( WINAPI *PFN_wglGetExtensionsStringARB )( HDC hdc );

//typedef unsigned char GLubyte;
//typedef unsigned int GLenum;
//typedef const GLubyte* ( APIENTRY *PFN_glGetString )( GLenum name );

namespace sgl {

OffscreenContextWGL::OffscreenContextWGL(OffscreenContextWGLParams params) : params(params) {}

struct OffscreenContextWGLFunctionTable {
    PFN_wglCreateContext wglCreateContext;
    PFN_wglDeleteContext wglDeleteContext;
    PFN_wglMakeCurrent wglMakeCurrent;
    PFN_wglGetProcAddress wglGetProcAddress;
    PFN_wglGetExtensionsStringARB wglGetExtensionsStringARB;
};

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

bool OffscreenContextWGL::loadFunctionTable() {
    opengl32Module = LoadLibraryA("opengl32.dll");
    if (!opengl32Module) {
        sgl::Logfile::get()->writeError("OffscreenContextWGL::initialize: Could not load opengl32.dll.", false);
        return false;
    }

    f->wglCreateContext = PFN_wglCreateContext(GetProcAddress(opengl32Module, TOSTRING(wglCreateContext)));
    f->wglDeleteContext = PFN_wglDeleteContext(GetProcAddress(opengl32Module, TOSTRING(wglDeleteContext)));
    f->wglMakeCurrent = PFN_wglMakeCurrent(GetProcAddress(opengl32Module, TOSTRING(wglMakeCurrent)));
    f->wglGetProcAddress = PFN_wglGetProcAddress(GetProcAddress(opengl32Module, TOSTRING(wglGetProcAddress)));
    //f->wglCreateContext = &wglCreateContext;
    //f->wglDeleteContext = &wglDeleteContext;
    //f->wglMakeCurrent = &wglMakeCurrent;
    //f->wglGetProcAddress = &wglGetProcAddress;

    if (!f->wglCreateContext
            || !f->wglDeleteContext
            || !f->wglMakeCurrent
            || !f->wglGetProcAddress) {
        sgl::Logfile::get()->writeError(
                "Error in OffscreenContextWGL::loadFunctionTable: "
                "At least one function pointer could not be loaded.", false);
        return false;
    }
    return true;
}

// --- Code adapted from https://community.khronos.org/t/how-to-use-opengl-with-a-device-chosen-by-you/63017/6 ---
static HWND WINAPI windowFromDeviceContextReplacement(HDC dc) {
    static HWND wnd = nullptr;

    if (dc == nullptr) {
        return nullptr;
    }

    if (wnd == nullptr) {
        WNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "dummy_window_class";
        RegisterClassA(&wc);
        wnd = CreateWindowA(wc.lpszClassName, nullptr, WS_POPUP, 0, 0, 32, 32, nullptr, nullptr, wc.hInstance, nullptr);
    }

    return wnd;
}

static void patchWindowFromDeviceContext(HMODULE user32Module) {
    DWORD old_prot;
    auto wfdc = (unsigned __int64)GetProcAddress(user32Module, "WindowFromDC");

    VirtualProtect((void*)wfdc, 14, PAGE_EXECUTE_WRITECOPY, &old_prot);

    // jmp [eip + 0]
    *(char *)(wfdc + 0) = char(0xFF);
    *(char *)(wfdc + 1) = char(0x25);
    *(unsigned *)(wfdc + 2) = 0x00000000;
    *(unsigned __int64 *)(wfdc + 6) = (unsigned __int64)&windowFromDeviceContextReplacement;
}
// --- END ---

bool OffscreenContextWGL::initialize() {
    user32Module = GetModuleHandleA("user32.dll");
    if (!params.device) {
        params.useDefaultDisplay = true;
    }
    if (!params.useDefaultDisplay) {
        patchWindowFromDeviceContext(user32Module);
    }
    f = new OffscreenContextWGLFunctionTable;

    if (params.useDefaultDisplay) {
        if (!initializeFromWindow()) {
            return false;
        }
    } else {
        if (!initializeFromDeviceContextExperimental()) {
            if (!initializeFromWindow()) {
                return false;
            }
        }
    }

    glrc = f->wglCreateContext(deviceContext);
    if (!glrc) {
        sgl::Logfile::get()->writeError(
                "OffscreenContextWGL::initializeFromDeviceContextExperimental: wglCreateContext failed.");
        return false;
    }
    if (!f->wglMakeCurrent(deviceContext, glrc)) {
        sgl::Logfile::get()->writeError(
                "OffscreenContextWGL::initializeFromDeviceContextExperimental: wglMakeCurrent failed.");
        return false;
    }

    isInitialized = true;

    f->wglGetExtensionsStringARB = PFN_wglGetExtensionsStringARB(getFunctionPointer(TOSTRING(wglGetExtensionsStringARB)));

    std::set<std::string> deviceExtensionsSet;
    std::string deviceExtensionsString;
    if (f->wglGetExtensionsStringARB) {
        const char* deviceExtensions = f->wglGetExtensionsStringARB(deviceContext);
        if (!deviceExtensions) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextWGL::initialize: wglGetExtensionsStringARB failed.", false);
            isInitialized = false;
            return false;
        }
        deviceExtensionsString = deviceExtensions;
        std::vector<std::string> deviceExtensionsVector;
        sgl::splitStringWhitespace(deviceExtensionsString, deviceExtensionsVector);
        deviceExtensionsSet = std::set<std::string>(deviceExtensionsVector.begin(), deviceExtensionsVector.end());
    }

    if (deviceExtensionsSet.find("WGL_NV_gpu_affinity") != deviceExtensionsSet.end()) {
        // TODO
    }
    if (deviceExtensionsSet.find("WGL_AMD_gpu_association") != deviceExtensionsSet.end()) {
        // TODO
    }

    if (f->wglGetExtensionsStringARB) {
        if (params.useDefaultDisplay) {
            sgl::Logfile::get()->write("<br>\n");
        }
        sgl::Logfile::get()->write("Device WGL extensions: " + deviceExtensionsString, sgl::BLUE);
    }

    //auto* pglGetString = PFN_glGetString(getFunctionPointer("glGetString"));
    //auto* pglGetString = &glGetString;
    //MessageBoxA(nullptr, (const char*)pglGetString(GL_VERSION), "OpenGL Version", 0);
    //MessageBoxA(nullptr, (const char*)pglGetString(GL_VENDOR), "OpenGL Vendor", 0);
    //MessageBoxA(nullptr, (const char*)pglGetString(GL_RENDERER), "OpenGL Renderer", 0);

    return true;
}

bool OffscreenContextWGL::initializeFromWindow() {
    const char* windowClassName = "wglwindowclass";
    const char* windowName = "wglwindowname";
    WNDCLASS wc{};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    wc.lpszClassName = windowClassName;
    wc.style = CS_OWNDC;
    if (!RegisterClass(&wc)) {
        sgl::Logfile::get()->writeError("OffscreenContextWGL::initializeFromWindow: RegisterClass failed.");
        return false;
    }
    hWnd = CreateWindowExA(
            0, windowClassName, windowName, WS_OVERLAPPEDWINDOW,
            0, 0, params.virtualWindowWidth, params.virtualWindowHeight,
            nullptr, nullptr, nullptr, nullptr);
    deviceContext = GetDC(hWnd);

    if (!loadFunctionTable()) {
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags =
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE
            | PFD_DEPTH_DONTCARE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pfi = ChoosePixelFormat(deviceContext, &pfd);
    SetPixelFormat(deviceContext, pfi, &pfd);

    return true;
}

#ifdef SUPPORT_VULKAN
std::string OffscreenContextWGL::selectDisplayNameForVulkanDevice() {
    if (!params.device) {
        return "";
    }
    const VkPhysicalDeviceIDProperties& physicalDeviceIdProperties = params.device->getDeviceIDProperties();
    if (!physicalDeviceIdProperties.deviceLUIDValid) {
        return "";
    }

    /*
     * The name of the display adapter associated with the GPU (e.g., \\.\DISPLAY1).
     * On Windows, multiple display adapters may exist for the same GPU.
     * Each display adapter may have multiple display monitors attached (e.g., \\.\DISPLAY1\Monitor0).
     * The name of the display adapter can be used with CreateDCA and the above patching code to create a suitable
     * OpenGL context.
     */
    std::string displayName;

    /*
     * D3D12 allows querying the display adapter name if an output display is connected to the adapter.
     * Thus, we go the route Vulkan LUID -> D3D12 adapter -> display adapter name.
     */
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    for (UINT adapterIdx = 0; dxgiFactory->EnumAdapters1(adapterIdx, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++adapterIdx) {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
        bool isSoftwareRenderer = (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        if (isSoftwareRenderer) {
            continue;
        }
        static_assert(VK_LUID_SIZE == sizeof(LUID));
        if (strncmp(
                (const char*)&dxgiAdapterDesc1.AdapterLuid, (const char*)physicalDeviceIdProperties.deviceLUID,
                VK_LUID_SIZE) != 0) {
            continue;
        }

        /*
         * VendorId (https://gamedev.stackexchange.com/questions/31625/get-video-chipset-manufacturer-in-direct3d):
         * NVIDIA: 0x10DE
         * AMD: 0x1002
         * Intel: 0x8086
         */
        //dxgiAdapterDesc1.VendorId;
        ComPtr<IDXGIOutput> out;
        for (size_t odx = 0; dxgiAdapter1->EnumOutputs((UINT)odx, &out) == S_OK; odx++) {
            DXGI_OUTPUT_DESC desc2;
            out->GetDesc(&desc2);
            displayName = sgl::wideStringArrayToStdString(desc2.DeviceName);
            break;
        }
    }

    if (displayName.empty()) {
        // This physical device (D3D12: adapter) is not associated with a display. Use a name matching heuristic.
        auto* pEnumDisplayDevicesA = PFN_EnumDisplayDevicesA(GetProcAddress(user32Module, "EnumDisplayDevicesA"));
        DWORD adapterIdx = 0;
        DISPLAY_DEVICEA displayDevice{};
        displayDevice.cb = sizeof(DISPLAY_DEVICEA);
        do {
            bool retVal = pEnumDisplayDevicesA(nullptr, adapterIdx, &displayDevice, 0);
            if (!retVal) {
                adapterIdx++;
                break;
            }
            if (strcmp(params.device->getPhysicalDeviceProperties().deviceName, displayDevice.DeviceString) == 0) {
                displayName = displayDevice.DeviceName;
                break;
            }
            adapterIdx++;
        } while (true);
    }

    return displayName;
}
#endif

bool OffscreenContextWGL::initializeFromDeviceContextExperimental() {
    std::string displayName;
#ifdef SUPPORT_VULKAN
    if (params.device) {
        displayName = selectDisplayNameForVulkanDevice();
        if (displayName.empty()) {
            // Could not match Vulkan device to display adapter.
            sgl::Logfile::get()->writeWarning(
                    "Warning in OffscreenContextWGL::initializeFromDeviceContextExperimental: "
                    "Could not match display adapter to Vulkan device.");
            displayName = "\\\\.\\DISPLAY1";
        }
    } else {
        displayName = "\\\\.\\DISPLAY1";
    }
#else
    displayName = "\\\\.\\DISPLAY1";
#endif

    if (!loadFunctionTable()) {
        return false;
    }

    sgl::Logfile::get()->write("<br>\n");
    sgl::Logfile::get()->write("Info: Calling CreateDCA for " + displayName + ".", sgl::BLUE);
    deviceContext = CreateDCA(displayName.c_str(), displayName.c_str(), nullptr, nullptr);
    if (!deviceContext) {
        sgl::Logfile::get()->writeWarning(
                "Warning in OffscreenContextWGL::initializeFromDeviceContextExperimental: CreateDCA failed for "
                + displayName + ".");
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags =
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE
            | PFD_DEPTH_DONTCARE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pfi = ChoosePixelFormat(deviceContext, &pfd);
    SetPixelFormat(deviceContext, pfi, &pfd);

    return true;
}

OffscreenContextWGL::~OffscreenContextWGL() {
    if (glrc) {
        //f->wglMakeCurrent(deviceContext, nullptr); // already done by wglDeleteContext
        if (!f->wglDeleteContext(glrc)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextWGL::~OffscreenContextWGL: wglDeleteContext failed.", true);
        }
        glrc = nullptr;
    }
    if (hWnd) {
        ReleaseDC(hWnd, deviceContext);
        deviceContext = nullptr;
        DestroyWindow(hWnd);
        hWnd = nullptr;
    } else if (deviceContext) {
        if (!DeleteDC(deviceContext)) {
            sgl::Logfile::get()->writeError(
                    "Error in OffscreenContextWGL::~OffscreenContextWGL: DeleteDC failed.", true);
        }
        deviceContext = nullptr;
    }

    if (f) {
        delete f;
        f = nullptr;
    }
    if (opengl32Module) {
        FreeLibrary(opengl32Module);
        opengl32Module = {};
    }
    if (user32Module) {
        FreeLibrary(user32Module);
        user32Module = {};
    }
}

void OffscreenContextWGL::makeCurrent() {
    if (!f->wglMakeCurrent(deviceContext, glrc)) {
        sgl::Logfile::get()->throwError("Error in OffscreenContextWGL::makeCurrent: wglMakeCurrent failed.");
    }
}

void* OffscreenContextWGL::getFunctionPointer(const char* functionName) {
    if (!isInitialized) {
        sgl::Logfile::get()->throwError(
                "Error in OffscreenContextWGL::getFunctionPointer: Context is not initialized.");
    }
    void* p = (void*)f->wglGetProcAddress(functionName);
    if (p == nullptr || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
        p = (void*)GetProcAddress(opengl32Module, functionName);
    }
    return p;
}

}
