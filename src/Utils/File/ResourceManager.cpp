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

#include "ResourceManager.hpp"
#include "ResourceBuffer.hpp"
#include <Utils/File/FileUtils.hpp>
#include <fstream>
#include <memory>
#include <boost/shared_ptr.hpp>

namespace sgl {

ResourceBufferPtr ResourceManager::getFileSync(const char *filename) {
    ResourceBufferPtr resource = getResourcePointer(filename);

    // Is the file already loaded?
    if (resource)
        return resource;

    // Load the resource on this thread otherwise
    if (FileUtils::get()->exists(filename) && !FileUtils::get()->isDirectory(filename)) {
        bool loaded = loadFile(filename, resource);
        if (!loaded) {
            return {};
        }
        resourceFiles.insert(std::make_pair(filename, resource));
    }

    return resource;
}

bool ResourceManager::loadFile(const char *filename, ResourceBufferPtr &resource) {
    //assert(resource.get() && "ResourceManager::loadFile: resource.get()");
    std::streampos size;
    std::ifstream file(filename, std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open()) {
        size = file.tellg();
        resource = std::make_shared<ResourceBuffer>(size);
        file.seekg(0, std::ios::beg);
        file.read(resource->getBuffer(), size);
        file.close();
        //resource->setIsLoaded();
        return true;
    }
    return false;
}

/*ResourceBufferPtr ResourceManager::getFileAsync(const char *filename) {
    ResourceBufferPtr resource;
    ResourceManager::get()->queue.push(make_pair(filename, resource));
    resourceFiles.insert(make_pair(filename, resource));
    return ResourceBufferPtr();
}*/

ResourceBufferPtr ResourceManager::getResourcePointer(const char *filename) {
    auto it = resourceFiles.find(filename);

    // If the file isn't loaded yet
    if (it == resourceFiles.end() || it->second.expired())
    {
        // Erase the file if it isn't referenced anymore
        if (it != resourceFiles.end())
            resourceFiles.erase(it);

        // Return an empty pointer, as the file still needs to be loaded
        return {};
    }

    return it->second.lock();
}

}
