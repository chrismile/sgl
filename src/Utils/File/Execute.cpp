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

#include "Execute.hpp"
#include <Utils/File/Logfile.hpp>
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#endif

namespace sgl {

char **convertStringListToArgv(std::list<std::string> &stringList) {
    char **argv = new char*[stringList.size() + 1];
    int i = 0;
    for (auto& it : stringList) {
        argv[i] = new char[it.size() + 1];
#if (defined(_MSC_VER) && _MSC_VER > 1910) || defined(__STDC_LIB_EXT1__)
        strcpy_s(argv[i], it.size() + 1, it.c_str());
#else
        strcpy(argv[i], it.c_str());
#endif
        ++i;
    }
    argv[stringList.size()] = nullptr;
    return argv;
}

void deleteArgv(char **argv) {
    int i = 0;
    while (argv[i] != nullptr)
    {
        delete[] (argv[i]);
        ++i;
    }
    delete[] argv;
}

std::string convertStringListToString(std::list<std::string> &stringList) {
    std::string args;
    for (auto& it : stringList) {
        args += it + ' ';
    }
    return args;
}


int executeProgram(const char *appName, std::list<std::string> &args) {
    char **argv = convertStringListToArgv(args);
#ifdef _WIN32
#if defined(_MSC_VER) && _MSC_VER > 1910
    intptr_t success = _spawnv(P_WAIT, appName, argv);
#else
    intptr_t success = spawnv(P_WAIT, appName, argv);
#endif
    if (success != 0) {
        std::string command = convertStringListToString(args);
        Logfile::get()->writeError(std::string() + command);
    }
    deleteArgv(argv);

    return int(success);
#else
    pid_t pid;
    switch (pid = fork()) {
    case -1:
        Logfile::get()->writeError("Error using fork()");
        deleteArgv(argv);
        return -1;
        break;
    case 0:
        execv(appName, argv);
        break;
    default:
        int status;
        if (wait(&status) != pid) {
            Logfile::get()->writeError("Error using wait()");
            deleteArgv(argv);
            return -1;
        }
        //int success = WEXITSTATUS(status);
    }
    deleteArgv(argv);
#endif
    return 0;
}

#ifndef _WIN32
std::string exec(const char* command) {
    char *buffer = new char[256];
    std::string output;
    FILE *pipe = popen(command, "r");
    if (!pipe)
        throw std::runtime_error("popen() failed");
    while (!feof(pipe)) {
        if (fgets(buffer, 256, pipe) != nullptr)
            output += buffer;
    }
    pclose(pipe);
    delete[] buffer;
    return output;
}
#else
std::string exec(const char* command) {
    return "";
}
#endif

}
