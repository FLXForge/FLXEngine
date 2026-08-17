#include "RuntimeObjectBuilder.h"

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
