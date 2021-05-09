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

#include <Input/Keyboard.hpp>
#include <Defs.hpp>
#include <SDL2/SDL_keycode.h>
#include <string>

namespace sgl {

class DLL_OBJECT SDLKeyboard : public KeyboardInterface
{
public:
    SDLKeyboard();
    virtual ~SDLKeyboard();
    virtual void update(float dt);

    //! Keyboard keys
    //! SDLK - logical keys
    virtual bool isKeyDown(int button);
    virtual bool isKeyUp(int button);
    virtual bool keyPressed(int button);
    virtual bool keyReleased(int button);
    //! SDL_SCANCODE - physical keys
    virtual bool isScancodeDown(int button);
    virtual bool isScancodeUp(int button);
    virtual bool scancodePressed(int button);
    virtual bool scancodeReleased(int button);
    virtual int getNumKeys();
    virtual SDL_Keymod getModifier();

    /*! To support non-standard input methods a key buffer is needed.
     * It contains the chars that were typed this frame as UTF-8 chars. */
    virtual const char *getKeyBuffer() const;
    virtual void clearKeyBuffer();
    virtual void addToKeyBuffer(const char *str);

public:
    int numKeys;
    //! State of the keyboard in the current and the last frame
    Uint8 *keystate, *oldKeystate;
    //! CTRL, SHIFT, etc.
    SDL_Keymod modifier;
    std::string utf8KeyBuffer;
};

}

/*! SDL_SDLKEYBOARD_HPP_ */
#endif
