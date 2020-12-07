/*
 * Logfile.cpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#include "Logfile.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <Utils/Convert.hpp>
#ifdef __unix__
#include <Utils/File/Execute.hpp>
#endif

namespace sgl {

Logfile::Logfile () : closedLogfile(false)
{
}

Logfile::~Logfile ()
{
    closeLogfile();
}

void Logfile::closeLogfile()
{
    if (closedLogfile) {
        std::cerr << "Tried to close logfile multiple times!" << std::endl;
        return;
    }
    write("<br><br>End of file</font></body></html>");
    logfile.close();
    closedLogfile = true;
}

void Logfile::createLogfile (const char *filename, const char *appName)
{
    // Open file and write header
    logfile.open(filename);
    write(std::string() + "<html><head><title>Logfile (" + appName + ")</title></head>");
    write("<body><font face='courier new'>");
    writeTopic(std::string() + "Logfile (" + appName + ")", 2);

    // Log information on build configuration
#ifdef _DEBUG
    std::string build = "DEBUG";
#else
    std::string build = "RELEASE";
#endif
    write(std::string() + "Build: " + build + "<br>");

#ifdef __unix__
    std::string sysinfo = exec("uname -a");
    write(std::string() + "System info: " + sysinfo + "<br>");
#endif

    // Write link to project page
    write("<br><a href='https://github.com/chrismile/sgl/issues'>Inform the developers about issues</a><br><br>");
}

// Create the heading
void Logfile::writeTopic (const std::string &text, int size)
{
    write("<table width='100%%' ");
    write("bgcolor='#E0E0E5'><tr><td><font face='arial' ");
    write(std::string() + "size='+" + toString(size) + "'>");
    write(text);
    write("</font></td></tr></table>\n<br>");
}

// Write black text to the file
void Logfile::write(const std::string &text)
{
    logfile.write(text.c_str(), text.size());
    logfile.flush();
}

// Write colored text to the logfile
void Logfile::write(const std::string &text, int color)
{
    switch (color) {
    case BLACK:
        write("<font color=black>");  break;
    case WHITE:
        write("<font color=white>");  break;
    case RED:
        write("<font color=red>");    break;
    case GREEN:
        write("<font color=green>");  break;
    case BLUE:
        write("<font color=blue>");   break;
    case PURPLE:
        write("<font color=purple>"); break;
    case ORANGE:
        write("<font color=FF6A00>"); break;
    };

    write(text);
    write("</font>");
    write("<br>");
}

void Logfile::writeError(const std::string &text)
{
    std::cerr << text << std::endl;
    write(text, RED);
}

void Logfile::writeInfo(const std::string &text)
{
    std::cout << text << std::endl;
    write(text, BLUE);
}

}
