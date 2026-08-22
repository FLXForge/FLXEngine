#include "RuntimeObjectBuilder.h"

namespace
{
    void resolveAxisDefaults(
        MechanicsAxisDefinition& axis,
        const MechanicsMotionDefinition& motion
    )
    {
        if (!axis.hasSpeed)
        {
            axis.speed =
                motion.speed;
        }

        if (!axis.hasAcceleration)
        {
            axis.acceleration =
                motion.acceleration;
        }

        if (!axis.hasInertia)
        {
            axis.inertia =
                motion.inertia;
        }

        if (!axis.hasStep)
        {
            axis.step =
                motion.step;
        }
    }

    MechanicsMotionDefinition resolveMotionDefaults(
        MechanicsMotionDefinition motion
    )
    {
        resolveAxisDefaults(
            motion.horizontal,
            motion
        );

        resolveAxisDefaults(
            motion.vertical,
            motion
        );

        return motion;
    }
}

RuntimeObject RuntimeObjectBuilder::build(
    const ObjectDefinition& definition,
    const std::string& runtimeId,
    const std::string& parentId
)
{
    const std::string objectName =
        definition.id;

    RuntimeObject object(
        objectName,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 0.0f, 0.0f },
        WHITE
    );

    object.runtimeId = runtimeId;
    object.definitionId = definition.id;
    object.parentId = parentId;
    object.originalParentId = parentId;
    object.sourcePath = definition.sourcePath;
    object.hasOrigin = definition.hasOrigin;
    object.origin = definition.origin;
    object.position = definition.origin;
    object.previousPosition = definition.origin;
    object.size = definition.size;
    object.color = definition.color;
    object.visible = definition.visible;
    object.layer = definition.layer;
    object.attached = definition.attachOnCreate;
    object.originalOffset = definition.offset;
    object.attachFollowX = definition.attachFollowX;
    object.attachFollowY = definition.attachFollowY;
    object.attachFollowAngle = definition.attachFollowAngle;

    object.shapeMode = definition.shapeMode;
    object.shapeType = definition.shapeType;
    object.textContent = definition.textContent;
    object.radius = definition.radius;
    object.points = definition.points;

    object.mechanics = definition.mechanics;
    object.inherit = definition.inherit;
    object.mechanicsType = definition.mechanics.type;
    object.mechanicsMotion =
        resolveMotionDefaults(definition.mechanics.motion);
    object.mechanics.motion =
        object.mechanicsMotion;
    object.mechanicsRotation = definition.mechanics.rotation;

    object.speed = definition.mechanics.motion.speed.start;
    object.originSpeed = definition.mechanics.motion.speed.start;
    object.angle = definition.mechanics.rotation.angle;

    object.rotationSpeed = definition.mechanics.rotation.speed.start;
    object.acceleration = definition.mechanics.motion.acceleration;
    object.maxSpeed = definition.mechanics.motion.speed.limit;
    object.inertia = definition.mechanics.motion.inertia;

    object.boundsMode = definition.boundsMode;
    object.boundsOverflow = definition.boundsOverflow;

    object.group = definition.group;
    object.role = definition.role;
    object.controlPlayer = definition.controlPlayer;
    object.collisionType = definition.collisionType;
    object.collisionActive = definition.collisionActive;
    object.collisionRadius = definition.collisionRadius;
    object.collisionWith = definition.collisionWith;

    object.scripts = definition.scripts;
    object.resolvedScriptPaths = definition.resolvedScriptPaths;
    object.music = definition.music;
    object.sounds = definition.sounds;
    object.children = definition.children;
    object.childResources = definition.childResources;
    object.state = definition.initialState;
    object.creationMode = definition.creationMode;
    object.gridRules = definition.gridRules;
    object.gridPatternIsRows = definition.gridPatternIsRows;
    object.gridPattern = definition.gridPattern;
    object.gridRowPattern = definition.gridRowPattern;

    return object;
}
