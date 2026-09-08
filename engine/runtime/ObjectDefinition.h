#pragma once

#include <raylib.h>

#include "../audio/MusicDefinition.h"
#include "../audio/SoundDefinition.h"
#include "ScriptValue.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct GridCreationRules
{
    int rows = 0;
    int columns = 0;
    float cellWidth = 0.0f;
    float cellHeight = 0.0f;
};

enum class MechanicsType
{
    Direct,
    Polar
};

enum class MechanicsDiagonalMode
{
    Independent,
    Vector
};

enum class InheritCreationMode
{
    None,
    Copy,
    Compose
};

enum class InheritLiveMode
{
    None,
    Copy
};

struct MechanicsSpeedDefinition
{
    float start = 0.0f;
    float limit = 0.0f;
};

struct MechanicsAxisDefinition
{
    MechanicsSpeedDefinition speed;
    float acceleration = 0.0f;
    float inertia = 0.0f;
    float step = 0.0f;

    bool hasSpeed = false;
    bool hasAcceleration = false;
    bool hasInertia = false;
    bool hasStep = false;
};

struct MechanicsMotionDefinition
{
    MechanicsSpeedDefinition speed;
    float acceleration = 0.0f;
    float inertia = 0.0f;
    float step = 0.0f;
    MechanicsDiagonalMode diagonal = MechanicsDiagonalMode::Independent;
    MechanicsAxisDefinition horizontal;
    MechanicsAxisDefinition vertical;
};

struct MechanicsRotationDefinition
{
    float angle = 0.0f;
    MechanicsSpeedDefinition speed;
    float acceleration = 0.0f;
    float inertia = 0.0f;
    float step = 0.0f;
};

struct MechanicsDefinition
{
    MechanicsType type = MechanicsType::Direct;
    MechanicsMotionDefinition motion;
    MechanicsRotationDefinition rotation;
};

struct InheritDefinition
{
    InheritCreationMode creationAngle = InheritCreationMode::None;
    InheritCreationMode creationVelocity = InheritCreationMode::None;
    InheritLiveMode liveAngle = InheritLiveMode::None;
};

struct ColliderSizeDefinition
{
    float width = 0.0f;
    float height = 0.0f;
    bool hasWidth = false;
    bool hasHeight = false;
};

struct ColliderDefinition
{
    std::string type = "box";
    ColliderSizeDefinition size;
    Vector2 offset = Vector2{ 0.0f, 0.0f };
    float angle = 0.0f;
    std::vector<std::string> with;
    bool enabled = true;
    std::vector<std::string> states;
};

struct ObjectDefinition
{
    std::string id;
    std::string sourcePath;
    std::string spawnMode = "auto";
    bool component = false;

    Vector2 offset = Vector2{ 0.0f, 0.0f };
    bool hasOffset = false;

    bool attachFollowX = false;
    bool attachFollowY = false;
    bool attachFollowAngle = false;
    bool attachOnCreate = false;

    bool visible = true;
    bool hasVisual = false;
    int layer = 0;

    Vector2 origin = Vector2{ 0.0f, 0.0f };
    bool hasOrigin = false;

    Vector2 size = Vector2{ 0.0f, 0.0f };

    Color color = WHITE;
    std::string shapeMode = "fill";
    std::string shapeType = "block";
    std::string textContent;
    float radius = 0.0f;
    std::vector<Vector2> points;

    MechanicsDefinition mechanics;
    InheritDefinition inherit;

    std::string boundsMode = "none";
    bool boundsOverflow = false;

    std::string group;
    int controlPlayer = 0;
    std::unordered_map<std::string, ScriptValue> local;
    std::unordered_map<std::string, ColliderDefinition> collisions;

    std::vector<std::string> scripts;
    std::vector<std::string> scriptSourcePaths;
    std::vector<std::string> resolvedScriptPaths;
    std::unordered_map<std::string, MusicDefinition> music;
    std::unordered_map<std::string, SoundDefinition> sounds;
    std::map<std::string, std::shared_ptr<ObjectDefinition>> children;
    std::unordered_map<std::string, std::string> childResources;
    std::unordered_map<std::string, std::string> childSourcePaths;

    std::string initialState;
    std::unordered_map<std::string, std::vector<std::string>> stateTransitions;

    std::string creationMode = "individual";
    GridCreationRules gridRules;
    bool gridPatternIsRows = false;
    std::vector<std::string> gridPattern;
    std::vector<std::vector<std::string>> gridRowPattern;
};
