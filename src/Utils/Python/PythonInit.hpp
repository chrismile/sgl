/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
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

#ifndef SGL_PYTHONINIT_HPP
#define SGL_PYTHONINIT_HPP

#include <string>
#include <cstring>

#if defined(PYTHONHOME_PATH) || defined(__APPLE__)
#include <cstdlib>
#endif
#include <Python.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>

namespace sgl {

// PySys_SetArgv, Py_SetPythonHome and Py_SetPath were deprecated in version 3.11.
// Now we use the init config struct as outlined in: https://docs.python.org/3/c-api/init_config.html#init-config
#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11)
#define USE_PY_INIT_CONFIG
#endif

/// Include before main function.
static void pythonInit(int argc, char *argv[]) {
#ifdef USE_PY_INIT_CONFIG
    PyStatus status;
    PyConfig config{};
    PyConfig_InitPythonConfig(&config);

    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        sgl::Logfile::get()->throwError("Fatal error: Py_InitializeFromConfig failed.");
    }
#endif

#ifdef PYTHONHOME
#ifdef _MSC_VER
    char* pythonhomeEnvVar = nullptr;
    size_t stringSize = 0;
    if (_dupenv_s(&pythonhomeEnvVar, &stringSize, "PYTHONHOME") != 0) {
        pythonhomeEnvVar = nullptr;
    }
#else // !defined(_MSC_VER)
    const char* pythonhomeEnvVar = getenv("PYTHONHOME");
#endif // _MSC_VER
    if (!pythonhomeEnvVar || strlen(pythonhomeEnvVar) == 0) {
#if !defined(__APPLE__) || !defined(PYTHONPATH)
#ifdef USE_PY_INIT_CONFIG
        config.isolated = 1;
        PyConfig_SetString(&config, &config.home, PYTHONHOME);
#else
        Py_SetPythonHome(PYTHONHOME);
#endif
#endif // !defined(__APPLE__) || !defined(PYTHONPATH)
        // As of 2022-01-25, "lib-dynload" is not automatically found when using MSYS2 together with MinGW.
#if (defined(__MINGW32__) || defined(__APPLE__)) && defined(PYTHONPATH)
#ifdef __MINGW32__
#ifdef USE_PY_INIT_CONFIG
        PyWideStringList_Append(&config.module_search_paths, PYTHONPATH ";" PYTHONPATH "/site-packages;" PYTHONPATH "/lib-dynload");
#else
        Py_SetPath(PYTHONPATH ";" PYTHONPATH "/site-packages;" PYTHONPATH "/lib-dynload");
#endif
#else // !defined(__MINGW32__)
        std::wstring pythonhomeWide = PYTHONHOME;
        std::string pythonhomeNormal(pythonhomeWide.size(), ' ');
        pythonhomeNormal.resize(std::wcstombs(
                &pythonhomeNormal[0], pythonhomeWide.c_str(), pythonhomeWide.size()));
        std::wstring pythonpathWide = PYTHONPATH;
        std::string pythonpathNormal(pythonpathWide.size(), ' ');
        pythonpathNormal.resize(std::wcstombs(
                &pythonpathNormal[0], pythonpathWide.c_str(), pythonpathWide.size()));
        if (!sgl::FileUtils::get()->exists(pythonhomeNormal)) {
            uint32_t pathBufferSize = 0;
            _NSGetExecutablePath(nullptr, &pathBufferSize);
            char* pathBuffer = new char[pathBufferSize];
            _NSGetExecutablePath(pathBuffer, &pathBufferSize);
            std::string executablePythonHome =
                    sgl::FileUtils::get()->getPathToFile(std::string() + pathBuffer) + "python3";
            if (sgl::FileUtils::get()->exists(executablePythonHome)) {
                std::wstring pythonHomeLocal(executablePythonHome.size(), L' ');
                pythonHomeLocal.resize(std::mbstowcs(
                        &pythonHomeLocal[0], executablePythonHome.c_str(), executablePythonHome.size()));
                std::string pythonVersionString = sgl::FileUtils::get()->getPathAsList(pythonpathNormal).back();
                 std::wstring pythonVersionStringWide(pythonVersionString.size(), L' ');
                pythonVersionStringWide.resize(std::mbstowcs(
                        &pythonVersionStringWide[0], pythonVersionString.c_str(), pythonVersionString.size()));
               std::wstring pythonPathLocal =
                        pythonHomeLocal + L"/lib/" + pythonVersionStringWide;
                std::wstring inputPath =
                        pythonPathLocal + L":"
                        + pythonPathLocal + L"/site-packages:"
                        + pythonPathLocal + L"/lib-dynload";
#ifdef USE_PY_INIT_CONFIG
                config.isolated = 1;
                PyConfig_SetString(&config, &config.home, pythonHomeLocal.c_str());
                //PyConfig_SetBytesString(&config, &config.home, pythonHomeLocal.c_str());
                PyWideStringList_Append(&config.module_search_paths, inputPath.c_str());
#else
                Py_SetPythonHome(pythonHomeLocal.c_str());
                Py_SetPath(inputPath.c_str());
#endif
            } else {
                sgl::Logfile::get()->throwError("Fatal error: Couldn't find Python home.");
            }
            delete[] pathBuffer;
        } else {
#ifdef USE_PY_INIT_CONFIG
            config.isolated = 1;
            PyConfig_SetString(&config, &config.home, PYTHONHOME);
            Py_SetPath(PYTHONPATH ":" PYTHONPATH "/site-packages:" PYTHONPATH "/lib-dynload");
#else
            Py_SetPythonHome(PYTHONHOME);
            PyWideStringList_Append(&config.module_search_paths, PYTHONPATH ":" PYTHONPATH "/site-packages:" PYTHONPATH "/lib-dynload");
#endif
        }
#endif // __MINGW32__
#endif // (defined(__MINGW32__) || defined(__APPLE__)) && defined(PYTHONPATH)
    }
#ifdef _MSC_VER
    free(pythonhomeEnvVar);
    pythonhomeEnvVar = nullptr;
#endif
#endif // PYTHONHOME

#ifdef USE_PY_INIT_CONFIG
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        sgl::Logfile::get()->throwError("Fatal error: Py_InitializeFromConfig failed.");
    }
#else // !defined(USE_PY_INIT_CONFIG)
    // PyMem_Malloc causes segmentation fault on Fedora 39.
    //wchar_t** argvWidestr = (wchar_t**)PyMem_Malloc(sizeof(wchar_t*) * argc);
    wchar_t** argvWidestr = (wchar_t**)malloc(sizeof(wchar_t*) * argc);
    for (int i = 0; i < argc; i++) {
        wchar_t* argWidestr = Py_DecodeLocale(argv[i], nullptr);
        argvWidestr[i] = argWidestr;
    }
    Py_Initialize();
    PySys_SetArgv(argc, argvWidestr);
#endif
}

}

#endif //SGL_PYTHONINIT_HPP
