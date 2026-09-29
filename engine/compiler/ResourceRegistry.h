#pragma once

#include "../runtime/ObjectDefinition.h"

#include <map>
#include <memory>
#include <string>

using ResourceId = std::string;

struct ScriptResource
{
    ResourceId id;
    std::string sourceName;
    std::string code;
};

class ResourceRegistry
{
public:
    ResourceRegistry() = default;
    ResourceRegistry(const ResourceRegistry& other);
    ResourceRegistry& operator=(const ResourceRegistry& other);
    ResourceRegistry(ResourceRegistry&& other) noexcept = default;
    ResourceRegistry& operator=(ResourceRegistry&& other) noexcept = default;

    bool addObject(
        const ResourceId& id,
        const ObjectDefinition& definition
    );

    bool addObjectOwned(
        const ResourceId& id,
        std::unique_ptr<ObjectDefinition> definition
    );

    const ObjectDefinition* findObject(
        const ResourceId& id
    ) const;

    bool hasObject(
        const ResourceId& id
    ) const;

    size_t objectCount() const;

    bool addScript(
        const ResourceId& id,
        const ScriptResource& script
    );

    const ScriptResource* findScript(
        const ResourceId& id
    ) const;

    bool hasScript(
        const ResourceId& id
    ) const;

    size_t scriptCount() const;

    const std::map<ResourceId, ObjectDefinition>& allObjects() const;
    const std::map<ResourceId, ScriptResource>& allScripts() const;

private:
    std::map<ResourceId, std::unique_ptr<ObjectDefinition>> objects;
    mutable std::map<ResourceId, ObjectDefinition> objectSnapshot;
    std::map<ResourceId, ScriptResource> scripts;
};
