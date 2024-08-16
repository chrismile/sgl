/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#include <string>
#include <iostream>
#include <fstream>

#ifdef USE_TBB
#if __has_include(<tbb/version.h>)
#include <tbb/version.h>
#else
#include <tbb/tbb_stddef.h>
#endif
#elif defined(_OPENMP)
#include <omp.h>
#endif

#include <Utils/Convert.hpp>
#include <Utils/Dialog.hpp>
#ifdef __unix__
#include <Utils/File/Execute.hpp>
#endif

#include "../StringUtils.hpp"
#include "Logfile.hpp"

namespace sgl {

Logfile::Logfile () : closedLogfile(false) {
}

Logfile::~Logfile () {
    closeLogfile();
}

void Logfile::closeLogfile() {
    if (closedLogfile) {
        std::cerr << "Tried to close logfile multiple times!" << std::endl;
        return;
    }
    write("<br><br>End of file.</font></body></html>");
    logfile.close();
    closedLogfile = true;
}

void Logfile::createLogfile(const std::string& filename, const std::string& appName) {
    // Open the file and write the header.
    logfile.open(filename);
    write(std::string() + "<html><head><title>Logfile (" + appName + ")</title></head>");
    write("<body><font face='courier new'>");
    writeTopic(std::string() + "Logfile (" + appName + ")", 2);

    // Log information on build configuration
#ifdef _DEBUG
    std::string build = "Debug";
#else
    std::string build = "Release";
#endif
    write(std::string() + "Build type: " + build + "<br>");

#if defined(_WIN32)
    write("Operating system: Windows<br>");
#elif defined(__linux__)
    write("Operating system: Linux<br>");
#elif defined(__unix__)
    write("Operating system: Unix<br>");
#else
    write("Operating system: Unknown<br>");
#endif

#if defined(__MINGW32__)
    write("Compiler: MinGW<br>");
#elif defined(_MSC_VER)
    write("Compiler: MSVC<br>");
#elif defined(__GNUC__)
    write("Compiler: GCC<br>");
#elif defined(__clang__)
    write("Compiler: Clang<br>");
#else
    write("Compiler: Unknown<br>");
#endif

#ifdef USE_TBB
    write(
            "Threading library: TBB (" + std::to_string(TBB_VERSION_MAJOR) + "."
            + std::to_string(TBB_VERSION_MINOR) + ")<br>");
#elif _OPENMP
    write(
            "Threading library: OpenMP (" + std::to_string(_OPENMP) + "), "
            + std::to_string(omp_get_max_threads()) + " threads<br>");
#else
    write("Threading library: None<br>");
#endif

#if defined(__unix__) && !defined(__EMSCRIPTEN__)
    std::string sysinfo = exec("uname -a");
    write(std::string() + "System info: " + sysinfo + "<br>");
#endif

    // Write a link to the issues section of the project.
    write("<br><a href='https://github.com/chrismile/" + appName + "/issues'>Inform the developers about issues</a><br><br>");
}

// Writes the header.
void Logfile::writeTopic (const std::string &text, int size) {
    write("<table width='100%%' ");
    write("bgcolor='#E0E0E5'><tr><td><font face='arial' ");
    write(std::string() + "size='+" + toString(size) + "'>");
    write(text);
    write("</font></td></tr></table>\n<br>");
}

// Writes black text to the file.
void Logfile::write(const std::string &text) {
    logfile.write(text.c_str(), text.size());
    logfile.flush();
}

// Writes colored text to the logfile.
void Logfile::write(const std::string &text, int color) {
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
        write("<font color=FF9200>"); break;
    };

    write(text);
    write("</font>");
    write("<br>");
}

void Logfile::writeWarning(const std::string &text, bool openMessageBox) {
    std::cerr << text << std::endl;
    write(text, ORANGE);
    if (openMessageBox) {
        dialog::openMessageBox("Warning", text, dialog::Icon::WARNING);
    }
}

void Logfile::writeError(const std::string &text, bool openMessageBox) {
    std::cerr << text << std::endl;
    write(text, RED);
    if (openMessageBox) {
        dialog::openMessageBoxBlocking("Error occurred", text, dialog::Icon::ERROR);
    }
}

void Logfile::writeErrorMultiline(const std::string &text, bool openMessageBox) {
    std::cerr << text << std::endl;
    std::string textHtml = sgl::stringReplaceAllCopy(text, "\n", "<br>\n");
    write(textHtml, RED);
    if (openMessageBox) {
        dialog::openMessageBoxBlocking("Error occurred", text, dialog::Icon::ERROR);
    }
}

void Logfile::throwError(const std::string &text, bool openMessageBox) {
    write(text, RED);
    if (openMessageBox) {
        dialog::openMessageBoxBlocking("Fatal error occurred", text, dialog::Icon::ERROR);
    }
    throw std::runtime_error(text);
}

void Logfile::writeInfo(const std::string &text) {
    std::cout << text << std::endl;
    write(text, BLUE);
}

}
