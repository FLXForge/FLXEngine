#include "PersistenceSystem.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    constexpr uint8_t ObfuscationKey = 0x5Au;
    constexpr uint32_t FormatVersion = 1u;
    constexpr char Magic[] = { 'F', 'L', 'X', 'S', 'A', 'V', 'E', '1' };

    std::filesystem::path saveDirectory()
    {
        return std::filesystem::current_path() / "saves";
    }

    bool isSafeName(
        const std::string& value
    )
    {
        if (value.empty())
        {
            return false;
        }

        return std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char c)
            {
                return std::isalnum(c) || c == '_' || c == '-';
            }
        );
    }

    std::filesystem::path savePath(
        const std::string& name
    )
    {
        return saveDirectory() / (name + ".flxsave");
    }

    uint32_t checksum(
        const std::vector<uint8_t>& bytes
    )
    {
        uint32_t result = 2166136261u;

        for (uint8_t byte : bytes)
        {
            result ^= byte;
            result *= 16777619u;
        }

        return result;
    }

    void writeU32(
        std::vector<uint8_t>& bytes,
        uint32_t value
    )
    {
        bytes.push_back(static_cast<uint8_t>(value & 0xffu));
        bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xffu));
        bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xffu));
        bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xffu));
    }

    void writeDouble(
        std::vector<uint8_t>& bytes,
        double value
    )
    {
        static_assert(sizeof(double) == 8);

        const uint8_t* raw =
            reinterpret_cast<const uint8_t*>(&value);

        bytes.insert(bytes.end(), raw, raw + sizeof(double));
    }

    bool readU32(
        const std::vector<uint8_t>& bytes,
        size_t& offset,
        uint32_t& value
    )
    {
        if (offset + 4 > bytes.size())
        {
            return false;
        }

        value =
            static_cast<uint32_t>(bytes[offset]) |
            (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
            (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
            (static_cast<uint32_t>(bytes[offset + 3]) << 24);

        offset += 4;

        return true;
    }

    bool readDouble(
        const std::vector<uint8_t>& bytes,
        size_t& offset,
        double& value
    )
    {
        if (offset + sizeof(double) > bytes.size())
        {
            return false;
        }

        std::copy_n(
            bytes.data() + offset,
            sizeof(double),
            reinterpret_cast<uint8_t*>(&value)
        );

        offset += sizeof(double);

        return true;
    }

    void writeString(
        std::vector<uint8_t>& bytes,
        const std::string& value
    )
    {
        writeU32(bytes, static_cast<uint32_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    }

    bool readString(
        const std::vector<uint8_t>& bytes,
        size_t& offset,
        std::string& value
    )
    {
        uint32_t size = 0;

        if (!readU32(bytes, offset, size))
        {
            return false;
        }

        if (offset + size > bytes.size())
        {
            return false;
        }

        value.assign(
            reinterpret_cast<const char*>(bytes.data() + offset),
            static_cast<size_t>(size)
        );

        offset += size;

        return true;
    }

    void obfuscate(
        std::vector<uint8_t>& bytes
    )
    {
        for (uint8_t& byte : bytes)
        {
            byte ^= ObfuscationKey;
        }
    }
}

bool PersistenceSystem::save(
    const std::string& name,
    const std::string& key,
    const PersistedValue& value
)
{
    if (!isSafeName(name) || !isSafeName(key))
    {
        Logger::warning(
            "save",
            "save() ignored invalid save name or key"
        );

        return false;
    }

    SaveMap values;

    if (std::filesystem::exists(savePath(name)))
    {
        std::optional<SaveMap> loaded =
            readFile(name);

        if (loaded.has_value())
        {
            values =
                std::move(loaded.value());
        }
        else
        {
            Logger::warning(
                "save",
                "Existing save file is corrupt; it will be replaced: " + name
            );
        }
    }

    values[key] =
        value;

    return writeFile(name, values);
}

std::optional<PersistedValue> PersistenceSystem::load(
    const std::string& name,
    const std::string& key
)
{
    if (!isSafeName(name) || !isSafeName(key))
    {
        Logger::warning(
            "save",
            "load() ignored invalid save name or key"
        );

        return std::nullopt;
    }

    if (!std::filesystem::exists(savePath(name)))
    {
        return std::nullopt;
    }

    std::optional<SaveMap> values =
        readFile(name);

    if (!values.has_value())
    {
        return std::nullopt;
    }

    const auto found =
        values->find(key);

    if (found == values->end())
    {
        return std::nullopt;
    }

    return found->second;
}

std::optional<PersistenceSystem::SaveMap> PersistenceSystem::readFile(
    const std::string& name
)
{
    const std::filesystem::path path =
        savePath(name);

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        Logger::warning(
            "save",
            "Save file could not be opened: " + path.string()
        );

        return std::nullopt;
    }

    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    if (bytes.size() < sizeof(Magic) + 4)
    {
        Logger::warning("save", "Save file is corrupt: " + name);
        return std::nullopt;
    }

    if (!std::equal(std::begin(Magic), std::end(Magic), bytes.begin()))
    {
        Logger::warning("save", "Save file has invalid signature: " + name);
        return std::nullopt;
    }

    size_t offset = sizeof(Magic);
    uint32_t storedChecksum = 0;

    if (!readU32(bytes, offset, storedChecksum))
    {
        Logger::warning("save", "Save file is corrupt: " + name);
        return std::nullopt;
    }

    std::vector<uint8_t> payload(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        bytes.end()
    );

    if (checksum(payload) != storedChecksum)
    {
        Logger::warning("save", "Save file checksum failed: " + name);
        return std::nullopt;
    }

    obfuscate(payload);

    size_t payloadOffset = 0;
    uint32_t version = 0;
    uint32_t count = 0;

    if (
        !readU32(payload, payloadOffset, version) ||
        !readU32(payload, payloadOffset, count) ||
        version != FormatVersion
    )
    {
        Logger::warning("save", "Save file version is invalid: " + name);
        return std::nullopt;
    }

    SaveMap values;

    for (uint32_t i = 0; i < count; ++i)
    {
        std::string key;
        uint32_t type = 0;

        if (
            !readString(payload, payloadOffset, key) ||
            !readU32(payload, payloadOffset, type)
        )
        {
            Logger::warning("save", "Save file entry is corrupt: " + name);
            return std::nullopt;
        }

        PersistedValue value;
        value.type =
            static_cast<PersistedValue::Type>(type);

        if (value.type == PersistedValue::Type::Boolean)
        {
            uint32_t storedBool = 0;

            if (!readU32(payload, payloadOffset, storedBool))
            {
                Logger::warning("save", "Save boolean is corrupt: " + name);
                return std::nullopt;
            }

            value.booleanValue =
                storedBool != 0;
        }
        else if (value.type == PersistedValue::Type::Number)
        {
            if (!readDouble(payload, payloadOffset, value.numberValue))
            {
                Logger::warning("save", "Save number is corrupt: " + name);
                return std::nullopt;
            }
        }
        else if (value.type == PersistedValue::Type::String)
        {
            if (!readString(payload, payloadOffset, value.stringValue))
            {
                Logger::warning("save", "Save string is corrupt: " + name);
                return std::nullopt;
            }
        }
        else
        {
            Logger::warning("save", "Save entry type is invalid: " + name);
            return std::nullopt;
        }

        values[key] =
            value;
    }

    return values;
}

bool PersistenceSystem::writeFile(
    const std::string& name,
    const SaveMap& values
)
{
    std::error_code error;
    std::filesystem::create_directories(saveDirectory(), error);

    if (error)
    {
        Logger::warning(
            "save",
            "Save directory could not be created: " + error.message()
        );

        return false;
    }

    std::vector<uint8_t> payload;
    writeU32(payload, FormatVersion);
    writeU32(payload, static_cast<uint32_t>(values.size()));

    for (const auto& pair : values)
    {
        writeString(payload, pair.first);
        writeU32(payload, static_cast<uint32_t>(pair.second.type));

        if (pair.second.type == PersistedValue::Type::Boolean)
        {
            writeU32(payload, pair.second.booleanValue ? 1u : 0u);
        }
        else if (pair.second.type == PersistedValue::Type::Number)
        {
            writeDouble(payload, pair.second.numberValue);
        }
        else if (pair.second.type == PersistedValue::Type::String)
        {
            writeString(payload, pair.second.stringValue);
        }
    }

    obfuscate(payload);

    std::vector<uint8_t> bytes;
    bytes.insert(bytes.end(), std::begin(Magic), std::end(Magic));
    writeU32(bytes, checksum(payload));
    bytes.insert(bytes.end(), payload.begin(), payload.end());

    const std::filesystem::path path =
        savePath(name);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        Logger::warning(
            "save",
            "Save file could not be written: " + path.string()
        );

        return false;
    }

    file.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );

    return file.good();
}
