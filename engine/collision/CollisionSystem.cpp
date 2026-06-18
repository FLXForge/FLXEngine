#include "CollisionSystem.h"
#include "../runtime/RuntimeHelpers.h"
#include "../runtime/RuntimeObject.h"
#include "../scripting/ScriptEngine.h"

#include <algorithm>

namespace
{
    bool canCollideWith(
        const RuntimeObject& object,
        const RuntimeObject& other
    )
    {
        if (!object.collisionActive)
        {
            return false;
        }

        if (object.collisionWith.empty())
        {
            return false;
        }

        return std::find(
            object.collisionWith.begin(),
            object.collisionWith.end(),
            other.group
        ) != object.collisionWith.end();
    }
}

void CollisionSystem::run(
    std::vector<RuntimeObject>& objects,
    ScriptEngine& scriptEngine
)
{
    for (size_t i = 0; i < objects.size(); ++i)
    {
        RuntimeObject& a =
            objects[i];

        if (!a.alive || !a.collisionActive)
        {
            continue;
        }

        for (size_t j = 0; j < objects.size(); ++j)
        {
            if (i == j)
            {
                continue;
            }

            RuntimeObject& b =
                objects[j];

            if (!b.alive)
            {
                continue;
            }

            if (!canCollideWith(a, b))
            {
                continue;
            }

            if (RuntimeHelpers::intersects(a, b))
            {
                for (const auto& scriptPath : a.resolvedScriptPaths)
                {
                    scriptEngine.callScriptFunction(
                        scriptPath,
                        "collision",
                        a,
                        b
                    );

                    if (!a.alive)
                    {
                        break;
                    }
                }

                if (!a.alive)
                {
                    break;
                }
            }
        }
    }
}
