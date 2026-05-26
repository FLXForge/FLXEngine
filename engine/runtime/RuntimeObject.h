#pragma once

#include <string>
#include <raylib.h>
#include <vector>

class RuntimeObject
{
public:
    RuntimeObject(
        const std::string& name,
        Vector2 origin,
        Vector2 size,
        Color color
    );

    void draw(int scale) const;

public:
    std::string name;
    std::string group;

    bool visible;

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

    std::vector<std::string> scripts;
    std::vector<std::string> resolvedScriptPaths;
};