#pragma once

#include "../runtime/BitmapDefinition.h"
#include <map>
#include <memory>

// Internal GPU cache owned by a world, never by the compiled project.
class BitmapTextureCache
{
public:
    BitmapTextureCache() = default;
    BitmapTextureCache(const BitmapTextureCache&) = delete;
    BitmapTextureCache& operator=(const BitmapTextureCache&) = delete;
    ~BitmapTextureCache();
    Texture2D get(const std::shared_ptr<const BitmapDefinition>& bitmap);
    void clear();

private:
    using Key = std::weak_ptr<const BitmapDefinition>;
    std::map<Key, Texture2D, std::owner_less<Key>> textures;
};
