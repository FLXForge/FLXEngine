#pragma once

#include <optional>
#include <filesystem>
#include <string>
#include <unordered_map>

struct PersistedValue
{
    enum class Type
    {
        Boolean = 1,
        Number = 2,
        String = 3
    };

    Type type = Type::Number;
    bool booleanValue = false;
    double numberValue = 0.0;
    std::string stringValue;
};

class PersistenceSystem
{
public:
    explicit PersistenceSystem(const std::optional<std::filesystem::path>& directory = std::nullopt);
    bool save(
        const std::string& name,
        const std::string& key,
        const PersistedValue& value
    );

    std::optional<PersistedValue> load(
        const std::string& name,
        const std::string& key
    );

private:
    std::filesystem::path directory;
    std::filesystem::path savePath(const std::string& name) const;
    bool exists(const std::string& name) const;
    using SaveMap =
        std::unordered_map<std::string, PersistedValue>;

    std::optional<SaveMap> readFile(
        const std::string& name
    );

    bool writeFile(
        const std::string& name,
        const SaveMap& values
    );
};
