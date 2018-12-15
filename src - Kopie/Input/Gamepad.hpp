/*!
 * Gamepad.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef INPUT_GAMEPAD_HPP_
#define INPUT_GAMEPAD_HPP_

#include <Defs.hpp>
#include <glm/vec2.hpp>

namespace sgl {

class DLL_OBJECT GamepadInterface
{
public:
	virtual ~GamepadInterface() {}
	virtual void update(float dt)=0;
	//! Re-open all gamepads
	virtual void refresh() {}
	virtual int getNumGamepads()=0;
	virtual const char *getGamepadName(int j)=0;

	//! Gamepad buttons
	virtual bool isButtonDown(int button, int gamepadIndex = 0)=0;
	virtual bool isButtonUp(int button, int gamepadIndex = 0)=0;
	virtual bool buttonPressed(int button, int gamepadIndex = 0)=0;
	virtual bool buttonReleased(int button, int gamepadIndex = 0)=0;
	virtual int getNumButtons(int gamepadIndex = 0)=0;

	//! Gamepad control stick axes
	virtual float axisX(int stickIndex = 0, int gamepadIndex = 0)=0;
	virtual float axisY(int stickIndex = 0, int gamepadIndex = 0)=0;
	virtual glm::vec2 axis(int stickIndex = 0, int gamepadIndex = 0)=0;
	virtual uint8_t getDirectionPad(int dirPadIndex = 0, int gamepadIndex = 0)=0;
	virtual uint8_t getDirectionPadPressed(int dirPadIndex = 0, int gamepadIndex = 0)=0;

	//! Force Feedback support:
	//! time in seconds
	virtual void rumble(float strength, float time, int gamepadIndex = 0)=0;
};

extern GamepadInterface *Gamepad;

}

/*! INPUT_GAMEPAD_HPP_ */
#endif
