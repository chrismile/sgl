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

#ifndef INPUT_KEYBOARD_HPP_
#define INPUT_KEYBOARD_HPP_

#include <Defs.hpp>
#include <SDL2/SDL_keycode.h>

namespace sgl {

class DLL_OBJECT KeyboardInterface {
public:
    virtual ~KeyboardInterface() = default;
    virtual void update(float dt)=0;

    /// Keyboard keys
    /// SDLK - logical keys
    virtual bool isKeyDown(int button)=0;
    virtual bool isKeyUp(int button)=0;
    virtual bool keyPressed(int button)=0;
    virtual bool keyReleased(int button)=0;
    /// SDL_SCANCODE - physical keys
    virtual bool isScancodeDown(int button)=0;
    virtual bool isScancodeUp(int button)=0;
    virtual bool scancodePressed(int button)=0;
    virtual bool scancodeReleased(int button)=0;
    virtual int getNumKeys()=0;
    virtual SDL_Keymod getModifier()=0;

    /**
     * To support non-standard input methods a key buffer is needed.
     * It contains the chars that were typed this frame as UTF-8 chars.
     */
    virtual const char *getKeyBuffer() const=0;
    virtual void clearKeyBuffer()=0;
    virtual void addToKeyBuffer(const char *str)=0;
};

DLL_OBJECT extern KeyboardInterface *Keyboard;

}

/*! INPUT_KEYBOARD_HPP_ */
#endif
