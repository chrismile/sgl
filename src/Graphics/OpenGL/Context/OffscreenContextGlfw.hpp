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

#ifndef SGL_OFFSCREENCONTEXTGLFW_HPP
#define SGL_OFFSCREENCONTEXTGLFW_HPP

#include "OffscreenContext.hpp"

struct GLFWwindow;
typedef struct GLFWwindow GLFWwindow;
#if defined(_WIN32) && !defined(_WINDEF_)
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
#endif

namespace sgl {

struct DLL_OBJECT OffscreenContextGlfwParams {
    bool useDebugContext = false;
    int contextVersionMajor = 4;
    int contextVersionMinor = 5;
    int pbufferWidth = 32;
    int pbufferHeight = 32;
};

struct OffscreenContextGlfwFunctionTable;

/**
 * Creates an offscreen context using GLFW. GLFW can be loaded dynamically at runtime if sgl wasn't linked to it.
 * For more details see:
 * - https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/open_gl_interop/offscreen_context.cpp
 */
class DLL_OBJECT OffscreenContextGlfw : public OffscreenContext {
public:
    explicit OffscreenContextGlfw(OffscreenContextGlfwParams params = {});
    bool initialize() override;
    ~OffscreenContextGlfw() override;
    void makeCurrent() override;
    [[nodiscard]] bool getIsInitialized() const override { return isInitialized; }

private:
    bool isInitialized = false;
    bool glfwInitCalled = false;
#ifdef _WIN32
    HMODULE glfwHandle = nullptr;
#else
    void* glfwHandle = nullptr;
#endif
    OffscreenContextGlfwParams params = {};
    GLFWwindow* glfwWindow = nullptr;

    // Function table.
    bool loadFunctionTable();
    OffscreenContextGlfwFunctionTable* f = nullptr;
};

}

#endif //SGL_OFFSCREENCONTEXTGLFW_HPP
