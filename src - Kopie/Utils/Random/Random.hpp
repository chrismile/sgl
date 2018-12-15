/*
 * Random.hpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef SYSTEM_RANDOM_RANDOM_HPP_
#define SYSTEM_RANDOM_RANDOM_HPP_

#include <ctime>
#include <cstdint>
#include <climits>
#include <algorithm>
#include <vector>
#include <list>

using namespace std;

namespace sgl {

class RandomGenerator
{
public:
	RandomGenerator() { seed = (uint32_t)time(NULL); }
	RandomGenerator(uint32_t _seed) { seed = _seed; }
	virtual ~RandomGenerator() {}
	virtual uint32_t getRandomUint32()=0;
	virtual int getRandomIntBetween(int min, int max);
	virtual float getRandomFloatBetween(float min, float max);

	// Shuffles the elements in the container
	template <class T>
	void shuffle(vector<T> &container)
	{
		for (int i = container.size() - 1; i > 0; --i) {
			int index = getRandomIntBetween(0, i);
			std::swap(container[i], container[index]);
		}
	}

	// WARNING: I suspect the code beneath to have produced flawed lists.
	// Please use "shuffle(vector<T> &container)" for now!
	/*template <class T>
	void shuffle(list<T> &container)
	{
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
