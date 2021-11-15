/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

ListenerToken EventManager::addListener(uint32_t eventType, const EventFunc& func) {
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
void EventManager::triggerEvent(const EventPtr& event) {
    auto mapEntry = listeners.find(event->getType());
    if (mapEntry == listeners.end()) {
        return;
    }

    for (auto& it : mapEntry->second) {
        it.second(event);
    }
}

// Adds an event to the event queue, which is updated by calling the function "update"
void EventManager::queueEvent(const EventPtr& event) {
    eventQueue.push_back(event);
}

}
