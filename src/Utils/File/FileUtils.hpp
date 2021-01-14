/*!
 * FileUtils.hpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
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
    void initialize(const std::string &_titleName, int argc, char *argv[]);
    inline const std::string &getTitleName() { return titleName; }
    inline int get_argc() { return argc; }
    inline char **get_argv() { return argv; }

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
    char **argv;
    /// Name of the application
    std::string titleName;
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
