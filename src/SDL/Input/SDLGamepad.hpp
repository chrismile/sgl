/*
 * SDLGamepad.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef SDL_SDLGAMEPAD_HPP_
#define SDL_SDLGAMEPAD_HPP_

#include <SDL2/SDL.h>
#include <vector>
#include <Input/Gamepad.hpp>
#include <glm/vec2.hpp>

namespace sgl {

class OldGamepadState;

class DLL_OBJECT SDLGamepad : public GamepadInterface
{
public:
	SDLGamepad();
	virtual ~SDLGamepad();
	void initialize();
	void release();
	virtual void update(float dt);
	virtual void refresh(); // Re-open all gamepads
	virtual int getNumGamepads();
	virtual const char *getGamepadName(int j);

	// Gamepad buttons
	virtual bool isButtonDown(int button, int gamepadIndex = 0);
	virtual bool isButtonUp(int button, int gamepadIndex = 0);
	virtual bool buttonPressed(int button, int gamepadIndex = 0);
	virtual bool buttonReleased(int button, int gamepadIndex = 0);
	virtual int getNumButtons(int gamepadIndex = 0);

	// Gamepad control stick axes
	virtual float axisX(int stickIndex = 0, int gamepadIndex = 0);
	virtual float axisY(int stickIndex = 0, int gamepadIndex = 0);
	virtual glm::vec2 axis(int stickIndex = 0, int gamepadIndex = 0);
	virtual uint8_t getDirectionPad(int dirPadIndex = 0, int gamepadIndex = 0);
	virtual uint8_t getDirectionPadPressed(int dirPadIndex = 0, int gamepadIndex = 0);

	// Force Feedback support:
	virtual void rumble(float strength, float time, int gamepadIndex = 0); // time in seconds

protected:
	std::vector<SDL_Joystick*> gamepads; // Array containing the state of the gamepads
	std::vector<OldGamepadState*> oldGamepads; // Array containing the state of the gamepads in the last frame
	int numGamepads;

	std::vector<SDL_Haptic*> hapticList;
	std::vector<bool> rumbleInited;
};

}

#endif /* SDL_SDLGAMEPAD_HPP_ */
