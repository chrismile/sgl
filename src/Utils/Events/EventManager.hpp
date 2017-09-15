/*
 * EventManager.hpp
 *
 *  Created on: 10.09.2017
 *      Author: christoph
 */

#ifndef SRC_UTILS_EVENTS_EVENTMANAGER_HPP_
#define SRC_UTILS_EVENTS_EVENTMANAGER_HPP_

#include <cstdint>
#include <map>
#include <list>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "Stream/Stream.hpp"
#include <Utils/Singleton.hpp>

namespace sgl {

class Event;
typedef boost::shared_ptr<Event> EventPtr;
typedef std::function<void(EventPtr)> EventFunc;
typedef uint32_t ListenerToken;
typedef std::list<std::pair<ListenerToken, EventFunc>> EventFuncList;

class Event {
public:
	Event(uint32_t eventType) : eventType(eventType) {}
	virtual ~Event() {}
	inline uint32_t getType() const { return eventType; }
	virtual void serialize(WriteStream &stream) {}
	virtual void deserialize(ReadStream &stream) {}

private:
	uint32_t eventType;
};

class DLL_OBJECT EventManager : public Singleton<EventManager> {
public:
	EventManager();
	void update();

	// Creates a listener
	ListenerToken addListener(uint32_t eventType, EventFunc func);
	void removeListener(uint32_t eventType, ListenerToken token);

	// Event function is called instantly
	void triggerEvent(EventPtr event);
	// Adds an event to the event queue, which is updated by calling the function "update"
	void queueEvent(EventPtr event);
	//bool threadSafeQueueEvent(const EventDataPtr &event);


private:
	std::map<uint32_t, EventFuncList> listeners;
	std::list<EventPtr> eventQueue;
	uint32_t listenerCounter;
	//MultithreadedQueue<EventPtr> threadSafeEventQueue
};

}

#endif /* SRC_UTILS_EVENTS_EVENTMANAGER_HPP_ */
