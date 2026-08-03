#include "ResourceRegistry.h"

bool ResourceRegistry::addObject(
    const ResourceId& id,
    const ObjectDefinition& definition
)
{
    return objects.emplace(id, definition).second;
}

const ObjectDefinition* ResourceRegistry::findObject(
    const ResourceId& id
) const
{
    const auto it =
        objects.find(id);

    if (it == objects.end())
    {
        return nullptr;
    }

    return &it->second;
}

bool ResourceRegistry::hasObject(
    const ResourceId& id
) const
{
    return objects.find(id) != objects.end();
}

size_t ResourceRegistry::objectCount() const
{
    return objects.size();
}

bool ResourceRegistry::addScript(
    const ResourceId& id,
    const ScriptResource& script
)
{
    return scripts.emplace(id, script).second;
}

const ScriptResource* ResourceRegistry::findScript(
    const ResourceId& id
) const
{
    const auto it =
        scripts.find(id);

    if (it == scripts.end())
    {
        return nullptr;
    }

    return &it->second;
}

bool ResourceRegistry::hasScript(
    const ResourceId& id
) const
{
    return scripts.find(id) != scripts.end();
}

size_t ResourceRegistry::scriptCount() const
{
    return scripts.size();
}

const std::map<ResourceId, ObjectDefinition>& ResourceRegistry::allObjects() const
{
    return objects;
}

const std::map<ResourceId, ScriptResource>& ResourceRegistry::allScripts() const
{
    return scripts;
}
