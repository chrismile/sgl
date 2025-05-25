/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#ifndef UTILS_FILE_RESOURCEMANAGER_HPP_
#define UTILS_FILE_RESOURCEMANAGER_HPP_

#include <string>
#include <map>
#include <memory>

//#include <System/Multithreading/MultithreadedQueue.hpp>
#include "ResourceBuffer.hpp"
#include <Utils/Singleton.hpp>

namespace sgl {

const uint32_t RESOURCE_LOADED_ASYNC_EVENT = 1041457103U;

class ResourceLoadingProcess;
class DLL_OBJECT ResourceManager : public Singleton<ResourceManager> {
friend class ResourceLoadingProcess;
public:
    /// Interface
    /// Loads the resource from the hard-drive
    ResourceBufferPtr getFileSync(const char *filename);
    /// Returns empty buffer; RESOURCE_LOADED_ASYNC_EVENT is triggered when the file was loaded
    //ResourceBufferPtr getFileAsync(const char *filename);

private:
    /// Internal interface for querying already loaded files
    ResourceBufferPtr getResourcePointer(const char *filename);

    /// Internes Laden der Daten
    bool loadFile(const char *filename, ResourceBufferPtr &resource);

    std::map<std::string, std::weak_ptr<ResourceBuffer>> resourceFiles;
    //MultithreadedQueue<std::pair<std::string, std::shared_ptr<ResourceBuffer>>> queue;
};

}

/*! UTILS_FILE_RESOURCEMANAGER_HPP_ */
#endif
