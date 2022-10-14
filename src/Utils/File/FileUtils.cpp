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

#define _FILE_OFFSET_BITS 64

#include "FileUtils.hpp"
#include <fstream>
#include <cstdlib>

#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#ifdef _WIN32
#define _WIN32_IE 0x0400
#include <shlobj.h>
#include <windef.h>
#endif

#include <Utils/File/Logfile.hpp>
#include <boost/algorithm/string.hpp>

#ifdef USE_BOOST_LOCALE
#include <boost/locale.hpp>
#include <boost/locale/collator.hpp>
#endif

namespace sgl {

void FileUtils::initialize(const std::string &_appName, int _argc, char *_argv[]) {
    initialize(_appName, _argc, const_cast<const char**>(_argv));
}

void FileUtils::initialize(const std::string &_appName, int _argc, const char *_argv[]) {
    argc = _argc;
    argv = _argv;
    execPath = boost::filesystem::absolute(argv[0]).string();
    execDir = boost::filesystem::path(execPath).parent_path().string() + "/";

    appName = _appName;
#if defined(__unix__) || defined(__APPLE__)
    std::string homeDirectory = getenv("HOME");

#ifndef __APPLE__
    std::string appNameNoWhitespace;
    for (char c : appName) {
        if (c != ' ' && c != '\t') {
            appNameNoWhitespace += c;
        } else {
            appNameNoWhitespace += '-';
        }
    }

    configDir = homeDirectory + "/.config/" + boost::to_lower_copy(appNameNoWhitespace) + "/";
    userDir = homeDirectory + "/";

    // Use the system-wide path "/var/games" if it is available on the system
    if (exists("/var/games")) {
        sharedDir = "/var/games/";
    } else {
        sharedDir = getConfigDirectory();
    }
#else
    std::string appNameNoWhitespace;
    for (char c : appName) {
        if (c != ' ' && c != '\t') {
            appNameNoWhitespace += c;
        }
    }

    configDir = homeDirectory + "/Library/Preferences/" + appNameNoWhitespace + "/";
    userDir = homeDirectory + "/";
    sharedDir = "/Library/Preferences/" + appNameNoWhitespace + "/";
#endif
#else
    std::string appNameNoWhitespace;
    for (char c : appName) {
        if (c != ' ' && c != '\t') {
            appNameNoWhitespace += c;
        }
    }
    // Use WinAPI to query the AppData folder
    char userDataPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, userDataPath, CSIDL_APPDATA, true);
    std::string dir = std::string() + userDataPath + "/";
    for (std::string::iterator it = dir.begin(); it != dir.end(); ++it) {
        if (*it == '\\') *it = '/';
    }
    configDir = dir + appNameNoWhitespace + "/";

    // For now the configuration directory is also the shared storage.
    // Using the Windows Registry could be a potential better choice.
    sharedDir = getConfigDirectory();

    // Now query the user directory
    char userDirPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, userDirPath, CSIDL_PROFILE, true);
    dir = std::string() + userDirPath + "/";
    for (std::string::iterator it = dir.begin(); it != dir.end(); ++it) {
        if (*it == '\\') *it = '/';
    }
    userDir = dir;
#endif

    // Create the usage directory on first use/after deletion
    if (!exists(getConfigDirectory())) {
        createDirectory(getConfigDirectory());
    }
}

// Check whether file has certain extension
bool FileUtils::hasExtension(const char *fileString, const char *extension) {
    std::string lowerFilename = boost::to_lower_copy(std::string(fileString));
    return boost::ends_with(lowerFilename, extension);
}

std::string FileUtils::filenameWithoutExtension(const std::string &filename) {
    size_t dotPos = filename.find_last_of(".");
    return filename.substr(0, dotPos);
}

// /home/user/Info.txt -> Info.txt
std::string FileUtils::getPureFilename(const std::string &path) {
    for (int i = (int)path.size() - 2; i >= 0; --i) {
        if (path.at(i) == '/' || path.at(i) == '\\') {
            return path.substr(i+1, path.size()-i-1);
        }
    }
    return path;
}

// Info.txt -> Info
std::string FileUtils::removeExtension(const std::string &path) {
    for (int i = (int)path.size() - 1; i >= 0; --i) {
        if (path.at(i) == '.') {
            return path.substr(0, i);
        }
    }
    return path;
}

// "/home/user/Info.txt" -> "/home/user/"
std::string FileUtils::getPathToFile(const std::string &path) {
    for (int i = int(path.size()) - 1; i >= 0; --i) {
        if (path.at(i) == '/' || path.at(i) == '\\') {
            return path.substr(0, i+1);
        }
    }
    return path;
}


