/*
 * ResourceManager.cpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#include "ResourceManager.hpp"
#include "ResourceBuffer.hpp"
#include <Utils/File/FileUtils.hpp>
#include <fstream>
#include <boost/shared_ptr.hpp>

namespace sgl {

ResourceBufferPtr ResourceManager::getFileSync(const char *filename)
{
	ResourceBufferPtr resource = getResourcePointer(filename);

	// Is the file already loaded?
	if (resource)
		return resource;

	// Load the resource on this thread otherwise
	if (FileUtils::get()->exists(filename) && !FileUtils::get()->isDirectory(filename)) {
		bool loaded = loadFile(filename, resource);
		if (!loaded) {
			return ResourceBufferPtr();
		}
		resourceFiles.insert(std::make_pair(filename, resource));
	}

	return resource;
}

bool ResourceManager::loadFile(const char *filename, ResourceBufferPtr &resource)
{
	//assert(resource.get() && "ResourceManager::loadFile: resource.get()");
	std::streampos size;
	std::ifstream file(filename, std::ios::in|std::ios::binary|std::ios::ate);
	if (file.is_open()) {
		size = file.tellg();
		resource = ResourceBufferPtr(new ResourceBuffer(size));
		file.seekg(0, std::ios::beg);
		file.read(resource->getBuffer(), size);
		file.close();
		//resource->setIsLoaded();
		return true;
	}
	return false;
}

/*ResourceBufferPtr ResourceManager::getFileAsync(const char *filename)
{
	ResourceBufferPtr resource;
	ResourceManager::get()->queue.push(make_pair(filename, resource));
	resourceFiles.insert(make_pair(filename, resource));
	return ResourceBufferPtr();
}*/

ResourceBufferPtr ResourceManager::getResourcePointer(const char *filename)
{
	auto it = resourceFiles.find(filename);

	// If the file isn't loaded yet
	if (it == resourceFiles.end() || it->second.expired())
	{
		// Erase the file if it isn't referenced anymore
		if (it != resourceFiles.end())
			resourceFiles.erase(it);

		// Return an empty pointer, as the file still needs to be loaded
		return ResourceBufferPtr();
	}

	return it->second.lock();
}

}
