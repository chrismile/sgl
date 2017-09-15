/*
 * SDLKeyboard.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
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

	// Keyboard keys
	virtual bool isKeyDown(int button); // SDLK - logical keys
	virtual bool isKeyUp(int button);
	virtual bool keyPressed(int button);
	virtual bool keyReleased(int button);
	virtual bool isScancodeDown(int button); // SDL_SCANCODE - physical keys
	virtual bool isScancodeUp(int button);
	virtual bool scancodePressed(int button);
	virtual bool scancodeReleased(int button);
	virtual int getNumKeys();
	virtual SDL_Keymod getModifier();

	// To support non-standard input methods a key buffer is needed.
	// It contains the chars that were typed this frame as UTF-8 chars.
	virtual const char *getKeyBuffer() const;
	virtual void clearKeyBuffer();
	virtual void addToKeyBuffer(const char *str);

public:
	int numKeys;
	Uint8 *keystate, *oldKeystate; // State of the keyboard in the current and the last frame
	SDL_Keymod modifier; // CTRL, SHIFT, etc.
	std::string utf8KeyBuffer;
};

}

#endif /* SDL_SDLKEYBOARD_HPP_ */