std::list<std::string> FileUtils::getFilesInDirectoryList(const std::string &dirPath) {
    boost::filesystem::path dir(dirPath);
    if (!boost::filesystem::exists(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectory: Path \""
                + dir.string() + "\" does not exist!");
        return std::list<std::string>();
    }
    if (!boost::filesystem::is_directory(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectory: \""
                + dir.string() + "\" is not a directory!");
        return std::list<std::string>();
    }

    std::list<std::string> files;
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator i(dir); i != end; ++i) {
        files.push_back(i->path().string());
#ifdef _WIN32
        std::string &currPath = files.back();
        for (std::string::iterator it = currPath.begin(); it != currPath.end(); ++it) {
            if (*it == '\\') *it = '/';
        }
#endif
    }
    return files;
}


std::vector<std::string> FileUtils::getFilesInDirectoryVector(const std::string &dirPath) {
    boost::filesystem::path dir(dirPath);
    if (!boost::filesystem::exists(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectoryAsVector: Path \""
                + dir.string() + "\" does not exist!");
        return std::vector<std::string>();
    }
    if (!boost::filesystem::is_directory(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectoryAsVector: \""
                + dir.string() + "\" is not a directory!");
        return std::vector<std::string>();
    }

    std::vector<std::string> files;
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator i(dir); i != end; ++i) {
        files.push_back(i->path().string());
#ifdef _WIN32
        std::string &currPath = files.back();
        for (std::string::iterator it = currPath.begin(); it != currPath.end(); ++it) {
            if (*it == '\\') *it = '/';
        }
#endif
    }
    return files;
}


std::vector<std::string> FileUtils::getPathAsList(const std::string &dirPath) {
    std::vector<std::string> files;
    if (dirPath.size() == 0)
        return files;
#ifndef _WIN32
    // Starts with root directory?
    if (dirPath.at(0) == '/')
        files.push_back("/");
#endif
    std::string tempString;
    for (size_t i = 0; i < dirPath.size(); ++i) {
        if (dirPath.at(i) == '/' || dirPath.at(i) == '\\') {
            if (tempString != "") {
                files.push_back(tempString);
                tempString = "";
            }
        } else {
            tempString += dirPath.at(i);
        }
    }

    if (tempString != "") {
        files.push_back(tempString);
        tempString = "";
    }

    return files;
}


bool FileUtils::isDirectory(const std::string &dirPath) {
    boost::filesystem::path dir(dirPath);
    if (boost::filesystem::is_directory(dir))
        return true;
    return false;
}

bool FileUtils::exists(const std::string &filePath) {
    boost::filesystem::path dir(filePath);
    if (boost::filesystem::exists(dir))
        return true;
    return false;
}

bool FileUtils::directoryExists(const std::string &dirPath) {
    return isDirectory(dirPath) && exists(dirPath);
}


void FileUtils::deleteFileEnding(std::string &path) {
    if (path.size() != 0)
    {
        for (int i = int(path.size()) - 1; i >= 0; --i)
        {
            if (path.at(i) == '.')
            {
                path = path.substr(0, i);
                break;
            }
        }
    }
}


void FileUtils::createDirectory(const std::string &path) {
    boost::filesystem::path dir(path);
    boost::filesystem::create_directory(dir);
}


void FileUtils::ensureDirectoryExists(const std::string &path) {
    std::vector<std::string> directories;
    splitPathNoTrim(path, directories);
    std::string currentDirectory;

    for (size_t i = 0; i < directories.size(); ++i)
    {
        currentDirectory = currentDirectory + directories.at(i) + "/";
        if (i == 0) {
#ifdef _WIN32
            // Do we have a drive letter?
            if (directories.at(i).size() == 2 && directories.at(i).at(1) == ':')
                continue;
#else
            // Is this the root directory?
            if (directories.at(i).empty())
                continue;
#endif
        }
        if (!exists(currentDirectory))
            createDirectory(currentDirectory);
    }
}

void FileUtils::rename(const std::string &filename, const std::string &newFilename) {
    boost::filesystem::path file(filename);
    boost::filesystem::path newFile(newFilename);
    boost::filesystem::rename(file, newFilename);
}

bool FileUtils::removeFile(const std::string &filename) {
    boost::filesystem::path file(filename);
    return boost::filesystem::remove(filename);
}

bool FileUtils::removeAll(const std::string &filename) {
    boost::filesystem::path file(filename);
    return boost::filesystem::remove_all(filename);
}

