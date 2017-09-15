/*
 * StringUtils.hpp
 *
 *  Created on: 11.09.2017
 *      Author: christoph
 */

#ifndef SRC_UTILS_STRINGUTILS_HPP_
#define SRC_UTILS_STRINGUTILS_HPP_

#include <string>
#include <cstring>
#include <vector>

namespace sgl {

// Converts strings like "This is a test!" with seperator ' ' to { "This", "is", "a", "test!" }
template<class InputIterator>
void splitString(const char *stringObject, char seperator, InputIterator list) {
	size_t stringObjectLength = strlen(stringObject);
	std::string buffer = "";
	for (size_t i = 0; i < stringObjectLength; ++i) {
		if (stringObject[i] != seperator) {
			buffer += stringObject[i];
		} else {
			if (buffer.length() > 0) {
				list.push_back(buffer);
				buffer = "";
			}
		}
	}
	if (buffer.length() > 0) {
		list.push_back(buffer);
		buffer = "";
	}
}


}

#endif /* SRC_UTILS_STRINGUTILS_HPP_ */
