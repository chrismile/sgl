/*!
 * AppLogic.hpp
 *
 *  Created on: 02.09.2015
 *      Author: Christoph Neuhauser
 */

#ifndef LOGIC_APPLOGIC_HPP_
#define LOGIC_APPLOGIC_HPP_

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <SDL2/SDL.h>

#include "Utils/FramerateSmoother.hpp"

namespace sgl {

class Event;
typedef boost::shared_ptr<Event> EventPtr;

class AppLogic
{
public:
    AppLogic();
    virtual ~AppLogic();
    virtual void run();
    virtual void updateBase(float dt);

    /// Override these functions in the derived classes
    virtual void update(float dt) {} // Called once per rendered frame
    virtual void updateFixed(float dt) {} // Called at a fixed rate (e.g. for physics simulation)
    virtual void processSDLEvent(const SDL_Event &event) {}
    virtual void resolutionChanged(EventPtr event) {}
    virtual void render() {}

    virtual void setPrintFPS(bool enabled);
    inline float getFPS() { return fps; }
    inline void quit() { running = false; }

protected:
    virtual void saveScreenshot(const std::string &filename);
    float fps;
    FramerateSmoother framerateSmoother;

private:
    void makeScreenshot();
    bool running;
    bool screenshot;

    uint64_t fpsCounterUpdateFrequency;
    bool printFPS;

};

}

/*! LOGIC_APPLOGIC_HPP_ */
#endif
