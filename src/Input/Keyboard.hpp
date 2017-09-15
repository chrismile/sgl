/*
 * Keyboard.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef INPUT_KEYBOARD_HPP_
#define INPUT_KEYBOARD_HPP_

#include <Defs.hpp>
#include <SDL2/SDL_keycode.h>

namespace sgl {

class DLL_OBJECT KeyboardInterface
{
public:
	virtual ~KeyboardInterface() {}
	virtual void update(float dt)=0;

	// Keyboard keys
	virtual bool isKeyDown(int button)=0; // SDLK - logical keys
	virtual bool isKeyUp(int button)=0;
	virtual bool keyPressed(int button)=0;
	virtual bool keyReleased(int button)=0;
	virtual bool isScancodeDown(int button)=0; // SDL_SCANCODE - physical keys
	virtual bool isScancodeUp(int button)=0;
	virtual bool scancodePressed(int button)=0;
	virtual bool scancodeReleased(int button)=0;
	virtual int getNumKeys()=0;
	virtual SDL_Keymod getModifier()=0;

	// To support non-standard input methods a key buffer is needed.
	// It contains the chars that were typed this frame as UTF-8 chars.
	virtual const char *getKeyBuffer() const=0;
	virtual void clearKeyBuffer()=0;
	virtual void addToKeyBuffer(const char *str)=0;
};

extern KeyboardInterface *Keyboard;

}

#endif /* INPUT_KEYBOARD_HPP_ */
