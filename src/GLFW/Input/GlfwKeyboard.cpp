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

#ifdef SUPPORT_SDL3
#define SDL_ENABLE_OLD_NAMES
#endif
#include <GLFW/glfw3.h>
#include <GLFW/GlfwWindow.hpp>
#include <Utils/AppSettings.hpp>
#include "GlfwKeyboard.hpp"

namespace sgl {

GlfwKeyboard::GlfwKeyboard() {
    // Set the number of keys.
    numKeys = GLFW_KEY_LAST + 1;

    // Allocate and initialize the memory for the keystates.
    oldKeystate = new uint8_t[numKeys];
    keystate = new uint8_t[numKeys];
    memset((void*)oldKeystate, 0, numKeys);
    memset((void*)keystate, 0, numKeys);
    modifier = 0;

    // For new v2 input API. Values obtained from ImGui_ImplSDL2_KeycodeToImGuiKey.
    imGuiToGlfwKeyMap = std::unordered_map<int, int>{
            std::make_pair(ImGuiKey_Tab, GLFW_KEY_TAB),
            std::make_pair(ImGuiKey_LeftArrow, GLFW_KEY_LEFT),
            std::make_pair(ImGuiKey_RightArrow, GLFW_KEY_RIGHT),
            std::make_pair(ImGuiKey_UpArrow, GLFW_KEY_UP),
            std::make_pair(ImGuiKey_DownArrow, GLFW_KEY_DOWN),
            std::make_pair(ImGuiKey_PageUp, GLFW_KEY_PAGE_UP),
            std::make_pair(ImGuiKey_PageDown, GLFW_KEY_PAGE_DOWN),
            std::make_pair(ImGuiKey_Home, GLFW_KEY_HOME),
            std::make_pair(ImGuiKey_End, GLFW_KEY_END),
            std::make_pair(ImGuiKey_Insert, GLFW_KEY_INSERT),
            std::make_pair(ImGuiKey_Delete, GLFW_KEY_DELETE),
            std::make_pair(ImGuiKey_Backspace, GLFW_KEY_BACKSPACE),
            std::make_pair(ImGuiKey_Space, GLFW_KEY_SPACE),
            std::make_pair(ImGuiKey_Enter, GLFW_KEY_ENTER),
            std::make_pair(ImGuiKey_Escape, GLFW_KEY_ESCAPE),
            std::make_pair(ImGuiKey_Apostrophe, GLFW_KEY_APOSTROPHE),
            std::make_pair(ImGuiKey_Comma, GLFW_KEY_COMMA),
            std::make_pair(ImGuiKey_Minus, GLFW_KEY_MINUS),
            std::make_pair(ImGuiKey_Period, GLFW_KEY_PERIOD),
            std::make_pair(ImGuiKey_Slash, GLFW_KEY_SLASH),
            std::make_pair(ImGuiKey_Semicolon, GLFW_KEY_SEMICOLON),
            std::make_pair(ImGuiKey_Equal, GLFW_KEY_EQUAL),
            std::make_pair(ImGuiKey_LeftBracket, GLFW_KEY_LEFT_BRACKET),
            std::make_pair(ImGuiKey_Backslash, GLFW_KEY_BACKSLASH),
            std::make_pair(ImGuiKey_RightBracket, GLFW_KEY_RIGHT_BRACKET),
            std::make_pair(ImGuiKey_GraveAccent, GLFW_KEY_GRAVE_ACCENT),
            std::make_pair(ImGuiKey_CapsLock, GLFW_KEY_CAPS_LOCK),
            std::make_pair(ImGuiKey_ScrollLock, GLFW_KEY_SCROLL_LOCK),
            std::make_pair(ImGuiKey_NumLock, GLFW_KEY_NUM_LOCK),
            std::make_pair(ImGuiKey_PrintScreen, GLFW_KEY_PRINT_SCREEN),
            std::make_pair(ImGuiKey_Pause, GLFW_KEY_PAUSE),
            std::make_pair(ImGuiKey_Keypad0, GLFW_KEY_KP_0),
            std::make_pair(ImGuiKey_Keypad1, GLFW_KEY_KP_1),
            std::make_pair(ImGuiKey_Keypad2, GLFW_KEY_KP_2),
            std::make_pair(ImGuiKey_Keypad3, GLFW_KEY_KP_3),
            std::make_pair(ImGuiKey_Keypad4, GLFW_KEY_KP_4),
            std::make_pair(ImGuiKey_Keypad5, GLFW_KEY_KP_5),
            std::make_pair(ImGuiKey_Keypad6, GLFW_KEY_KP_6),
            std::make_pair(ImGuiKey_Keypad7, GLFW_KEY_KP_7),
            std::make_pair(ImGuiKey_Keypad8, GLFW_KEY_KP_8),
            std::make_pair(ImGuiKey_Keypad9, GLFW_KEY_KP_9),
            std::make_pair(ImGuiKey_KeypadDecimal, GLFW_KEY_KP_DECIMAL),
            std::make_pair(ImGuiKey_KeypadDivide, GLFW_KEY_KP_DIVIDE),
            std::make_pair(ImGuiKey_KeypadMultiply, GLFW_KEY_KP_MULTIPLY),
            std::make_pair(ImGuiKey_KeypadSubtract, GLFW_KEY_KP_SUBTRACT),
            std::make_pair(ImGuiKey_KeypadAdd, GLFW_KEY_KP_ADD),
            std::make_pair(ImGuiKey_KeypadEnter, GLFW_KEY_KP_ENTER),
            std::make_pair(ImGuiKey_KeypadEqual, GLFW_KEY_KP_EQUAL),
            std::make_pair(ImGuiKey_LeftShift, GLFW_KEY_LEFT_SHIFT),
            std::make_pair(ImGuiKey_LeftCtrl, GLFW_KEY_LEFT_CONTROL),
            std::make_pair(ImGuiKey_LeftAlt, GLFW_KEY_LEFT_ALT),
            std::make_pair(ImGuiKey_LeftSuper, GLFW_KEY_LEFT_SUPER),
            std::make_pair(ImGuiKey_RightShift, GLFW_KEY_RIGHT_SHIFT),
            std::make_pair(ImGuiKey_RightCtrl, GLFW_KEY_RIGHT_CONTROL),
            std::make_pair(ImGuiKey_RightAlt, GLFW_KEY_RIGHT_ALT),
            std::make_pair(ImGuiKey_RightSuper, GLFW_KEY_RIGHT_SUPER),
            std::make_pair(ImGuiKey_Menu, GLFW_KEY_MENU),
            std::make_pair(ImGuiKey_0, GLFW_KEY_0),
            std::make_pair(ImGuiKey_1, GLFW_KEY_1),
            std::make_pair(ImGuiKey_2, GLFW_KEY_2),
            std::make_pair(ImGuiKey_3, GLFW_KEY_3),
            std::make_pair(ImGuiKey_4, GLFW_KEY_4),
            std::make_pair(ImGuiKey_5, GLFW_KEY_5),
            std::make_pair(ImGuiKey_6, GLFW_KEY_6),
            std::make_pair(ImGuiKey_7, GLFW_KEY_7),
            std::make_pair(ImGuiKey_8, GLFW_KEY_8),
            std::make_pair(ImGuiKey_9, GLFW_KEY_9),
            std::make_pair(ImGuiKey_A, GLFW_KEY_A),
            std::make_pair(ImGuiKey_B, GLFW_KEY_B),
            std::make_pair(ImGuiKey_C, GLFW_KEY_C),
            std::make_pair(ImGuiKey_D, GLFW_KEY_D),
            std::make_pair(ImGuiKey_E, GLFW_KEY_E),
            std::make_pair(ImGuiKey_F, GLFW_KEY_F),
            std::make_pair(ImGuiKey_G, GLFW_KEY_G),
            std::make_pair(ImGuiKey_H, GLFW_KEY_H),
            std::make_pair(ImGuiKey_I, GLFW_KEY_I),
            std::make_pair(ImGuiKey_J, GLFW_KEY_J),
            std::make_pair(ImGuiKey_K, GLFW_KEY_K),
            std::make_pair(ImGuiKey_L, GLFW_KEY_L),
            std::make_pair(ImGuiKey_M, GLFW_KEY_M),
            std::make_pair(ImGuiKey_N, GLFW_KEY_N),
            std::make_pair(ImGuiKey_O, GLFW_KEY_O),
            std::make_pair(ImGuiKey_P, GLFW_KEY_P),
            std::make_pair(ImGuiKey_Q, GLFW_KEY_Q),
            std::make_pair(ImGuiKey_R, GLFW_KEY_R),
            std::make_pair(ImGuiKey_S, GLFW_KEY_S),
            std::make_pair(ImGuiKey_T, GLFW_KEY_T),
            std::make_pair(ImGuiKey_U, GLFW_KEY_U),
            std::make_pair(ImGuiKey_V, GLFW_KEY_V),
            std::make_pair(ImGuiKey_W, GLFW_KEY_W),
            std::make_pair(ImGuiKey_X, GLFW_KEY_X),
            std::make_pair(ImGuiKey_Y, GLFW_KEY_Y),
            std::make_pair(ImGuiKey_Z, GLFW_KEY_Z),
            std::make_pair(ImGuiKey_F1, GLFW_KEY_F1),
            std::make_pair(ImGuiKey_F2, GLFW_KEY_F2),
            std::make_pair(ImGuiKey_F3, GLFW_KEY_F3),
            std::make_pair(ImGuiKey_F4, GLFW_KEY_F4),
            std::make_pair(ImGuiKey_F5, GLFW_KEY_F5),
            std::make_pair(ImGuiKey_F6, GLFW_KEY_F6),
            std::make_pair(ImGuiKey_F7, GLFW_KEY_F7),
            std::make_pair(ImGuiKey_F8, GLFW_KEY_F8),
            std::make_pair(ImGuiKey_F9, GLFW_KEY_F9),
            std::make_pair(ImGuiKey_F10, GLFW_KEY_F10),
            std::make_pair(ImGuiKey_F11, GLFW_KEY_F11),
            std::make_pair(ImGuiKey_F12, GLFW_KEY_F12),
            std::make_pair(ImGuiKey_F13, GLFW_KEY_F13),
            std::make_pair(ImGuiKey_F14, GLFW_KEY_F14),
            std::make_pair(ImGuiKey_F15, GLFW_KEY_F15),
            std::make_pair(ImGuiKey_F16, GLFW_KEY_F16),
            std::make_pair(ImGuiKey_F17, GLFW_KEY_F17),
            std::make_pair(ImGuiKey_F18, GLFW_KEY_F18),
            std::make_pair(ImGuiKey_F19, GLFW_KEY_F19),
            std::make_pair(ImGuiKey_F20, GLFW_KEY_F20),
            std::make_pair(ImGuiKey_F21, GLFW_KEY_F21),
            std::make_pair(ImGuiKey_F22, GLFW_KEY_F22),
            std::make_pair(ImGuiKey_F23, GLFW_KEY_F23),
            std::make_pair(ImGuiKey_F24, GLFW_KEY_F24),

    };
}

GlfwKeyboard::~GlfwKeyboard() {
    delete[] oldKeystate;
    delete[] keystate;
}

void GlfwKeyboard::update(float dt) {
    // Copy the the keystates to the oldKeystate array
    memcpy((void*)oldKeystate, keystate, numKeys);

    // Get the new keystates from GLFW
    auto* glfwWindow = static_cast<GlfwWindow*>(AppSettings::get()->getMainWindow())->getGlfwWindow();
    for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++) {
        keystate[i] = uint8_t(glfwGetKey(glfwWindow, i));
    }

