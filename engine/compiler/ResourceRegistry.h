#pragma once

#include "../runtime/ObjectDefinition.h"

#include <map>
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
    bool addObject(
        const ResourceId& id,
        const ObjectDefinition& definition
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
    std::map<ResourceId, ObjectDefinition> objects;
    std::map<ResourceId, ScriptResource> scripts;
};
