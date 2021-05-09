/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#ifndef HEXVOLUMERENDERER_CIRCULARQUEUE_HPP
#define HEXVOLUMERENDERER_CIRCULARQUEUE_HPP

#include <cstddef>
#include <cassert>

template<class T>
class CircularQueue
{
public:
    CircularQueue(size_t maxCapacity = 32) {
        startPointer = 0;
        endPointer = 0;
        queueCapacity = maxCapacity;
        queueSize = 0;
        if (maxCapacity != 0) {
            queueData = new T[queueCapacity];
        } else {
            queueData = nullptr;
        }
    }
    ~CircularQueue() {
        delete[] queueData;
    }

    void enqueue(const T& data) {
        if (queueSize == queueCapacity) {
            resize(queueCapacity == 0 ? 4 : queueCapacity * 2);
        }

        queueData[endPointer] = data;
        endPointer = (endPointer + 1) % queueCapacity;
        queueSize++;
    }
    T popFront() {
        assert(queueSize > 0);
        T data = queueData[startPointer];
        startPointer = (startPointer + 1) % queueCapacity;
        queueSize--;
        return data;
    }

    inline size_t isEmpty() { return queueSize == 0; }
    inline size_t getSize() { return queueSize; }
    inline size_t getCapacity() { return queueCapacity; }

    void resize(size_t newCapacity) {
        // Copy data to larger array.
        T *newData = new T[newCapacity];
        int readIdx = startPointer;
        int writeIdx = 0;
        for (int i = 0; i < queueSize; i++) {
            newData[writeIdx] = queueData[readIdx];
            readIdx = (readIdx + 1) % queueCapacity;
            writeIdx++;
        }

        // Reset the pointers.
        startPointer = 0;
        endPointer = queueSize;
        queueCapacity = newCapacity;

        // Delete the old data and copy the new data.
        if (queueData) {
            delete[] queueData;
        }
        queueData = newData;
    }

private:
    T *queueData;
    size_t startPointer, endPointer;
    size_t queueCapacity;
    size_t queueSize;
};

#endif //HEXVOLUMERENDERER_CIRCULARQUEUE_HPP
