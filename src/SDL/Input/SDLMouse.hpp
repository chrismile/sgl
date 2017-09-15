/*!
 * SDLMouse.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef SDL_SDLMOUSE_HPP_
#define SDL_SDLMOUSE_HPP_

#include <Input/Mouse.hpp>
#include <Math/Geometry/Point2.hpp>

namespace sgl {

struct MouseState {
	MouseState() : buttonState(0), scrollWheel(0) {}
	int buttonState;
	Point2 pos;
	int scrollWheel;
};

class DLL_OBJECT SDLMouse : public MouseInterface
{
public:
	SDLMouse();
	virtual ~SDLMouse();
	virtual void update(float dt);

	//! Mouse position
	virtual Point2 getAxis();
	virtual int getX();
	virtual int getY();
	virtual Point2 mouseMovement();
	virtual bool mouseMoved();
	virtual void warp(const Point2 &windowPosition);

	//! Mouse buttons
	virtual bool isButtonDown(int button);
	virtual bool isButtonUp(int button);
	virtual bool buttonPressed(int button);
	virtual bool buttonReleased(int button);
	//! -1: Scroll down; 0: No scrolling; 1: Scroll up
	virtual float getScrollWheel();

	//! Function for event processing (SDL only suppots querying scroll wheel state within the event queue)
	void setScrollWheelValue(int val);

protected:
	//! States in the current and last frame
	MouseState state, oldState;
};

}

/*! SDL_SDLMOUSE_HPP_ */
#endif
