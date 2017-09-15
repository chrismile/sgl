/*
 * FileManager.hpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph
 */

#ifndef UTILS_FILE_FILEMANAGER_HPP_
#define UTILS_FILE_FILEMANAGER_HPP_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace sgl {

template <class AssetType, class AssetInfo>
class FileManager
{
public:
	virtual ~FileManager() {}
	typedef boost::shared_ptr<AssetType> AssetPtr;
	typedef boost::weak_ptr<AssetType> WeakAssetPtr;
	AssetPtr getAsset(AssetInfo &assetInfo);

protected:
	virtual AssetPtr loadAsset(AssetInfo &assetInfo)=0;
	std::map<AssetInfo, WeakAssetPtr> assetMap;
};

template <class AssetType, class AssetInfo>
boost::shared_ptr<AssetType> FileManager<AssetType, AssetInfo>::getAsset(AssetInfo &assetInfo)
{
	auto it = assetMap.find(assetInfo);

	// Do we need to (re-)load the asset?
	if (it == assetMap.end() || it->second._empty() || it->second.expired()) {
		boost::shared_ptr<AssetType> asset = loadAsset(assetInfo);
		assetMap[assetInfo] = boost::weak_ptr<AssetType>(asset);
		return asset;
	}

	return it->second.lock();
}

}


#endif /* UTILS_FILE_FILEMANAGER_HPP_ */
