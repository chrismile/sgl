/*
 * EventManager.cpp
 *
 *  Created on: 11.09.2017
 *      Author: Christoph Neuhauser
 */

#include "EventManager.hpp"

namespace sgl {

EventManager::EventManager() {
	listenerCounter = 0;
}

void EventManager::update() {
	while (!eventQueue.empty()) {
		EventPtr event = eventQueue.front();
		eventQueue.pop_front();
		triggerEvent(event);
	}
}

ListenerToken EventManager::addListener(uint32_t eventType, EventFunc func) {
	ListenerToken token = listenerCounter++;

	EventFuncList &eventListenerList = listeners[eventType];
	eventListenerList.push_back(make_pair(token, func));

	return token;
}

void EventManager::removeListener(uint32_t eventType, ListenerToken token) {
	EventFuncList &listenerList = listeners[eventType];
	for (auto it = listenerList.begin(); it != listenerList.end(); it++) {
		if (it->first == token) {
			listenerList.erase(it);
			return;
		}
	}
}

// Event function is called instantly
void EventManager::triggerEvent(EventPtr event) {
	auto mapEntry = listeners.find(event->getType());
	if (mapEntry == listeners.end()) {
		return;
	}

	for (auto it = mapEntry->second.begin(); it != mapEntry->second.end(); it++) {
		it->second(event);
	}
}

// Adds an event to the event queue, which is updated by calling the function "update"
void EventManager::queueEvent(EventPtr event) {
	eventQueue.push_back(event);
}

}
