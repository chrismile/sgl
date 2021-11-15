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

#ifndef SRC_UTILS_EVENTS_EVENTMANAGER_HPP_
#define SRC_UTILS_EVENTS_EVENTMANAGER_HPP_

#include <cstdint>
#include <map>
#include <list>
#include <functional>
#include <memory>
#include "Stream/Stream.hpp"
#include <Utils/Singleton.hpp>

namespace sgl {

class Event;
typedef std::shared_ptr<Event> EventPtr;
typedef std::function<void(const EventPtr&)> EventFunc;
typedef uint32_t ListenerToken;
typedef std::list<std::pair<ListenerToken, EventFunc>> EventFuncList;

class DLL_OBJECT Event {
public:
    explicit Event(uint32_t eventType) : eventType(eventType) {}
    virtual ~Event() = default;
    [[nodiscard]] inline uint32_t getType() const { return eventType; }
    virtual void serialize(WriteStream &stream) {}
    virtual void deserialize(ReadStream &stream) {}

private:
    uint32_t eventType;
};

class DLL_OBJECT EventManager : public Singleton<EventManager> {
public:
    EventManager();
    void update();

    //! Creates a listener
    ListenerToken addListener(uint32_t eventType, const EventFunc& func);
    void removeListener(uint32_t eventType, ListenerToken token);

    //! Event function is called instantly
    void triggerEvent(const EventPtr& event);
    //! Adds an event to the event queue, which is updated by calling the function "update"
    void queueEvent(const EventPtr& event);
    //bool threadSafeQueueEvent(const EventDataPtr &event);


private:
    std::map<uint32_t, EventFuncList> listeners;
    std::list<EventPtr> eventQueue;
    uint32_t listenerCounter;
    //MultithreadedQueue<EventPtr> threadSafeEventQueue
};

}

/*! SRC_UTILS_EVENTS_EVENTMANAGER_HPP_ */
#endif
