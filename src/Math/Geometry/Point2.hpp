/*!
 * Point2.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef MATH_POINT2_HPP_
#define MATH_POINT2_HPP_

namespace sgl {

class Point2
{
public:
	Point2(int _x, int _y) : x(_x), y(_y) {}
	Point2() : x(0), y(0) {}
	int x, y;
};

}

/*! MATH_POINT2_HPP_ */
#endif
