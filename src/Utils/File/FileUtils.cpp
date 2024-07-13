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

#include <algorithm>
#include <fstream>
#include <cstdlib>

#include <filesystem>

#ifdef _WIN32
#define _WIN32_IE 0x0400
#include <shlobj.h>
#include <windef.h>
#endif

#ifdef USE_BOOST_LOCALE
#include <boost/locale.hpp>
#include <boost/locale/collator.hpp>
#endif

#include <Utils/StringUtils.hpp>
#include <Utils/File/Logfile.hpp>

#include "FileUtils.hpp"

namespace sgl {

void FileUtils::initialize(const std::string &_appName, int _argc, char *_argv[]) {
    initialize(_appName, _argc, const_cast<const char**>(_argv));
}

void FileUtils::initialize(const std::string &_appName, int _argc, const char *_argv[]) {
    argc = _argc;
    argv = _argv;
    execPath = std::filesystem::absolute(argv[0]).string();
    execDir = std::filesystem::path(execPath).parent_path().string() + "/";

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

    std::string userConfigDir = homeDirectory + "/.config/";
    configDir = userConfigDir + sgl::toLowerCopy(appNameNoWhitespace) + "/";
    userDir = homeDirectory + "/";
    if (!exists(userConfigDir)) {
        createDirectory(userConfigDir);
    }

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
    std::string lowerFilename = sgl::toLowerCopy(std::string(fileString));
    return sgl::endsWith(lowerFilename, extension);
}

std::string FileUtils::filenameWithoutExtension(const std::string &filename) {
    size_t dotPos = filename.find_last_of(".");
    return filename.substr(0, dotPos);
}

std::string FileUtils::getPureFilename(const std::string &path) {
    for (int i = (int)path.size() - 2; i >= 0; --i) {
        if (path.at(i) == '/' || path.at(i) == '\\') {
            return path.substr(i+1, path.size()-i-1);
        }
    }
    return path;
}

std::string FileUtils::removeExtension(const std::string &path) {
    for (int i = (int)path.size() - 1; i >= 0; --i) {
        if (path.at(i) == '.') {
            return path.substr(0, i);
        }
    }
    return path;
}

std::string FileUtils::getFileExtension(const std::string &path) {
    auto dotLocation = path.find_last_of('.');
    if (dotLocation != std::string::npos) {
        return path.substr(dotLocation + 1);
    }
    return "";
}

std::string FileUtils::getFileExtensionLower(const std::string &path) {
    auto dotLocation = path.find_last_of('.');
    if (dotLocation != std::string::npos) {
        return sgl::toLowerCopy(path.substr(dotLocation + 1));
    }
    return "";
}

std::string FileUtils::getPathToFile(const std::string &path) {
    for (int i = int(path.size()) - 1; i >= 0; --i) {
        if (path.at(i) == '/' || path.at(i) == '\\') {
            return path.substr(0, i+1);
        }
    }
    return path;
}


std::list<std::string> FileUtils::getFilesInDirectoryList(const std::string &dirPath) {
    std::filesystem::path dir(dirPath);
    if (!std::filesystem::exists(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectory: Path \""
                + dir.string() + "\" does not exist!");
        return std::list<std::string>();
    }
    if (!std::filesystem::is_directory(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectory: \""
                + dir.string() + "\" is not a directory!");
        return std::list<std::string>();
    }

    std::list<std::string> files;
    std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator i(dir); i != end; ++i) {
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
    std::filesystem::path dir(dirPath);
    if (!std::filesystem::exists(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectoryAsVector: Path \""
                + dir.string() + "\" does not exist!");
        return std::vector<std::string>();
    }
    if (!std::filesystem::is_directory(dir)) {
        Logfile::get()->writeError(std::string() + "FileUtils::getFilesInDirectoryAsVector: \""
                + dir.string() + "\" is not a directory!");
        return std::vector<std::string>();
    }

    std::vector<std::string> files;
    std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator i(dir); i != end; ++i) {
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
    std::filesystem::path dir(dirPath);
    if (std::filesystem::is_directory(dir))
        return true;
    return false;
}

bool FileUtils::exists(const std::string &filePath) {
    std::filesystem::path dir(filePath);
    if (std::filesystem::exists(dir))
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
    std::filesystem::path dir(path);
    std::filesystem::create_directory(dir);
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
    std::filesystem::path file(filename);
    std::filesystem::path newFile(newFilename);
    std::filesystem::rename(file, newFilename);
}

bool FileUtils::removeFile(const std::string &filename) {
    std::filesystem::path file(filename);
    return std::filesystem::remove(filename);
}

bool FileUtils::removeAll(const std::string &filename) {
    std::filesystem::path file(filename);
    return std::filesystem::remove_all(filename);
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

    std::filesystem::path file(sourceFile);
    std::vector<std::string> filePath;
    splitPath(sourceFile, filePath);
    std::filesystem::path destination(destinationDirectory);
    if (!sgl::endsWith(destinationDirectory.c_str(), "/") || !sgl::endsWith(destinationDirectory.c_str(), "\\"))
        destination = std::filesystem::path(destinationDirectory + "/" + filePath.at(filePath.size()-1));
    else
        destination = std::filesystem::path(destinationDirectory + filePath.at(filePath.size()-1));
    std::filesystem::copy_file(file, destination);
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
            || sgl::startsWith(path, "/") || sgl::startsWith(path, "\\");
#else
    bool isAbsolutePath =
            sgl::startsWith(path, "/");
#endif
    return isAbsolutePath;
}

std::string FileUtils::getPathAbsolute(const std::string &path) {
    return std::filesystem::absolute(path).string();
}

std::string FileUtils::getPathAbsoluteGeneric(const std::string &path) {
    return std::filesystem::absolute(path).generic_string();
}

bool FileUtils::getPathAbsoluteEquivalent(const std::string &pathStr0, const std::string &pathStr1) {
    auto path0 = std::filesystem::absolute(pathStr0);
    auto path1 = std::filesystem::absolute(pathStr1);
    return std::filesystem::equivalent(path0, path1);
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
    std::filesystem::path path0(pathStr0);
    std::filesystem::path path1(pathStr1);
    return std::filesystem::equivalent(path0, path1);
}

struct CaseInsensitiveComparator {
    bool operator() (const std::string& lhs, const std::string& rhs) const {
        std::string lowerCaseStringLhs = sgl::toLowerCopy(lhs);
        std::string lowerCaseStringRhs = sgl::toLowerCopy(rhs);
        return lowerCaseStringLhs < lowerCaseStringRhs;
    }
};

void FileUtils::sortPathStrings(std::vector<std::string>& pathStrings) {
    // TODO: Replace with ICU.
#ifdef USE_BOOST_LOCALE
    std::sort(pathStrings.begin(), pathStrings.end(), boost::locale::comparator<char, boost::locale::collator_base::secondary>());
#else
    std::sort(pathStrings.begin(), pathStrings.end(), CaseInsensitiveComparator());
#endif
}

}
