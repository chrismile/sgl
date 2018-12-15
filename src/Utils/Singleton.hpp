/*!
 * Singleton.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_UTILS_SINGLETON_HPP_
#define SRC_UTILS_SINGLETON_HPP_

#include <memory>

#ifdef _WIN32
#include <boost/interprocess/detail/intermodule_singleton.hpp>
#endif

namespace sgl {

#ifdef _WIN32

template <class T>
class Singleton : public boost::interprocess::ipcdetail::intermodule_singleton<T>
{
public:
	/*virtual ~Singleton () { }
	inline static void deleteSingleton() { singleton = std::unique_ptr<T>(); }*/

	//! Creates static instance if necessary and returns the pointer to it
	inline static T *get()
	{
		return &boost::interprocess::ipcdetail::intermodule_singleton<T>::get();
	}
};

#else

//! Singleton instance of classes T derived from Singleton<T> can be accessed using T::get().

template <class T>
class Singleton
{
public:
	virtual ~Singleton () { }
	inline static void deleteSingleton() { singleton = std::unique_ptr<T>(); }

	//! Creates static instance if necessary and returns the pointer to it
	inline static T *get()
	{
		if (!singleton.get())
			singleton = std::unique_ptr<T>(new T);
		return singleton.get();
	}

protected:
	static std::unique_ptr<T> singleton;
};

template <class T>
std::unique_ptr<T> Singleton<T>::singleton;

#endif

}

/*! SRC_UTILS_SINGLETON_HPP_ */
#endif
