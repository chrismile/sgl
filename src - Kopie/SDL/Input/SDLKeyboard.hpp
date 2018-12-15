/*!
 * SDLKeyboard.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
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
