#pragma once

#include <cstdint>

namespace flx::binary
{
    constexpr uint32_t MaxBinaryFileSize =
        64u * 1024u * 1024u;

    constexpr uint32_t MaxStringSize =
        32u * 1024u * 1024u;

    constexpr uint32_t MaxTotalDecodedElements =
        50000u;

    constexpr uint32_t MaxTotalDecodedStringBytes =
        16u * 1024u * 1024u;

    constexpr uint32_t MaxResourceCount =
        100000u;

    constexpr uint32_t MaxCollectionCount =
        100000u;

    constexpr uint32_t MaxPointCount =
        100000u;

    constexpr uint32_t MaxMusicChannelCount =
        64u;

    constexpr uint32_t MaxGridRowCount =
        10000u;
}
