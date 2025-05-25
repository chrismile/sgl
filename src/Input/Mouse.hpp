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

#ifndef INPUT_MOUSE_HPP_
#define INPUT_MOUSE_HPP_

#include <utility>
#include <cstdint>
#include <Math/Geometry/Point2.hpp>
#include <Defs.hpp>

namespace sgl {

const uint32_t MOUSE_MOVED_EVENT = 1409365187U;

class DLL_OBJECT MouseInterface {
public:
    virtual ~MouseInterface() = default;
    virtual void update(float dt)=0;

    /// Mouse position
    virtual Point2 getAxis()=0;
    virtual int getX()=0;
    virtual int getY()=0;
    virtual Point2 mouseMovement()=0;
    virtual std::pair<float, float> getAxisFractional() { auto pt = getAxis(); return { float(pt.x), float(pt.y) }; }
    virtual float getXFractional() { return float(getX()); }
    virtual float getYFractional() { return float(getY()); }
    virtual std::pair<float, float> mouseMovementFractional() { auto pt = mouseMovement(); return { float(pt.x), float(pt.y) }; }
    virtual bool mouseMoved()=0;
    virtual void warp(const Point2 &windowPosition)=0;

    /// Mouse buttons
    virtual bool isButtonDown(int button)=0;
    virtual bool isButtonUp(int button)=0;
    virtual bool buttonPressed(int button)=0;
    virtual bool buttonReleased(int button)=0;
    /// -1: Scroll down; 0: No scrolling; 1: Scroll up
    virtual float getScrollWheel()=0;
};

DLL_OBJECT extern MouseInterface *Mouse;

}

/*! INPUT_MOUSE_HPP_ */
#endif
