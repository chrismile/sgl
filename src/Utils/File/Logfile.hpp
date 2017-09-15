/*
 * Logfile.hpp
 *
 *  Created on: 27.08.2017
 *      Author: christoph
 */

#ifndef SRC_UTILS_FILE_LOGFILE_HPP_
#define SRC_UTILS_FILE_LOGFILE_HPP_

#include <fstream>
#include <Utils/Singleton.hpp>

namespace sgl {

// Colors for the output text
enum FONTCOLORS {
	BLACK, WHITE, RED, GREEN, BLUE, PURPLE, ORANGE
};

class Logfile : public Singleton<Logfile>
{
public:
	Logfile();
	~Logfile();
	void createLogfile(const char *filename, const char *appName);
	void closeLogfile();

	// Write to Logfile
	void writeTopic(const std::string &text, int size);
	void write(const std::string &text);
	void write(const std::string &text, int color);
	void writeError(const std::string &text); // Outputs text on stderr, too
	void writeInfo(const std::string &text); // Outputs text on stdout, too

private:
	bool closedLogfile;
	std::ofstream logfile;
};

}

#endif /* SRC_UTILS_FILE_LOGFILE_HPP_ */
