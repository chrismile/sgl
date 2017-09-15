/*
 * ResourceBuffer.hpp
 *
 *  Created on: 13.01.2015
 *      Author: Christoph
 */

#ifndef UTILS_FILE_RESOURCEBUFFER_HPP_
#define UTILS_FILE_RESOURCEBUFFER_HPP_

#include <boost/shared_ptr.hpp>
#include <atomic>

namespace sgl {

class ResourceBuffer
{
public:
	ResourceBuffer(size_t size) : bufferSize(size), loaded(false) { data = new char[bufferSize]; }
	~ResourceBuffer() { if (data) { delete[] data; data = NULL; } }
	inline char *getBuffer() { return data; }
	inline const char *getBuffer() const { return data; }
	inline size_t getBufferSize() { return bufferSize; }
	inline bool getIsLoaded() { return loaded; }
	inline void setIsLoaded() { loaded = true; }

private:
	char *data;
	size_t bufferSize;
	std::atomic<bool> loaded; // For asynchronously loaded resources
	boost::shared_ptr<ResourceBuffer> parentZipFileResource; // optional!
};

typedef boost::shared_ptr<ResourceBuffer> ResourceBufferPtr;

}

#endif /* UTILS_FILE_RESOURCEBUFFER_HPP_ */
