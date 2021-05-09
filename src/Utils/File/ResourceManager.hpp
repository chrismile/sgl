/*!
 * ResourceManager.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef UTILS_FILE_RESOURCEMANAGER_HPP_
#define UTILS_FILE_RESOURCEMANAGER_HPP_

#include <map>
#include <memory>

//#include <System/Multithreading/MultithreadedQueue.hpp>
#include "ResourceBuffer.hpp"
#include <Utils/Singleton.hpp>

namespace sgl {

const uint32_t RESOURCE_LOADED_ASYNC_EVENT = 1041457103U;

class ResourceLoadingProcess;
class DLL_OBJECT ResourceManager : public Singleton<ResourceManager>
{
friend class ResourceLoadingProcess;
public:
    //! Interface
    //! Loads the resource from the hard-drive
    ResourceBufferPtr getFileSync(const char *filename);
    //! Returns empty buffer; RESOURCE_LOADED_ASYNC_EVENT is triggered when the file was loaded
    //ResourceBufferPtr getFileAsync(const char *filename);

private:
    //! Internal interface for querying already loaded files
    ResourceBufferPtr getResourcePointer(const char *filename);

    //! Internes Laden der Daten
    bool loadFile(const char *filename, ResourceBufferPtr &resource);

    std::map<std::string, std::weak_ptr<ResourceBuffer>> resourceFiles;
    //MultithreadedQueue<std::pair<std::string, std::shared_ptr<ResourceBuffer>>> queue;
};

}

/*! UTILS_FILE_RESOURCEMANAGER_HPP_ */
#endif
