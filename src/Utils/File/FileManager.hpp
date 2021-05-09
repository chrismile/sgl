/*!
 * FileManager.hpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef UTILS_FILE_FILEMANAGER_HPP_
#define UTILS_FILE_FILEMANAGER_HPP_

#include <string>
#include <map>
#include <memory>

namespace sgl {

template <class AssetType, class AssetInfo>
class FileManager
{
public:
    virtual ~FileManager() {}
    typedef std::shared_ptr<AssetType> AssetPtr;
    typedef std::weak_ptr<AssetType> WeakAssetPtr;
    AssetPtr getAsset(AssetInfo &assetInfo);

protected:
    virtual AssetPtr loadAsset(AssetInfo &assetInfo)=0;
    std::map<AssetInfo, WeakAssetPtr> assetMap;
};

template <class AssetType, class AssetInfo>
std::shared_ptr<AssetType> FileManager<AssetType, AssetInfo>::getAsset(AssetInfo &assetInfo)
{
    auto it = assetMap.find(assetInfo);

    //! Do we need to (re-)load the asset?
    if (it == assetMap.end() || it->second.expired()) {
        std::shared_ptr<AssetType> asset = loadAsset(assetInfo);
        assetMap[assetInfo] = std::weak_ptr<AssetType>(asset);
        return asset;
    }

    return it->second.lock();
}

}

/*! UTILS_FILE_FILEMANAGER_HPP_ */
#endif
