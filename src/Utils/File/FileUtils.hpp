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

#ifndef SRC_UTILS_FILE_FILEUTILS_HPP_
#define SRC_UTILS_FILE_FILEUTILS_HPP_

#include <Defs.hpp>
#include <Utils/Singleton.hpp>
#include <string>
#include <vector>
#include <list>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace sgl {

class DLL_OBJECT FileUtils : public Singleton<FileUtils>
{
public:
    /// Title name is the name of the application and argc/argv are the arguments passed to the main function
    void initialize(const std::string &_appName, int argc, char *argv[]);
    void initialize(const std::string &_appName, int argc, const char *argv[]);
    inline const std::string &getAppName() { return appName; }
    [[nodiscard]] inline int get_argc() const { return argc; }
    inline const char **get_argv() { return argv; }

    /// Directory containing the application
    inline std::string getExecutableDirectory() { return execDir + "/"; }
    /// Directory the app may write to, e.g. a folder in AppData (Windows) or in .config (Linux)
    inline std::string getConfigDirectory() { return configDir; }
    /// Directory of the user, e.g. C:/Users/<Name> (Windows) or /home/<Name> (Linux)
    inline std::string getUserDirectory() { return userDir; }
    /// Directoy available for all users on the system, e.g. /var/games (Linux) or just the configDir (Windows)
    inline std::string getSharedDirectory() { return sharedDir; }

    /// Check whether file has certain extension
    bool hasExtension(const char *fileString, const char *extension);
    std::string filenameWithoutExtension(const std::string &filename);
    /// "/home/user/Info.txt" -> "Info.txt"
    std::string getPureFilename(const std::string &path);
    /// "Info.txt" -> "Info"
    std::string removeExtension(const std::string &path);
    /// "/home/user/Info.txt" -> "/home/user/"
    std::string getPathToFile(const std::string &path);

    std::list<std::string> getFilesInDirectoryList(const std::string &dirPath);
    std::vector<std::string> getFilesInDirectoryVector(const std::string &dirPath);
    std::vector<std::string> getPathAsList(const std::string &dirPath);
    bool isDirectory(const std::string &dirPath);
    bool exists(const std::string &filePath);
    bool directoryExists(const std::string &dirPath);
    void deleteFileEnding(std::string &path);
    void createDirectory (const std::string &path);
    void ensureDirectoryExists(const std::string &directory);
    /// Rename a file - you have to pass full paths twice!
    void rename(const std::string &filename, const std::string &newFilename);
    /// Delete file; returns success
    bool removeFile(const std::string &filename);
    /// Delete file or directory inclusive all directories; returns success
    bool removeAll(const std::string &filename);
    void copyFileToDirectory(const std::string &sourceFile, const std::string &destinationDirectory);
    void splitPath(const std::string &path, std::list<std::string> &list);
    void splitPath(const std::string &path, std::vector<std::string> &list);

    // Do the two paths point to the same resource?
    bool pathsEquivalent(const std::string &pathStr0, const std::string &pathStr1);

    // Sorts the array of path strings in a case insensitive way.
    void sortPathStrings(std::vector<std::string>& pathStrings);

private:
    int argc;
    const char **argv;
    /// Name of the application
    std::string appName;
    /// Directory containing the application
    std::string execDir;
    /// Directory the app may write to, e.g. a folder in AppData (Windows) or .config (Linux)
    std::string configDir;
    /// Directory of the user, e.g. C:/Users/<Name> (Windows) or /home/<Name> (Linux)
    std::string userDir;
    /// Directoy available for all users on the system, e.g. /var/games (Linux) or just the configDir (Windows)
    std::string sharedDir;
};

}

/*! SRC_UTILS_FILE_FILEUTILS_HPP_ */
#endif
