/*!
 * Execute.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_UTILS_EXECUTE_HPP_
#define SRC_UTILS_EXECUTE_HPP_

#include <string>
#include <list>

namespace sgl {

//! Executes the program "appName" with the argument list "args".
int executeProgram(const char *appName, std::list<std::string> &args);

//! Returns output of program
std::string exec(const char* command);

}

/*! SRC_UTILS_EXECUTE_HPP_ */
#endif
