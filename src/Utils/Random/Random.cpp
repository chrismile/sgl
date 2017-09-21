/*
 * Random.cpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph
 */

#include "Random.hpp"

namespace sgl {

int RandomGenerator::getRandomIntBetween(int min, int max)
{
	if (min > max) {
		int temp = min;
		min = max;
		max = temp;
	}
	return ((getRandomUint32() % (max - min + 1)) + min);
}

float RandomGenerator::getRandomFloatBetween(float min, float max)
{
	if (min > max) {
		float temp = min;
		min = max;
		max = temp;
	}
	return (getRandomUint32() / static_cast<float>(4294967295UL) * (max - min) + min);
}

}
