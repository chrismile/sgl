/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#ifndef SGL_PATHWATCH_HPP
#define SGL_PATHWATCH_HPP

#include <string>
#include <functional>

#include <boost/algorithm/string/predicate.hpp>

#include <Utils/File/FileUtils.hpp>

namespace sgl {

struct PathWatchImplData;

/**
 * Watches a file or directory inside some parent directory for changes.
 */
class DLL_OBJECT PathWatch {
public:
    ~PathWatch();

    /**
     * Examples:
     * - setDirectories("Data/TransferFunctions/multivar/", true)
     * - setDirectories("Data/DataSets/datasets.json", false)
     * - setDirectories("Data/TransferFunctions/multivar/", true)
     *
     * Getting and watching the parent directory is needed, as we need to also be notified if the watched path node is
     * deleted and then recreated.
     *
     * @param path The path of the file or directory to watch (the name, NOT the path).
     * @param isFolder Is 'path' pointing to a folder or file?
     */
    void setPath(const std::string& _path, bool _isFolder) {
        isFolder = _isFolder;
        path = _path;
        if (isFolder && !boost::ends_with(path, "/") && !boost::ends_with(path, "\\")) {
            path = path + "/";
        }
        std::vector<std::string> pathList = sgl::FileUtils::get()->getPathAsList(path);
        if (pathList.size() > 1) {
            parentDirectoryPath = "";
            for (size_t i = 0; i < pathList.size() - 1; i++) {
                if (i == 0 && pathList.at(0) == "/") {
                    parentDirectoryPath += "/";
                } else {
                    parentDirectoryPath += pathList.at(i) + "/";
                }
            }
        } else {
            parentDirectoryPath = ".";
        }
        watchedNodeName = pathList.back();
    }

    /// Initializes the watch.
    void initialize();

    /// Returns true when the directory content changed.
    void update(std::function<void()> pathChangedCallback);

protected:
    void _freeInternal();
    bool isFolder = false;
    std::string path; // E.g., "Data/TransferFunctions/multivar/" or "Data/DataSets/datasets.json" or "data.xml".
    std::string parentDirectoryPath; // E.g., "Data/TransferFunctions/" or "Data/DataSets/" or ".".
    std::string watchedNodeName; // E.g., "multivar", "datasets.json" or "data.xml".
    PathWatchImplData* data = nullptr;
};

}

#endif //SGL_PATHWATCH_HPP