void FileUtils::copyFileToDirectory(const std::string &sourceFile, const std::string &destinationDirectory) {
    if (!exists(sourceFile)) {
        std::string errorString = std::string() + "FileUtils::CopyFileToDirectory: File to copy (\" +" + sourceFile + "+ \") doesn't exist!";
        Logfile::get()->writeError(std::string() + "FileUtils::CopyFileToDirectory: File to copy (\""
                + destinationDirectory + "\") does not exist!");
        return;
    }
    if (!exists(destinationDirectory)) {
        Logfile::get()->writeError(std::string() + "FileUtils::CopyFileToDirectory: Destination directory \""
                + destinationDirectory + "\" does not exist!");
        return;
    }

    boost::filesystem::path file(sourceFile);
    std::vector<std::string> filePath;
    splitPath(sourceFile, filePath);
    boost::filesystem::path destination(destinationDirectory);
    if (!boost::ends_with(destinationDirectory.c_str(), "/") || !boost::ends_with(destinationDirectory.c_str(), "\\"))
        destination = boost::filesystem::path(destinationDirectory + "/" + filePath.at(filePath.size()-1));
    else
        destination = boost::filesystem::path(destinationDirectory + filePath.at(filePath.size()-1));
    boost::filesystem::copy_file(file, destination);
}

void FileUtils::splitPath(const std::string &path, std::list<std::string> &pathList) {
    std::string::const_iterator it;
    std::string buffer = "";
    for (it = path.begin(); it != path.end(); it++) {
        if (*it != '/' && *it != '\\') {
            buffer += *it;
        } else {
            if (buffer != "") {
                pathList.push_back(buffer);
                buffer = "";
            }
        }
    }
    if (buffer != "") {
        pathList.push_back(buffer);
    }
}

void FileUtils::splitPath(const std::string &path, std::vector<std::string> &pathList) {
    std::string::const_iterator it;
    std::string buffer = "";
    for (it = path.begin(); it != path.end(); it++) {
        if (*it != '/' && *it != '\\') {
            buffer += *it;
        } else {
            if (buffer != "") {
                pathList.push_back(buffer);
                buffer = "";
            }
        }
    }
    if (buffer != "") {
        pathList.push_back(buffer);
    }
}

void FileUtils::splitPathNoTrim(const std::string &path, std::vector<std::string> &pathList) {
    std::string::const_iterator it;
    std::string buffer = "";
    for (it = path.begin(); it != path.end(); it++) {
        if (*it != '/' && *it != '\\') {
            buffer += *it;
        } else {
            pathList.push_back(buffer);
            buffer = "";
        }
    }
    if (buffer != "") {
        pathList.push_back(buffer);
    }
}

bool FileUtils::getIsPathAbsolute(const std::string &path) {
#ifdef _WIN32
    bool isAbsolutePath =
            (path.size() > 1 && path.at(1) == ':')
            || boost::starts_with(path, "/") || boost::starts_with(path, "\\");
#else
    bool isAbsolutePath =
            boost::starts_with(path, "/");
#endif
    return isAbsolutePath;
}

size_t FileUtils::getFileSizeInBytes(const std::string &path) {
#if defined(__linux__) || defined(__MINGW32__) // __GNUC__? Does GCC generally work on non-POSIX systems?
    FILE* file = fopen64(path.c_str(), "rb");
#else
    FILE* file = fopen(path.c_str(), "rb");
#endif
    if (!file) {
        sgl::Logfile::get()->writeError(
                std::string() + "Error in FileUtils::getFileSizeInBytes: File \""
                + path + "\" could not be opened.");
        return 0;
    }
#if defined(_WIN32) && !defined(__MINGW32__)
    _fseeki64(file, 0, SEEK_END);
    size_t bufferSize = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);
#else
    fseeko(file, 0, SEEK_END);
    size_t bufferSize = ftello(file);
    fseeko(file, 0, SEEK_SET);
#endif
    fclose(file);
    return bufferSize;
}

bool FileUtils::pathsEquivalent(const std::string &pathStr0, const std::string &pathStr1) {
    boost::filesystem::path path0(pathStr0);
    boost::filesystem::path path1(pathStr1);
    return boost::filesystem::equivalent(path0, path1);
}

struct CaseInsensitiveComparator {
    bool operator() (const std::string& lhs, const std::string& rhs) const {
        std::string lowerCaseStringLhs = boost::to_lower_copy(lhs);
        std::string lowerCaseStringRhs = boost::to_lower_copy(rhs);
        return lowerCaseStringLhs < lowerCaseStringRhs;
    }
};

void FileUtils::sortPathStrings(std::vector<std::string>& pathStrings) {
#ifdef USE_BOOST_LOCALE
    std::sort(pathStrings.begin(), pathStrings.end(), boost::locale::comparator<char, boost::locale::collator_base::secondary>());
#else
    std::sort(pathStrings.begin(), pathStrings.end(), CaseInsensitiveComparator());
#endif
}

}
