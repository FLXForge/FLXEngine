#include "ResourceRegistry.h"

#include <memory>

namespace
{
    void copyObjectDefinitionFields(
        ObjectDefinition& target,
        const ObjectDefinition& source
    )
    {
        target.id = source.id;
        target.sourcePath = source.sourcePath;
        target.spawnMode = source.spawnMode;
        target.component = source.component;

        target.offset = source.offset;
        target.hasOffset = source.hasOffset;

        target.attachFollowX = source.attachFollowX;
        target.attachFollowY = source.attachFollowY;
        target.attachFollowAngle = source.attachFollowAngle;
        target.attachOnCreate = source.attachOnCreate;

        target.visible = source.visible;
        target.hasVisual = source.hasVisual;
        target.layer = source.layer;

        target.origin = source.origin;
        target.hasOrigin = source.hasOrigin;
        target.size = source.size;
        target.color = source.color;
        target.shapeMode = source.shapeMode;
        target.shapeType = source.shapeType;
        target.textContent = source.textContent;
        target.radius = source.radius;
        target.points = source.points;

        target.mechanics = source.mechanics;
        target.inherit = source.inherit;

        target.boundsMode = source.boundsMode;
        target.boundsOverflow = source.boundsOverflow;

        target.group = source.group;
        target.controlPlayer = source.controlPlayer;
        target.local = source.local;
        target.collisions = source.collisions;

        target.scripts = source.scripts;
        target.scriptSourcePaths = source.scriptSourcePaths;
        target.resolvedScriptPaths = source.resolvedScriptPaths;
        target.music = source.music;
        target.sounds = source.sounds;
        target.children = source.children;
        target.childResources = source.childResources;
        target.childSourcePaths = source.childSourcePaths;

        target.initialState = source.initialState;
        target.stateTransitions = source.stateTransitions;

        target.creationMode = source.creationMode;
        target.gridRules = source.gridRules;
        target.gridPatternIsRows = source.gridPatternIsRows;
        target.gridPattern = source.gridPattern;
        target.gridRowPattern = source.gridRowPattern;
    }
}

ResourceRegistry::ResourceRegistry(
    const ResourceRegistry& other
)
    : scripts(other.scripts)
{
    for (const auto& object : other.objects)
    {
        auto copy =
            std::make_unique<ObjectDefinition>();

        copyObjectDefinitionFields(
            *copy,
            *object.second
        );

        objects.emplace(
            object.first,
            std::move(copy)
        );
    }
}

ResourceRegistry& ResourceRegistry::operator=(
    const ResourceRegistry& other
)
{
    if (this == &other)
    {
        return *this;
    }

    objects.clear();
    objectSnapshot.clear();
    scripts =
        other.scripts;

    for (const auto& object : other.objects)
    {
        auto copy =
            std::make_unique<ObjectDefinition>();

        copyObjectDefinitionFields(
            *copy,
            *object.second
        );

        objects.emplace(
            object.first,
            std::move(copy)
        );
    }

    return *this;
}

bool ResourceRegistry::addObject(
    const ResourceId& id,
    const ObjectDefinition& definition
)
{
    auto object =
        std::make_unique<ObjectDefinition>();

    copyObjectDefinitionFields(
        *object,
        definition
    );

    const bool added =
        objects.emplace(id, std::move(object)).second;

    if (added)
    {
        objectSnapshot.clear();
    }

    return added;
}

bool ResourceRegistry::addObjectOwned(
    const ResourceId& id,
    std::unique_ptr<ObjectDefinition> definition
)
{
    if (!definition)
    {
        return false;
    }

    const bool added =
        objects.emplace(id, std::move(definition)).second;

    if (added)
    {
        objectSnapshot.clear();
    }

    return added;
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

    return it->second.get();
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
    if (objectSnapshot.size() != objects.size())
    {
        objectSnapshot.clear();

        for (const auto& object : objects)
        {
            ObjectDefinition& snapshot =
                objectSnapshot[object.first];

            copyObjectDefinitionFields(
                snapshot,
                *object.second
            );
        }
    }

    return objectSnapshot;
}

const std::map<ResourceId, ScriptResource>& ResourceRegistry::allScripts() const
{
    return scripts;
}
