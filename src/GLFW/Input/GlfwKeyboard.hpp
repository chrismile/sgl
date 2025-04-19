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

#ifndef SGL_GLFWKEYBOARD_HPP
#define SGL_GLFWKEYBOARD_HPP

#include <string>
#include <map>
#include <unordered_map>

#include <Defs.hpp>
#include <Input/Keyboard.hpp>

namespace sgl {

class DLL_OBJECT GlfwKeyboard : public KeyboardInterface {
public:
    GlfwKeyboard();
    ~GlfwKeyboard() override;
    void update(float dt) override;

    // GLFW callbacks.
    void onKey(int key, int scancode, int action, int mods);
    void onChar(unsigned int codepoint);
    void onCharMods(unsigned int codepoint, int mods);

    /// Keyboard keys
    /**
     * Logical keys (SDLK).
     * This function can take either SDLK or ImGuiKey values.
     * The former is only supported for the SDL backend!
     */
    bool isKeyDown(int button) override;
    bool isKeyUp(int button) override;
    bool keyPressed(int button) override;
    bool keyReleased(int button) override;
    /**
     * Physical keys (SDL_SCANCODE).
     */
    bool isScancodeDown(int button) override;
    bool isScancodeUp(int button) override;
    bool scancodePressed(int button) override;
    bool scancodeReleased(int button) override;
    int getNumKeys() override;
    bool getModifier(ImGuiKey modifier) override;
#if defined(SUPPORT_SDL)
    SDL_Keymod getModifier() override;
#endif

    /**
     * To support non-standard input methods a key buffer is needed.
     * It contains the chars that were typed this frame as UTF-8 chars.
     */
    [[nodiscard]] const char *getKeyBuffer() const override;
    void clearKeyBuffer() override;
    void addToKeyBuffer(const char *str) override;

public:
    int numKeys;
    /// State of the keyboard in the current and the last frame
    uint8_t *keystate, *oldKeystate;
    /// CTRL, SHIFT, etc.
    int modifier;
    std::string utf8KeyBuffer;
    //std::unordered_map<ImGuiKey, int> imGuiToGlfwKeyMap;
    std::unordered_map<int, int> imGuiToGlfwKeyMap;
};

}

#endif //SGL_GLFWKEYBOARD_HPP
