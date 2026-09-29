#pragma once

#include "BinaryException.h"
#include "BinaryLimits.h"

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>

namespace flx::binary
{
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(std::ostream& output);

        void writeU8(uint8_t value);
        void writeU32(uint32_t value);
        void writeI32(int32_t value);
        void writeF32(float value);
        void writeBool(bool value);
        void writeString(const std::string& value);
        void writeCount(
            size_t value,
            uint32_t limit,
            const std::string& field
        );
        void accountResource(const std::string& field);

    private:
        void accountElements(
            size_t count,
            const std::string& field
        );
        void accountStringBytes(
            size_t count,
            const std::string& field
        );
        void writeBytes(
            const void* data,
            size_t size,
            const std::string& field
        );

        std::ostream& output;
        uint32_t totalElements = 0;
        uint32_t totalStringBytes = 0;
    };
}
