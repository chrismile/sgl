/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#include "RemoteUtils.hpp"

#ifdef __linux__

#include <fstream>
#include <cstdlib>
#include <dirent.h>

#include <Utils/StringUtils.hpp>
#include <Utils/File/Execute.hpp>

/**
 * Returns the PID of a process with the passed name.
 * If no process with this name is running, -1 is returned.
 * Code from: https://stackoverflow.com/questions/45037193/how-to-check-if-a-process-is-running-in-c
 */
int getProcIdByName(const std::string& procName) {
    int pid = -1;
    DIR* dp = opendir("/proc");
    if (dp) {
        struct dirent* dirp;
        while (pid < 0 && (dirp = readdir(dp))) {
            int id = std::atoi(dirp->d_name);
            if (id > 0) {
                std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty()) {
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos) {
                        cmdLine = cmdLine.substr(0, pos);
                    }
                    pos = cmdLine.rfind('/');
                    if (pos != std::string::npos) {
                        cmdLine = cmdLine.substr(pos + 1);
                    }
                    if (procName == cmdLine) {
                        pid = id;
                    }
                }
            }
        }
    }
    closedir(dp);
    return pid;
}

bool guessUseDownloadSwapchain() {
    // Heuristic #1: xdpyinfo contains the string "VNC" or "vnc".
    std::string xdpyinfoOutput = sgl::exec("xdpyinfo 2>&1");
    bool isVncUsed = xdpyinfoOutput.find("vnc") != std::string::npos || xdpyinfoOutput.find("VNC") != std::string::npos;

    // Heuristic #2: A non-standard X11 display is used.
    const char* displayVar = getenv("DISPLAY");
    bool isNonStandardDisplay = displayVar && !sgl::startsWith(displayVar, ":0");

    // Heuristic #3: x11vnc is not running.
    bool x11vncUsed = getProcIdByName("x11vnc") >= 0;

    return isVncUsed || (isNonStandardDisplay && !x11vncUsed);
}

#else

bool guessUseDownloadSwapchain() {
    return false;
}

#endif
