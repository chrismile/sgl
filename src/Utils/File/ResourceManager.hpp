/*
 * ResourceManager.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef UTILS_FILE_RESOURCEMANAGER_HPP_
#define UTILS_FILE_RESOURCEMANAGER_HPP_

//#include <System/Multithreading/MultithreadedQueue.hpp>
#include "ResourceBuffer.hpp"
#include <Utils/Singleton.hpp>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace sgl {

const uint32_t RESOURCE_LOADED_ASYNC_EVENT = 1041457103U;

class ResourceLoadingProcess;
class ResourceManager : public Singleton<ResourceManager>
{
friend class ResourceLoadingProcess;
public:
	// Interface
	ResourceBufferPtr getFileSync(const char *filename); // Loads the resource from the hard-drive
	//ResourceBufferPtr getFileAsync(const char *filename); // Returns empty buffer; RESOURCE_LOADED_ASYNC_EVENT is triggered when the file was loaded

private:
	// Internal interface for querying already loaded files
	ResourceBufferPtr getResourcePointer(const char *filename);

	// Internes Laden der Daten
	bool loadFile(const char *filename, ResourceBufferPtr &resource);

	std::map< std::string, boost::weak_ptr<ResourceBuffer> > resourceFiles;
	//MultithreadedQueue< std::pair<std::string, boost::shared_ptr<ResourceBuffer>> > queue;
};

}

#endif /* UTILS_FILE_RESOURCEMANAGER_HPP_ */
