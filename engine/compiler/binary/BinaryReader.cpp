#include "BinaryReader.h"

#include <cstring>
#include <istream>
#include <limits>

namespace flx::binary
{
    static_assert(sizeof(float) == 4, "FLX binary requires 32-bit floats");
    static_assert(std::numeric_limits<float>::is_iec559, "FLX binary requires IEEE-754 floats");

    BinaryReader::BinaryReader(std::istream& input)
        : input(input)
    {
    }

    uint8_t BinaryReader::readU8(const std::string& field)
    {
        uint8_t value = 0;
        readBytes(&value, sizeof(value), field);
        return value;
    }

    uint32_t BinaryReader::readU32(const std::string& field)
    {
        uint8_t bytes[4] = {};
        readBytes(bytes, sizeof(bytes), field);

        return
            static_cast<uint32_t>(bytes[0]) |
            (static_cast<uint32_t>(bytes[1]) << 8) |
            (static_cast<uint32_t>(bytes[2]) << 16) |
            (static_cast<uint32_t>(bytes[3]) << 24);
    }

    int32_t BinaryReader::readI32(const std::string& field)
    {
        const uint32_t bits =
            readU32(field);

        int32_t value = 0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    float BinaryReader::readF32(const std::string& field)
    {
        const uint32_t bits =
            readU32(field);

        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    bool BinaryReader::readBool(const std::string& field)
    {
        const uint8_t value =
            readU8(field);

        if (value == 0u)
        {
            return false;
        }

        if (value == 1u)
        {
            return true;
        }

        throw BinaryException(
            DiagnosticCode::InvalidCompiledProjectValue,
            "Invalid compiled project boolean value",
            field
        );
    }

    std::string BinaryReader::readString(const std::string& field)
    {
        const uint32_t size =
            readU32(field);

        if (size > MaxStringSize)
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project string limit exceeded",
                field
            );
        }

        accountStringBytes(
            size,
            field
        );

        std::string value(size, '\0');

        if (size > 0)
        {
            readBytes(
                value.data(),
                size,
                field
            );
        }

        return value;
    }

    uint32_t BinaryReader::readCount(
        uint32_t limit,
        const std::string& field
    )
    {
        const uint32_t value =
            readU32(field);

        if (value > limit)
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

        return value;
    }

    void BinaryReader::accountResource(const std::string& field)
    {
        accountElements(
            1,
            field
        );
    }

    bool BinaryReader::hasTrailingData()
    {
        input.peek();

        if (input.eof())
        {
            return false;
        }

        return true;
    }

    void BinaryReader::accountElements(
        uint32_t count,
        const std::string& field
    )
    {
        if (count > MaxTotalDecodedElements ||
            totalElements > MaxTotalDecodedElements - count)
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project decoded element budget exceeded",
                field
            );
        }

        totalElements +=
            count;
    }

    void BinaryReader::accountStringBytes(
        uint32_t count,
        const std::string& field
    )
    {
        if (count > MaxTotalDecodedStringBytes ||
            totalStringBytes > MaxTotalDecodedStringBytes - count)
        {
            throw BinaryException(
                DiagnosticCode::CompiledProjectLimitExceeded,
                "Compiled project decoded string budget exceeded",
                field
            );
        }

        totalStringBytes +=
            count;
    }

    void BinaryReader::readBytes(
        void* data,
        size_t size,
        const std::string& field
    )
    {
        input.read(
            static_cast<char*>(data),
            static_cast<std::streamsize>(size)
        );

        if (!input)
        {
            throw BinaryException(
                DiagnosticCode::TruncatedCompiledProject,
                "Compiled project file is truncated",
                field
            );
        }
    }
}
