/*!
 * Mouse.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef INPUT_MOUSE_HPP_
#define INPUT_MOUSE_HPP_

#include <Math/Geometry/Point2.hpp>
#include <Defs.hpp>

namespace sgl {

const uint32_t MOUSE_MOVED_EVENT = 1409365187U;

class DLL_OBJECT MouseInterface
{
public:
	virtual ~MouseInterface() {}
	virtual void update(float dt)=0;

	//! Mouse position
	virtual Point2 getAxis()=0;
	virtual int getX()=0;
	virtual int getY()=0;
	virtual Point2 mouseMovement()=0;
	virtual bool mouseMoved()=0;
	virtual void warp(const Point2 &windowPosition)=0;

	//! Mouse buttons
	virtual bool isButtonDown(int button)=0;
	virtual bool isButtonUp(int button)=0;
	virtual bool buttonPressed(int button)=0;
	virtual bool buttonReleased(int button)=0;
	//! -1: Scroll down; 0: No scrolling; 1: Scroll up
	virtual float getScrollWheel()=0;
};

extern MouseInterface *Mouse;

}

/*! INPUT_MOUSE_HPP_ */
#endif
