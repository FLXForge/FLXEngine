#include "BitmapTextureCache.h"
#include <stdexcept>

BitmapTextureCache::~BitmapTextureCache() { clear(); }

void BitmapTextureCache::clear()
{
    if (IsWindowReady())
        for (const auto& entry : textures) UnloadTexture(entry.second);
    textures.clear();
}

Texture2D BitmapTextureCache::get(const std::shared_ptr<const BitmapDefinition>& bitmap)
{
    if (!bitmap || !bitmap->valid() || !IsWindowReady())
        throw std::runtime_error("Cannot draw bitmap without valid pixels and a graphics context");
    const Key key(bitmap);
    const auto found = textures.find(key);
    if (found != textures.end()) return found->second;
    // Prune on cache misses only; a warm frame performs just one lookup.
    for (auto it = textures.begin(); it != textures.end();)
    {
        if (it->first.expired())
        {
            UnloadTexture(it->second);
            it = textures.erase(it);
        }
        else ++it;
    }
    // Borrow CPU storage for this upload only; raylib does not own this Image.
    const Image image{const_cast<Color*>(bitmap->pixels.data()), bitmap->width,
        bitmap->height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    const Texture2D texture = LoadTextureFromImage(image);
    if (!texture.id) throw std::runtime_error("Bitmap texture initialization failed");
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    textures.emplace(key, texture);
    return texture;
}