    modifier = 0;
    if (keystate[GLFW_KEY_LEFT_CONTROL] || keystate[GLFW_KEY_RIGHT_CONTROL]) {
        modifier |= ImGuiMod_Ctrl;
    }
    if (keystate[GLFW_KEY_LEFT_SHIFT] || keystate[GLFW_KEY_RIGHT_SHIFT]) {
        modifier |= ImGuiMod_Shift;
    }
    if (keystate[GLFW_KEY_LEFT_ALT] || keystate[GLFW_KEY_RIGHT_ALT]) {
        modifier |= ImGuiMod_Alt;
    }
    if (keystate[GLFW_KEY_LEFT_SUPER] || keystate[GLFW_KEY_RIGHT_SUPER]) {
        modifier |= ImGuiMod_Super;
    }
}

void GlfwKeyboard::onKey(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        const char* clipboardText = glfwGetClipboardString(nullptr);
        if (clipboardText) {
            addToKeyBuffer(clipboardText);
        }
    }
}

void GlfwKeyboard::onChar(unsigned int codepoint) {
    ;
}

void GlfwKeyboard::onCharMods(unsigned int codepoint, int mods) {
    ;
}


// Keyboard keys
bool GlfwKeyboard::isKeyDown(int button) {
    if (button <= int(GLFW_KEY_LAST)) {
        return keystate[button];
    } else {
        auto it = imGuiToGlfwKeyMap.find(button);
        if (it == imGuiToGlfwKeyMap.end()) {
            return false;
        }
        return keystate[it->second];
    }
}

