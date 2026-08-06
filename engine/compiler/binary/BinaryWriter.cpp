#include "BinaryWriter.h"

#include <cstring>
#include <limits>
#include <ostream>

namespace flx::binary
{
    static_assert(sizeof(float) == 4, "FLX binary requires 32-bit floats");
    static_assert(std::numeric_limits<float>::is_iec559, "FLX binary requires IEEE-754 floats");

    BinaryWriter::BinaryWriter(std::ostream& output)
        : output(output)
    {
    }

    void BinaryWriter::writeU8(uint8_t value)
    {
        writeBytes(&value, sizeof(value), "");
    }

    void BinaryWriter::writeU32(uint32_t value)
    {
        const uint8_t bytes[4] = {
            static_cast<uint8_t>(value & 0xffu),
            static_cast<uint8_t>((value >> 8) & 0xffu),
            static_cast<uint8_t>((value >> 16) & 0xffu),
            static_cast<uint8_t>((value >> 24) & 0xffu)
        };

        writeBytes(bytes, sizeof(bytes), "");
    }

    void BinaryWriter::writeI32(int32_t value)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        writeU32(bits);
    }

    void BinaryWriter::writeF32(float value)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        writeU32(bits);
    }

    void BinaryWriter::writeBool(bool value)
    {
        writeU8(value ? 1u : 0u);
    }

    void BinaryWriter::writeString(const std::string& value)
    {
        if (value.size() > MaxStringSize ||
            value.size() > std::numeric_limits<uint32_t>::max())
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project string limit exceeded",
                "string"
            );
        }

        accountStringBytes(
            value.size(),
            "string"
        );

        writeU32(static_cast<uint32_t>(value.size()));

        if (!value.empty())
        {
            writeBytes(
                value.data(),
                value.size(),
                "string"
            );
        }
    }

    void BinaryWriter::writeCount(
        size_t value,
        uint32_t limit,
        const std::string& field
    )
    {
        if (value > limit || value > std::numeric_limits<uint32_t>::max())
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project limit exceeded",
                field
            );
        }

        accountElements(
            value,
            field
        );

        writeU32(static_cast<uint32_t>(value));
    }

    void BinaryWriter::accountResource(const std::string& field)
    {
        accountElements(
            1,
            field
        );
    }

    void BinaryWriter::accountElements(
        size_t count,
        const std::string& field
    )
    {
        if (count > MaxTotalDecodedElements ||
            totalElements > MaxTotalDecodedElements - count)
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project element budget exceeded",
                field
            );
        }

        totalElements +=
            static_cast<uint32_t>(count);
    }

    void BinaryWriter::accountStringBytes(
        size_t count,
        const std::string& field
    )
    {
        if (count > MaxTotalDecodedStringBytes ||
            totalStringBytes > MaxTotalDecodedStringBytes - count)
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project string budget exceeded",
                field
            );
        }

        totalStringBytes +=
            static_cast<uint32_t>(count);
    }

    void BinaryWriter::writeBytes(
        const void* data,
        size_t size,
        const std::string& field
    )
    {
        output.write(
            static_cast<const char*>(data),
            static_cast<std::streamsize>(size)
        );

        if (!output)
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectWriteFailed,
                "Compiled project file could not be written",
                field
            );
        }
    }
}
