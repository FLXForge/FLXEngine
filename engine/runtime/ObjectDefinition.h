#pragma once

#include <raylib.h>

#include "../audio/MusicDefinition.h"
#include "../audio/SoundDefinition.h"

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

struct ObjectDefinition
{
    std::string id;
    std::string sourcePath;
    std::string spawnMode = "auto";

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

    float speed = 120.0f;
    bool hasSpeed = false;
    float angle = 0.0f;
    bool hasAngle = false;
    bool inheritParentAngle = false;

    float rotationSpeed = 0.0f;
    float acceleration = 0.0f;
    float maxSpeed = 0.0f;
    float inertia = 1.0f;

    std::string boundsMode = "none";
    bool boundsOverflow = false;

    std::string group;
    std::string role;
    std::string collisionType = "none";
    bool collisionActive = false;
    float collisionRadius = 0.0f;
    std::vector<std::string> collisionWith;

    std::vector<std::string> scripts;
    std::vector<std::string> resolvedScriptPaths;
    std::unordered_map<std::string, MusicDefinition> music;
    std::unordered_map<std::string, SoundDefinition> sounds;
    std::unordered_map<std::string, ObjectDefinition> children;
    std::unordered_map<std::string, std::string> childResources;

    std::string initialState;
    std::unordered_map<std::string, std::vector<std::string>> stateTransitions;

    std::string creationMode = "individual";
    GridCreationRules gridRules;
    bool gridPatternIsRows = false;
    std::vector<std::string> gridPattern;
    std::vector<std::vector<std::string>> gridRowPattern;
};
