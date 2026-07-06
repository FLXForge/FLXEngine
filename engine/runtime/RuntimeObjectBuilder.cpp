#include "RuntimeObjectBuilder.h"

RuntimeObject RuntimeObjectBuilder::build(
    const ObjectDefinition& definition,
    const std::string& runtimeId,
    const std::string& parentId
)
{
    RuntimeObject object(
        definition.id,
        definition.origin,
        definition.size,
        definition.color
    );

    object.runtimeId = runtimeId;
    object.parentId = parentId;
    object.originalParentId = parentId;
    object.sourcePath = definition.sourcePath;
    object.hasOrigin = definition.hasOrigin;
    object.visible = definition.visible;
    object.layer = definition.layer;
    object.attached = definition.attachOnCreate;
    object.originalOffset = definition.offset;
    object.attachFollowX = definition.attachFollowX;
    object.attachFollowY = definition.attachFollowY;
    object.attachFollowAngle = definition.attachFollowAngle;

    if (!definition.hasVisual)
    {
        object.visible = false;
    }

    object.shapeMode = definition.shapeMode;
    object.shapeType = definition.shapeType;
    object.textContent = definition.textContent;
    object.radius = definition.radius;
    object.points = definition.points;

    object.speed = definition.speed;
    object.originSpeed = definition.speed;
    object.angle = definition.angle;

    object.rotationSpeed = definition.rotationSpeed;
    object.acceleration = definition.acceleration;
    object.maxSpeed = definition.maxSpeed;
    object.inertia = definition.inertia;

    object.boundsMode = definition.boundsMode;
    object.boundsOverflow = definition.boundsOverflow;

    object.group = definition.group;
    object.role = definition.role;
    object.collisionType = definition.collisionType;
    object.collisionActive = definition.collisionActive;
    object.collisionRadius = definition.collisionRadius;
    object.collisionWith = definition.collisionWith;

    object.scripts = definition.scripts;
    object.music = definition.music;
    object.sounds = definition.sounds;
    object.children = definition.children;
    object.state = definition.initialState;
    object.stateTransitions = definition.stateTransitions;
    object.creationMode = definition.creationMode;
    object.gridRules = definition.gridRules;
    object.gridPatternIsRows = definition.gridPatternIsRows;
    object.gridPattern = definition.gridPattern;
    object.gridRowPattern = definition.gridRowPattern;

    return object;
}