bool GlfwKeyboard::isKeyUp(int button) {
    return !isKeyDown(button);
}

bool GlfwKeyboard::keyPressed(int button) {
    if (button <= int(GLFW_KEY_LAST)) {
        return keystate[button] && !oldKeystate[button];
    } else {
        auto it = imGuiToGlfwKeyMap.find(button);
        if (it == imGuiToGlfwKeyMap.end()) {
            return false;
        }
        auto scancode = it->second;
        return keystate[scancode] && !oldKeystate[scancode];
    }
}

bool GlfwKeyboard::keyReleased(int button) {
    if (button < int(ImGuiKey::ImGuiKey_NamedKey_BEGIN)) {
        return !keystate[button] && oldKeystate[button];
    } else {
        auto it = imGuiToGlfwKeyMap.find(button);
        if (it == imGuiToGlfwKeyMap.end()) {
            return false;
        }
        auto scancode = it->second;
        return !keystate[scancode] && oldKeystate[scancode];
    }
}

bool GlfwKeyboard::isScancodeDown(int button) {
    // TODO: glfwGetKeyScancode
    return keystate[button];
}

bool GlfwKeyboard::isScancodeUp(int button) {
    // TODO: glfwGetKeyScancode
    return !keystate[button];
}

bool GlfwKeyboard::scancodePressed(int button) {
    // TODO: glfwGetKeyScancode
    return keystate[button] && !oldKeystate[button];
}

