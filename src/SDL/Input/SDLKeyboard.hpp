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

#ifndef SDL_SDLKEYBOARD_HPP_
#define SDL_SDLKEYBOARD_HPP_

#include <string>
#include <map>
#include <unordered_map>

#include <Defs.hpp>
#include <Input/Keyboard.hpp>

namespace sgl {

class DLL_OBJECT SDLKeyboard : public KeyboardInterface {
public:
    SDLKeyboard();
    ~SDLKeyboard() override;
    void update(float dt) override;

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
     * Not supported on backends other than SDL (thus deprecated).
     */
    bool isScancodeDown(int button) override;
    bool isScancodeUp(int button) override;
    bool scancodePressed(int button) override;
    bool scancodeReleased(int button) override;
    int getNumKeys() override;
    bool getModifier(ImGuiKey modifier) override;
    SDL_Keymod getModifier() override;

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
#ifdef SUPPORT_SDL3
    bool *keystate, *oldKeystate;
#else
    Uint8 *keystate, *oldKeystate;
#endif
    /// CTRL, SHIFT, etc.
    SDL_Keymod modifier;
    std::string utf8KeyBuffer;
    //std::unordered_map<ImGuiKey, SDL_KeyCode> imGuiToSDLKMap;
    std::unordered_map<int, int> imGuiToSDLKMap;
};

}

/*! SDL_SDLKEYBOARD_HPP_ */
#endif
