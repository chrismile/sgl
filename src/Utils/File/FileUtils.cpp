/*
 * FileUtils.cpp
 *
 *  Created on: 10.09.2017
 *      Author: christoph
 */

#include "FileUtils.hpp"
#include <fstream>
#include <cstdlib>

#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#ifdef WIN32
#define _WIN32_IE 0x0400
#include <shlobj.h>
#include <windef.h>
#endif

#include <Utils/File/Logfile.hpp>
#include <boost/algorithm/string.hpp>

namespace sgl {

void FileUtils::initialize(const std::string &_titleName, int _argc, char *_argv[])
{
	argc = _argc;
	argv = _argv;
	execDir = argv[0];

	titleName = _titleName;
#ifdef __unix__
	configDir = std::string() + getenv("HOME") + "/.config/" + boost::to_lower_copy(titleName) + "/";
	userDir = std::string() + getenv("HOME") + "/";

	// Use the system-wide path "/var/games" if it is available on the system
	if (exists("/var/games")) {
		sharedDir = "/var/games/";
	} else {
		sharedDir = getConfigDirectory();
	}
#else
	// Use WinAPI to query the AppData folder
	char userDataPath[MAX_PATH];
	SHGetSpecialFolderPathA(NULL, userDataPath, CSIDL_APPDATA, true);
	std::string dir = std::string() + userDataPath + "/";
	for (std::string::iterator it = dir.begin(); it != dir.end(); ++it) {
		if (*it == '\\') *it = '/';
	}
	configDir = dir + titleName + "/";

	// For now the usage storage is also the shared storage.
	// Using the Windows Registry could be a potential better choice.
	sharedDir = getUsageStorage();

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
bool FileUtils::hasExtension(const char *fileString, const char *extension)
{
	std::string lowerFilename = boost::to_lower_copy(std::string(fileString));
	return boost::ends_with(lowerFilename, extension);
}

std::string FileUtils::filenameWithoutExtension(const std::string &filename)
{
	size_t dotPos = filename.find_last_of(".");
	return filename.substr(0, dotPos);
}

// /home/user/Info.txt -> Info.txt
std::string FileUtils::getPureFilename(const std::string &path)
{
	for (int i = path.size()-2; i >= 0; --i) {
		if (path.at(i) == '/' || path.at(i) == '\\') {
			return path.substr(i+1, path.size()-i-1);
		}
	}
	return path;
}

// Info.txt -> Info
std::string FileUtils::removeExtension(const std::string &path)
{
	for (int i = path.size()-1; i >= 0; --i) {
		if (path.at(i) == '.') {
			return path.substr(0, i);
		}
	}
	return path;
}

std::list<std::string> FileUtils::getFilesInDirectoryList(const std::string &dirPath)
{
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
#ifdef WIN32
		std::string &currPath = files.back();
		for (std::string::iterator it = currPath.begin(); it != currPath.end(); ++it) {
			if (*it == '\\') *it = '/';
		}
#endif
	}
	return files;
}


std::vector<std::string> FileUtils::getFilesInDirectoryVector(const std::string &dirPath)
{
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
#ifdef WIN32
		std::string &currPath = files.back();
		for (std::string::iterator it = currPath.begin(); it != currPath.end(); ++it) {
			if (*it == '\\') *it = '/';
		}
#endif
	}
	return files;
}


std::vector<std::string> FileUtils::getPathAsList(const std::string &dirPath)
{
	std::vector<std::string> files;
	if (dirPath.size() == 0)
		return files;
#ifndef WIN32
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


bool FileUtils::isDirectory(const std::string &dirPath)
{
	boost::filesystem::path dir(dirPath);
	if (boost::filesystem::is_directory(dir))
		return true;
	return false;
}

bool FileUtils::exists(const std::string &filePath)
{
	boost::filesystem::path dir(filePath);
	if (boost::filesystem::exists(dir))
		return true;
	return false;
}


void FileUtils::deleteFileEnding(std::string &path)
{
	if (path.size() != 0)
	{
		for (int i = path.size()-1; i >= 0; --i)
		{
			if (path.at(i) == '.')
			{
				path = path.substr(0, i);
				break;
			}
		}
	}
}


void FileUtils::createDirectory(const std::string &path)
{
	boost::filesystem::path dir(path);
	boost::filesystem::create_directory(dir);
}


void FileUtils::ensureDirectoryExists(const std::string &path)
{
	std::vector<std::string> directories;
	splitPath(path, directories);
	std::string currentDirectory;

	for (size_t i = 0; i < directories.size(); ++i)
	{
		currentDirectory = currentDirectory + directories.at(i) + "/";
		if (i == 0) {
#ifdef WIN32
			// Handelt es sich um einen Laufwerksbuchstaben?
			if (directories.at(i).size() == 2 && directories.at(i).at(1) == ':')
				continue;
#else
			// Handelt es sich um das Root-Directory?
			if (directories.at(i).size() == 0)
				continue;
#endif
		}
		if (!exists(currentDirectory))
			createDirectory(currentDirectory);
	}
}

void FileUtils::rename(const std::string &filename, const std::string &newFilename)
{
	boost::filesystem::path file(filename);
	boost::filesystem::path newFile(newFilename);
	boost::filesystem::rename(file, newFilename);
}

bool FileUtils::removeFile(const std::string &filename)
{
	boost::filesystem::path file(filename);
	return boost::filesystem::remove(filename);
}

bool FileUtils::removeAll(const std::string &filename)
{
	boost::filesystem::path file(filename);
	return boost::filesystem::remove_all(filename);
}

void FileUtils::copyFileToDirectory(const std::string &sourceFile, const std::string &destinationDirectory)
{
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

void FileUtils::splitPath(const std::string &path, std::list<std::string> &pathList)
{
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
		buffer = "";
	}
}

void FileUtils::splitPath(const std::string &path, std::vector<std::string> &pathList)
{
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
		buffer = "";
	}
}

}
