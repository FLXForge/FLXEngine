#pragma once

#include <optional>
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