bool GlfwKeyboard::scancodeReleased(int button) {
    // TODO: glfwGetKeyScancode
    return !keystate[button] && oldKeystate[button];
}

int GlfwKeyboard::getNumKeys() {
    return numKeys;
}

bool GlfwKeyboard::getModifier(ImGuiKey modifierImGui) {
    return (modifier & modifierImGui) != 0;
}

#if defined(SUPPORT_SDL)
SDL_Keymod GlfwKeyboard::getModifier() {
#ifdef SUPPORT_SDL3
    int keymod = SDL_KMOD_NONE;
    if ((modifier & SDL_KMOD_CTRL) != 0) {
        keymod |= SDL_KMOD_CTRL;
    } else if ((modifier & SDL_KMOD_SHIFT) != 0) {
        keymod |= SDL_KMOD_SHIFT;
    } else if ((modifier & SDL_KMOD_ALT) != 0) {
        keymod |= SDL_KMOD_ALT;
    }
#else
    int keymod = KMOD_NONE;
    if ((modifier & KMOD_CTRL) != 0) {
        keymod |= KMOD_CTRL;
    } else if ((modifier & KMOD_SHIFT) != 0) {
        keymod |= KMOD_SHIFT;
    } else if ((modifier & KMOD_ALT) != 0) {
        keymod |= KMOD_ALT;
    }
#endif
    return SDL_Keymod(keymod);
}
#endif


// To support non-standard input methods a key buffer is needed.
// It contains the chars that were typed this frame as UTF-8 chars.
const char *GlfwKeyboard::getKeyBuffer() const {
    return utf8KeyBuffer.c_str();
}

void GlfwKeyboard::clearKeyBuffer() {
    utf8KeyBuffer = "";
}

void GlfwKeyboard::addToKeyBuffer(const char *str) {
    utf8KeyBuffer += str;
}

}
