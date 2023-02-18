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

#ifndef SYSTEM_RANDOM_RANDOM_HPP_
#define SYSTEM_RANDOM_RANDOM_HPP_

#include <ctime>
#include <cstdint>
#include <climits>
#include <algorithm>
#include <vector>
#include <list>

namespace sgl {

class DLL_OBJECT RandomGenerator {
public:
    RandomGenerator() { seed = (uint32_t)time(nullptr); }
    explicit RandomGenerator(uint32_t _seed) { seed = _seed; }
    virtual ~RandomGenerator() = default;
    virtual uint32_t getRandomUint32()=0;
    virtual int getRandomIntBetween(int min, int max);
    virtual float getRandomFloatBetween(float min, float max);

    // Shuffles the elements in the container
    template <class T>
    void shuffle(std::vector<T>& container) {
        for (int i = container.size() - 1; i > 0; --i) {
            int index = getRandomIntBetween(0, i);
            std::swap(container[i], container[index]);
        }
    }

    // WARNING: I suspect the code beneath to have produced flawed lists.
    // Please use "shuffle(vector<T> &container)" for now!
    /*template <class T>
    void shuffle(std::list<T> &container) {
        auto it = container.end();
        for (int i = container.size() - 1; i > 0; --i) {
            --it;
            if (it == container.begin()) break;
            int index = getRandomIntBetween(0, i);
            //std::iter_swap(it, std::advance(it, index));
            typename list<T>::iterator swapIt = it;
            std::advance(swapIt, index);
            std::swap(*it, *swapIt);
        }
    }*/

protected:
    uint32_t seed;
};

}

#endif /* SYSTEM_RANDOM_RANDOM_HPP_ */
