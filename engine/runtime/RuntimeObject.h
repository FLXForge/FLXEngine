#pragma once

#include "ObjectDefinition.h"
#include "ScriptValue.h"

#include <string>
#include <raylib.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

enum class RuntimeTimerStatus
{
    Running,
    Paused,
    Done
};

struct RuntimeTimer
{
    float duration = 0.0f;
    float left = 0.0f;
    RuntimeTimerStatus status = RuntimeTimerStatus::Running;
};

class RuntimeObject
{
public:
    RuntimeObject(
        const std::string& name,
        Vector2 origin,
        Vector2 size,
        Color color
    );

    std::unordered_map<std::string, ScriptValue> local;

    std::unordered_map<std::string, ObjectDefinition> children;
    std::unordered_map<std::string, std::string> childResources;
    std::unordered_map<std::string, MusicDefinition> music;
    std::unordered_map<std::string, SoundDefinition> sounds;
    std::unordered_map<std::string, RuntimeTimer> timers;
    std::string creationMode = "individual";
    GridCreationRules gridRules;
    bool gridPatternIsRows = false;
    std::vector<std::string> gridPattern;
    std::vector<std::vector<std::string>> gridRowPattern;

    void draw(
        int scale,
        float screenWidth,
        float screenHeight
    ) const;
    void drawAt(Vector2 drawPosition, int scale) const;
    void drawCollision(float scale) const;

    void applyBounds(float screenWidth, float screenHeight);

public:
    std::string name;
    std::string runtimeId;
    std::string definitionId;
    std::string parentId;
    std::string originalParentId;
    std::string sourcePath;
    std::string group;
    std::string role;
    int controlPlayer = 0;
    std::string state;
    float stateTime = 0.0f;
    uint64_t stateEnteredFrame = 0;

    bool visible;
    bool alive;
    bool deadCalled;
    bool attached = false;
    int layer = 0;

    Vector2 origin;
    Vector2 position;
    Vector2 previousPosition;
    Vector2 size;
    Vector2 originalOffset;

    bool attachFollowX = false;
    bool attachFollowY = false;
    bool attachFollowAngle = false;

    bool hasOrigin = false;

    Color color;
    std::string shapeMode;

    float radius;
    MechanicsDefinition mechanics;
    InheritDefinition inherit;
    MechanicsType mechanicsType = MechanicsType::Direct;
    MechanicsMotionDefinition mechanicsMotion;
    MechanicsRotationDefinition mechanicsRotation;

    float speed;
    float angle;
    float originSpeed;
    
    Vector2 velocity;
    float angularVelocity = 0.0f;
    bool motionCommanded = false;
    bool rotationCommanded = false;
    Vector2 frameMotionDelta = Vector2{ 0.0f, 0.0f };

    float rotationSpeed;
    float acceleration;
    float maxSpeed;
    float inertia;

    std::string shapeType;
    std::string textContent;
    std::vector<Vector2> points;

    std::string boundsMode;
    bool boundsOverflow;

    bool collisionActive;
    std::vector<std::string> collisionWith;
    std::string collisionType;
    float collisionRadius;

    std::vector<std::string> scripts;
    std::vector<std::string> resolvedScriptPaths;
};
