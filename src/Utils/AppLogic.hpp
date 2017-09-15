/*
 * AppLogic.hpp
 *
 *  Created on: 02.09.2015
 *      Author: Christoph
 */

#ifndef LOGIC_APPLOGIC_HPP_
#define LOGIC_APPLOGIC_HPP_

#include <iostream>
#include <boost/shared_ptr.hpp>

namespace sgl {

class Event;
typedef boost::shared_ptr<Event> EventPtr;

class AppLogic
{
public:
	AppLogic();
	virtual ~AppLogic();
	virtual void run();
	virtual void update(float dt); // Call this function in derived class
	virtual void resolutionChanged(EventPtr event) { }
	virtual void render() {}
	virtual void setFPSCounterEnabled(bool enabled);

private:
	bool fpsCounterEnabled;
	bool running;
	bool screenshot;
};

}

#endif /* LOGIC_APPLOGIC_HPP_ */
