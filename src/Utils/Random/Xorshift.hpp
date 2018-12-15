/*
 * Xorshift.hpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef SYSTEM_RANDOM_XORSHIFT_HPP_
#define SYSTEM_RANDOM_XORSHIFT_HPP_

#include "Random.hpp"

namespace sgl {

class XorshiftRandomGenerator : public RandomGenerator
{
public:
	XorshiftRandomGenerator() : RandomGenerator() { initialize(); }
	XorshiftRandomGenerator(uint32_t _seed) : RandomGenerator(_seed) { initialize(); }
	virtual ~XorshiftRandomGenerator() {}
	uint32_t getRandomUint32() { return xorshift96(); }

private:
	void initialize();
	uint32_t xorshift96();
	uint32_t x, y, z;
};

}

#endif /* SYSTEM_RANDOM_XORSHIFT_HPP_ */
