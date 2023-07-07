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

#ifndef SGL_OFFSCREENCONTEXTGLX_HPP
#define SGL_OFFSCREENCONTEXTGLX_HPP

#include "OffscreenContext.hpp"

struct __GLXcontextRec;
typedef struct __GLXcontextRec *GLXContext;
typedef unsigned long XID;
typedef XID GLXPbuffer;
struct _XDisplay;
typedef struct _XDisplay Display;

namespace sgl {

namespace vk {
class Device;
}

/*
 * Notes:
 * - As of 2023-07-06, the NVIDIA 535.54.03 driver seems to work both with and without a pbuffer, but Mesa Zink only
 *   works without a pbuffer.
 */
struct DLL_OBJECT OffscreenContextGLXParams {
    bool useDefaultDisplay = true;
    bool createPBuffer = false;
    int pbufferWidth = 32;
    int pbufferHeight = 32;
    sgl::vk::Device* device = nullptr;
};

struct OffscreenContextGLXFunctionTable;

/**
 * Initializes an offscreen context with GLX. GLX is loaded dynamically at runtime.
 */
class DLL_OBJECT OffscreenContextGLX : public OffscreenContext {
public:
    explicit OffscreenContextGLX(OffscreenContextGLXParams params = {});
    bool initialize() override;
    ~OffscreenContextGLX() override;
    void makeCurrent() override;
    void* getFunctionPointer(const char* functionName) override;
    [[nodiscard]] bool getIsInitialized() const override { return isInitialized; }

private:
    bool isInitialized = false;
    void* x11Handle = nullptr;
    void* glxHandle = nullptr;
    OffscreenContextGLXParams params = {};
    Display* display = {};
    GLXContext context = {};
    GLXPbuffer pbuffer = {};

    // Function table.
    bool loadFunctionTable();
    OffscreenContextGLXFunctionTable* f = nullptr;
};

}

#endif //SGL_OFFSCREENCONTEXTGLX_HPP
