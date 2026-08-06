#pragma once

#include "BinaryException.h"
#include "BinaryLimits.h"

#include <cstdint>
#include <iosfwd>
#include <string>

namespace flx::binary
{
    class BinaryReader
    {
    public:
        explicit BinaryReader(std::istream& input);

        uint8_t readU8(const std::string& field = "");
        uint32_t readU32(const std::string& field = "");
        int32_t readI32(const std::string& field = "");
        float readF32(const std::string& field = "");
        bool readBool(const std::string& field = "");
        std::string readString(const std::string& field = "");
        uint32_t readCount(
            uint32_t limit,
            const std::string& field
        );
        bool hasTrailingData();

    private:
        void readBytes(
            void* data,
            size_t size,
            const std::string& field
        );

        std::istream& input;
    };
}
