#pragma once

#include <string>
#include <raylib.h>
#include <vector>
#include <unordered_map>

struct SpawnDefinition
{
    std::string prefab;
    Vector2 offset;
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

    std::unordered_map<std::string, double> local;

    std::unordered_map<std::string, SpawnDefinition> spawns;

    void draw(
        int scale,
        float screenWidth,
        float screenHeight
    ) const;
    void drawAt(Vector2 drawPosition, int scale) const;

    void applyBounds(float screenWidth, float screenHeight);

public:
    std::string name;
    std::string group;

    bool visible;
    bool alive;
    bool deadCalled;

    Vector2 origin;
    Vector2 position;
    Vector2 size;

    Color color;

    float speed;
    float angle;
    float originSpeed;
    
    Vector2 velocity;

    float rotationSpeed;
    float acceleration;
    float maxSpeed;
    float inertia;

    std::string shapeType;

    std::string boundsMode;
    bool boundsOverflow;

    std::vector<std::string> scripts;
    std::vector<std::string> resolvedScriptPaths;
};