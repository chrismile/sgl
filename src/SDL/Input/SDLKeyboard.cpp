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

#include "SDLKeyboard.hpp"
#include <SDL2/SDL.h>

namespace sgl {

SDLKeyboard::SDLKeyboard() {
    // Get the number of keys
    SDL_GetKeyboardState(&numKeys);

    // Allocate and initialize the memory for the keystates
    oldKeystate = new Uint8[numKeys];
    keystate = new Uint8[numKeys];
    memset((void*)oldKeystate, 0, numKeys);
    memset((void*)keystate, 0, numKeys);
    modifier = KMOD_NONE;

    // For new v2 input API. Values obtained from ImGui_ImplSDL2_KeycodeToImGuiKey.
    imGuiToSDLKMap = std::unordered_map<int, int>{
            std::make_pair(ImGuiKey_Tab, SDLK_TAB),
            std::make_pair(ImGuiKey_LeftArrow, SDLK_LEFT),
            std::make_pair(ImGuiKey_RightArrow, SDLK_RIGHT),
            std::make_pair(ImGuiKey_UpArrow, SDLK_UP),
            std::make_pair(ImGuiKey_DownArrow, SDLK_DOWN),
            std::make_pair(ImGuiKey_PageUp, SDLK_PAGEUP),
            std::make_pair(ImGuiKey_PageDown, SDLK_PAGEDOWN),
            std::make_pair(ImGuiKey_Home, SDLK_HOME),
            std::make_pair(ImGuiKey_End, SDLK_END),
            std::make_pair(ImGuiKey_Insert, SDLK_INSERT),
            std::make_pair(ImGuiKey_Delete, SDLK_DELETE),
            std::make_pair(ImGuiKey_Backspace, SDLK_BACKSPACE),
            std::make_pair(ImGuiKey_Space, SDLK_SPACE),
            std::make_pair(ImGuiKey_Enter, SDLK_RETURN),
            std::make_pair(ImGuiKey_Escape, SDLK_ESCAPE),
            std::make_pair(ImGuiKey_Apostrophe, SDLK_QUOTE),
            std::make_pair(ImGuiKey_Comma, SDLK_COMMA),
            std::make_pair(ImGuiKey_Minus, SDLK_MINUS),
            std::make_pair(ImGuiKey_Period, SDLK_PERIOD),
            std::make_pair(ImGuiKey_Slash, SDLK_SLASH),
            std::make_pair(ImGuiKey_Semicolon, SDLK_SEMICOLON),
            std::make_pair(ImGuiKey_Equal, SDLK_EQUALS),
            std::make_pair(ImGuiKey_LeftBracket, SDLK_LEFTBRACKET),
            std::make_pair(ImGuiKey_Backslash, SDLK_BACKSLASH),
            std::make_pair(ImGuiKey_RightBracket, SDLK_RIGHTBRACKET),
            std::make_pair(ImGuiKey_GraveAccent, SDLK_BACKQUOTE),
            std::make_pair(ImGuiKey_CapsLock, SDLK_CAPSLOCK),
            std::make_pair(ImGuiKey_ScrollLock, SDLK_SCROLLLOCK),
            std::make_pair(ImGuiKey_NumLock, SDLK_NUMLOCKCLEAR),
            std::make_pair(ImGuiKey_PrintScreen, SDLK_PRINTSCREEN),
            std::make_pair(ImGuiKey_Pause, SDLK_PAUSE),
            std::make_pair(ImGuiKey_Keypad0, SDLK_KP_0),
            std::make_pair(ImGuiKey_Keypad1, SDLK_KP_1),
            std::make_pair(ImGuiKey_Keypad2, SDLK_KP_2),
            std::make_pair(ImGuiKey_Keypad3, SDLK_KP_3),
            std::make_pair(ImGuiKey_Keypad4, SDLK_KP_4),
            std::make_pair(ImGuiKey_Keypad5, SDLK_KP_5),
            std::make_pair(ImGuiKey_Keypad6, SDLK_KP_6),
            std::make_pair(ImGuiKey_Keypad7, SDLK_KP_7),
            std::make_pair(ImGuiKey_Keypad8, SDLK_KP_8),
            std::make_pair(ImGuiKey_Keypad9, SDLK_KP_9),
            std::make_pair(ImGuiKey_KeypadDecimal, SDLK_KP_PERIOD),
            std::make_pair(ImGuiKey_KeypadDivide, SDLK_KP_DIVIDE),
            std::make_pair(ImGuiKey_KeypadMultiply, SDLK_KP_MULTIPLY),
            std::make_pair(ImGuiKey_KeypadSubtract, SDLK_KP_MINUS),
            std::make_pair(ImGuiKey_KeypadAdd, SDLK_KP_PLUS),
            std::make_pair(ImGuiKey_KeypadEnter, SDLK_KP_ENTER),
            std::make_pair(ImGuiKey_KeypadEqual, SDLK_KP_EQUALS),
            std::make_pair(ImGuiKey_LeftCtrl, SDLK_LCTRL),
            std::make_pair(ImGuiKey_LeftShift, SDLK_LSHIFT),
            std::make_pair(ImGuiKey_LeftAlt, SDLK_LALT),
            std::make_pair(ImGuiKey_LeftSuper, SDLK_LGUI),
            std::make_pair(ImGuiKey_RightCtrl, SDLK_RCTRL),
            std::make_pair(ImGuiKey_RightShift, SDLK_RSHIFT),
            std::make_pair(ImGuiKey_RightAlt, SDLK_RALT),
            std::make_pair(ImGuiKey_RightSuper, SDLK_RGUI),
            std::make_pair(ImGuiKey_Menu, SDLK_APPLICATION),
            std::make_pair(ImGuiKey_0, SDLK_0),
            std::make_pair(ImGuiKey_1, SDLK_1),
            std::make_pair(ImGuiKey_2, SDLK_2),
            std::make_pair(ImGuiKey_3, SDLK_3),
            std::make_pair(ImGuiKey_4, SDLK_4),
            std::make_pair(ImGuiKey_5, SDLK_5),
            std::make_pair(ImGuiKey_6, SDLK_6),
            std::make_pair(ImGuiKey_7, SDLK_7),
            std::make_pair(ImGuiKey_8, SDLK_8),
            std::make_pair(ImGuiKey_9, SDLK_9),
            std::make_pair(ImGuiKey_A, SDLK_a),
            std::make_pair(ImGuiKey_B, SDLK_b),
            std::make_pair(ImGuiKey_C, SDLK_c),
            std::make_pair(ImGuiKey_D, SDLK_d),
            std::make_pair(ImGuiKey_E, SDLK_e),
            std::make_pair(ImGuiKey_F, SDLK_f),
            std::make_pair(ImGuiKey_G, SDLK_g),
            std::make_pair(ImGuiKey_H, SDLK_h),
            std::make_pair(ImGuiKey_I, SDLK_i),
            std::make_pair(ImGuiKey_J, SDLK_j),
            std::make_pair(ImGuiKey_K, SDLK_k),
            std::make_pair(ImGuiKey_L, SDLK_l),
            std::make_pair(ImGuiKey_M, SDLK_m),
            std::make_pair(ImGuiKey_N, SDLK_n),
            std::make_pair(ImGuiKey_O, SDLK_o),
            std::make_pair(ImGuiKey_P, SDLK_p),
            std::make_pair(ImGuiKey_Q, SDLK_q),
            std::make_pair(ImGuiKey_R, SDLK_r),
            std::make_pair(ImGuiKey_S, SDLK_s),
            std::make_pair(ImGuiKey_T, SDLK_t),
            std::make_pair(ImGuiKey_U, SDLK_u),
            std::make_pair(ImGuiKey_V, SDLK_v),
            std::make_pair(ImGuiKey_W, SDLK_w),
            std::make_pair(ImGuiKey_X, SDLK_x),
            std::make_pair(ImGuiKey_Y, SDLK_y),
            std::make_pair(ImGuiKey_Z, SDLK_z),
            std::make_pair(ImGuiKey_F1, SDLK_F1),
            std::make_pair(ImGuiKey_F2, SDLK_F2),
            std::make_pair(ImGuiKey_F3, SDLK_F3),
            std::make_pair(ImGuiKey_F4, SDLK_F4),
            std::make_pair(ImGuiKey_F5, SDLK_F5),
            std::make_pair(ImGuiKey_F6, SDLK_F6),
            std::make_pair(ImGuiKey_F7, SDLK_F7),
            std::make_pair(ImGuiKey_F8, SDLK_F8),
            std::make_pair(ImGuiKey_F9, SDLK_F9),
            std::make_pair(ImGuiKey_F10, SDLK_F10),
            std::make_pair(ImGuiKey_F11, SDLK_F11),
            std::make_pair(ImGuiKey_F12, SDLK_F12),
            std::make_pair(ImGuiKey_F13, SDLK_F13),
            std::make_pair(ImGuiKey_F14, SDLK_F14),
            std::make_pair(ImGuiKey_F15, SDLK_F15),
            std::make_pair(ImGuiKey_F16, SDLK_F16),
            std::make_pair(ImGuiKey_F17, SDLK_F17),
            std::make_pair(ImGuiKey_F18, SDLK_F18),
            std::make_pair(ImGuiKey_F19, SDLK_F19),
            std::make_pair(ImGuiKey_F20, SDLK_F20),
            std::make_pair(ImGuiKey_F21, SDLK_F21),
            std::make_pair(ImGuiKey_F22, SDLK_F22),
            std::make_pair(ImGuiKey_F23, SDLK_F23),
            std::make_pair(ImGuiKey_F24, SDLK_F24),
            std::make_pair(ImGuiKey_AppBack, SDLK_AC_BACK),
            std::make_pair(ImGuiKey_AppForward, SDLK_AC_FORWARD),
    };
}

SDLKeyboard::~SDLKeyboard() {
    delete[] oldKeystate;
    delete[] keystate;
}

void SDLKeyboard::update(float dt) {
    // Copy the the keystates to the oldKeystate array
    memcpy((void*)oldKeystate, keystate, numKeys);

    // Get the new keystates from SDL
    const Uint8 *sdlKeystate = SDL_GetKeyboardState(&numKeys);
    memcpy((void*)keystate, sdlKeystate, numKeys);
    modifier = SDL_GetModState();
}


// Keyboard keys
bool SDLKeyboard::isKeyDown(int button) {
    if (button < int(ImGuiKey::ImGuiKey_NamedKey_BEGIN)) {
        return keystate[SDL_GetScancodeFromKey(button)];
    } else {
        auto it = imGuiToSDLKMap.find(button);
        if (it == imGuiToSDLKMap.end()) {
            return false;
        }
        return keystate[SDL_GetScancodeFromKey(it->second)];
    }
}

bool SDLKeyboard::isKeyUp(int button) {
    return !isKeyDown(button);
}

bool SDLKeyboard::keyPressed(int button) {
    if (button < int(ImGuiKey::ImGuiKey_NamedKey_BEGIN)) {
        return keystate[SDL_GetScancodeFromKey(button)] && !oldKeystate[SDL_GetScancodeFromKey(button)];
    } else {
        auto it = imGuiToSDLKMap.find(button);
        if (it == imGuiToSDLKMap.end()) {
            return false;
        }
        auto scancode = SDL_GetScancodeFromKey(it->second);
        return keystate[scancode] && !oldKeystate[scancode];
    }
}

bool SDLKeyboard::keyReleased(int button) {
    if (button < int(ImGuiKey::ImGuiKey_NamedKey_BEGIN)) {
        return !keystate[SDL_GetScancodeFromKey(button)] && oldKeystate[SDL_GetScancodeFromKey(button)];
    } else {
        auto it = imGuiToSDLKMap.find(button);
        if (it == imGuiToSDLKMap.end()) {
            return false;
        }
        auto scancode = SDL_GetScancodeFromKey(it->second);
        return !keystate[scancode] && oldKeystate[scancode];
    }
}

bool SDLKeyboard::isScancodeDown(int button) {
    return keystate[button];
}

bool SDLKeyboard::isScancodeUp(int button) {
    return !keystate[button];
}

bool SDLKeyboard::scancodePressed(int button) {
    return keystate[button] && !oldKeystate[button];
}

bool SDLKeyboard::scancodeReleased(int button) {
    return !keystate[button] && oldKeystate[button];
}

int SDLKeyboard::getNumKeys() {
    return numKeys;
}

bool SDLKeyboard::getModifier(ImGuiKey modifierImGui) {
    if (modifierImGui == ImGuiMod_Ctrl) {
        return (modifier & KMOD_CTRL) != 0;
    } else if (modifierImGui == ImGuiMod_Shift) {
        return (modifier & KMOD_SHIFT) != 0;
    } else if (modifierImGui == ImGuiMod_Alt) {
        return (modifier & KMOD_ALT) != 0;
    }
    return false;
}

SDL_Keymod SDLKeyboard::getModifier() {
    return modifier;
}


// To support non-standard input methods a key buffer is needed.
// It contains the chars that were typed this frame as UTF-8 chars.
const char *SDLKeyboard::getKeyBuffer() const {
    return utf8KeyBuffer.c_str();
}

void SDLKeyboard::clearKeyBuffer() {
    utf8KeyBuffer = "";
}

void SDLKeyboard::addToKeyBuffer(const char *str) {
    utf8KeyBuffer += str;
}

}
