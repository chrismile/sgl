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
