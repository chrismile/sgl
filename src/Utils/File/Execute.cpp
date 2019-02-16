/*
 * Execute.cpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
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
    char **argv = new char*[stringList.size()+1];
    int i = 0;
    for (std::list<std::string>::iterator it = stringList.begin(); it != stringList.end(); ++it)
    {
        argv[i] = new char[it->size()+1];
        strcpy(argv[i], it->c_str());
        ++i;
    }
    argv[stringList.size()] = NULL;
    return argv;
}

void deleteArgv(char **argv) {
    int i = 0;
    while (argv[i] != NULL)
    {
        delete[] (argv[i]);
        ++i;
    }
    delete[] argv;
}

std::string convertStringListToString(std::list<std::string> &stringList) {
    std::string args;
    for (std::list<std::string>::iterator it = stringList.begin(); it != stringList.end(); ++it)
    {
        args += *it + ' ';
    }
    return args;
}


int executeProgram(const char *appName, std::list<std::string> &args) {
    char **argv = convertStringListToArgv(args);
#ifdef _WIN32
    int success = spawnv(P_WAIT, appName, argv);
    if (success != 0) {
        std::string command = convertStringListToString(args);
        Logfile::get()->writeError(std::string() + command);
    }
    deleteArgv(argv);

    return success;
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
    char *buffer = new char[128];
    std::string output;
    FILE *pipe = popen(command, "r");
    if (!pipe)
        throw std::runtime_error("popen() failed");
    while (!feof(pipe)) {
        if (fgets(buffer, 256, pipe) != NULL)
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
