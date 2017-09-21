/*
 * Xorshift.cpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph
 */

#include "Xorshift.hpp"

namespace sgl {

void XorshiftRandomGenerator::initialize()
{
	x = 123456789UL ^ seed;
	y = 362436069UL ^ seed;
	z = 521288629UL ^ seed;
}

uint32_t XorshiftRandomGenerator::xorshift96() // Period: 2^96-1
{
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	uint32_t t = x;
	x = y;
	y = z;
	z = t ^ x ^ y;

	return z;
}

}
