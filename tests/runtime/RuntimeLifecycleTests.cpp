#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    constexpr float ScreenWidth = 320.0f;
    constexpr float ScreenHeight = 180.0f;
    constexpr float FrameDelta = 0.1f;

    struct RuntimeHarness
    {
        CompiledProject project;
        RuntimeWorld world;
        ScriptEngine scripts;

        RuntimeHarness()
        {
            project.rootId = "root";
            project.context.machine.video.screenWidth = static_cast<int>(ScreenWidth);
            project.context.machine.video.screenHeight = static_cast<int>(ScreenHeight);
            project.context.machine.video.outputScale = 1;
            scripts.setScreenScale(1);

            scripts.setFindObjectFunction(
                [this](const std::string& name)
                {
                    return world.findByName(name);
                }
            );

            scripts.setFindObjectByIdFunction(
                [this](const std::string& runtimeId)
                {
                    return world.findByRuntimeId(runtimeId);
                }
            );

            scripts.setSpawnObjectFunction(
                [this](RuntimeObject& source, const std::string& resourceId)
                {
                    world.spawn(source, resourceId, scripts);
                }
            );

            scripts.setKeepOnlyFunction(
                [this](const std::string& runtimeId)
                {
                    world.keepOnly(runtimeId);
                }
            );

            scripts.setKillObjectFunction(
                [this](const std::string& runtimeId)
                {
                    world.kill(runtimeId);
                }
            );

            scripts.setShowObjectFunction(
                [this](const std::string& runtimeId)
                {
                    world.show(runtimeId);
                }
            );

            scripts.setHideObjectFunction(
                [this](const std::string& runtimeId)
                {
                    world.hide(runtimeId);
                }
            );

            scripts.setRayCastFunction(
                [this](RuntimeObject& source, float angle, float distance)
                {
                    return world.rayCast(source, angle, distance);
                }
            );
        }

        void addScript(const std::string& id, const std::string& code)
        {
            ScriptResource script;
            script.id = id;
            script.sourceName = id + ".js";
            script.code = code;

            require(
                project.resources.addScript(id, script),
                "script should be added: " + id
            );
        }

        void addObject(ObjectDefinition definition)
        {
            if (definition.sourcePath.empty())
            {
                definition.sourcePath = definition.id + ".json";
            }

            require(
                project.resources.addObject(definition.id, definition),
                "object should be added: " + definition.id
            );
        }

        RuntimeLoadResult load()
        {
            return world.load(project, scripts);
        }

        void update(float delta = FrameDelta)
        {
            scripts.setFrameDelta(delta);
            world.update(scripts, ScreenWidth, ScreenHeight, delta);
        }

        void draw()
        {
            world.draw(scripts, 1, ScreenWidth, ScreenHeight, false);
        }
    };

    ObjectDefinition objectDefinition(
        const std::string& id,
        const std::string& script = ""
    )
    {
        ObjectDefinition definition;
        definition.id = id;
        definition.visible = true;
        definition.shapeType = "none";

        if (!script.empty())
        {
            definition.resolvedScriptPaths.push_back(script);
        }

        return definition;
    }

    RuntimeObject& requireObject(
        RuntimeWorld& world,
        const std::string& name
    )
    {
        RuntimeObject* object =
            world.findByName(name);

        require(object != nullptr, "runtime object should exist: " + name);

        return *object;
    }

    double localValue(
        RuntimeObject& object,
        const std::string& key
    )
    {
        const auto it =
            object.local.find(key);

        if (it == object.local.end())
        {
            return 0.0;
        }

        return it->second;
    }

    bool nearlyEqual(
        double left,
        double right,
        double epsilon = 0.0001
    )
    {
        return std::abs(left - right) <= epsilon;
    }

    bool sameColor(
        Color left,
        Color right
    )
    {
        return
            left.r == right.r &&
            left.g == right.g &&
            left.b == right.b &&
            left.a == right.a;
    }

    class HiddenTestWindow
    {
    public:
        HiddenTestWindow()
        {
            if (!IsWindowReady())
            {
                SetTraceLogLevel(LOG_WARNING);
                SetConfigFlags(FLAG_WINDOW_HIDDEN);
                InitWindow(1, 1, "FLX runtime lifecycle test");
                opened = true;
            }
        }

        ~HiddenTestWindow()
        {
            if (opened)
            {
                CloseWindow();
            }
        }

    private:
        bool opened = false;
    };

    int countRenderedColor(
        RuntimeHarness& harness,
        Color color
    )
    {
        HiddenTestWindow window;

        RenderTexture2D target =
            LoadRenderTexture(16, 16);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        harness.draw();
        EndTextureMode();

        Image image =
            LoadImageFromTexture(target.texture);

        int count = 0;

        for (int y = 0; y < image.height; ++y)
        {
            for (int x = 0; x < image.width; ++x)
            {
                if (sameColor(GetImageColor(image, x, y), color))
                {
                    ++count;
                }
            }
        }

        UnloadImage(image);
        UnloadRenderTexture(target);

        return count;
    }

    void testInitialLoadBornOrderAndIdentity()
    {
        RuntimeHarness harness;

        harness.addScript(
            "recordBorn",
            "function born(o) {"
            "  if (global['order'] == null) global['order'] = 0;"
            "  if (o.local['bornCount'] == null) o.local['bornCount'] = 0;"
            "  o.local['bornOrder'] = global['order'];"
            "  o.local['bornCount'] = o.local['bornCount'] + 1;"
            "  global['order'] = global['order'] + 1;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "recordBorn");
        root.childResources["parent"] = "parent";

        ObjectDefinition parent =
            objectDefinition("parent", "recordBorn");
        parent.childResources["child"] = "child";

        ObjectDefinition child =
            objectDefinition("child", "recordBorn");

        harness.addObject(root);
        harness.addObject(parent);
        harness.addObject(child);

        RuntimeLoadResult result =
            harness.load();

        require(result.success, "runtime should load initial graph");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeParent =
            requireObject(harness.world, "parent");
        RuntimeObject& runtimeChild =
            requireObject(harness.world, "child");

        require(localValue(runtimeRoot, "bornCount") == 1.0, "root born should run once");
        require(localValue(runtimeParent, "bornCount") == 1.0, "parent born should run once");
        require(localValue(runtimeChild, "bornCount") == 1.0, "child born should run once");

        require(localValue(runtimeRoot, "bornOrder") == 0.0, "root born order should be first");
        require(localValue(runtimeParent, "bornOrder") == 1.0, "parent born order should follow root");
        require(localValue(runtimeChild, "bornOrder") == 2.0, "child born order should follow parent");

        require(runtimeRoot.runtimeId != runtimeParent.runtimeId, "root and parent ids should differ");
        require(runtimeParent.runtimeId != runtimeChild.runtimeId, "parent and child ids should differ");
        require(runtimeChild.parentId == runtimeParent.runtimeId, "child parentId should point to parent");
        require(runtimeChild.originalParentId == runtimeParent.runtimeId, "child originalParentId should point to parent");
    }

    void testSpawnDuringBornIsStabilizedBeforeLoadReturns()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnerBorn",
            "function born(o) { spawn(o, 'manual'); }"
            "function action(o) { o.local['actionCount'] = (o.local['actionCount'] || 0) + 1; }"
        );

        harness.addScript(
            "spawnedProbe",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; }"
            "function action(o) { o.local['actionCount'] = (o.local['actionCount'] || 0) + 1; }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnerBorn");
        root.childResources["manual"] = "spawned";

        ObjectDefinition spawned =
            objectDefinition("spawned", "spawnedProbe");
        spawned.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(spawned);

        require(harness.load().success, "runtime should load born spawn project");

        RuntimeObject& runtimeSpawned =
            requireObject(harness.world, "spawned");

        require(localValue(runtimeSpawned, "bornCount") == 1.0, "born-spawned object should be born once");
        require(localValue(runtimeSpawned, "actionCount") == 0.0, "born-spawned object should not run action during load");

        harness.update();

        RuntimeObject& updatedSpawned =
            requireObject(harness.world, "spawned");

        require(localValue(updatedSpawned, "actionCount") == 1.0, "born-spawned object should join first frame action");
        require(localValue(updatedSpawned, "motionCount") == 1.0, "born-spawned object should join first frame motion");
    }

    void testRecursiveSpawnDuringBornIsStabilizedBeforeLoadReturns()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnA",
            "function born(o) { spawn(o, 'a'); }"
        );

        harness.addScript(
            "spawnB",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; spawn(o, 'b'); }"
        );

        harness.addScript(
            "bornProbe",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnA");
        root.childResources["a"] = "a";

        ObjectDefinition a =
            objectDefinition("a", "spawnB");
        a.spawnMode = "manual";
        a.childResources["b"] = "b";

        ObjectDefinition b =
            objectDefinition("b", "bornProbe");
        b.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(a);
        harness.addObject(b);

        require(harness.load().success, "runtime should stabilize recursive born spawns");

        RuntimeObject& runtimeA =
            requireObject(harness.world, "a");
        RuntimeObject& runtimeB =
            requireObject(harness.world, "b");

        require(localValue(runtimeA, "bornCount") == 1.0, "recursive spawned parent should be born once");
        require(localValue(runtimeB, "bornCount") == 1.0, "recursive spawned child should be born once");
    }

    void testSpawnDuringBornLimitFailsLoad()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnSelf",
            "function born(o) { spawn(o, 'self'); }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnSelf");
        root.childResources["self"] = "root";
        root.spawnMode = "manual";

        harness.addObject(root);

        RuntimeLoadResult result =
            harness.load();

        require(!result.success, "runtime load should fail when born spawn never stabilizes");
        require(result.diagnostics.hasErrors(), "born spawn limit should produce diagnostics");
    }

    void testSpawnDuringActionMotionAndCollisionPhases()
    {
        RuntimeHarness harness;

        harness.addScript(
            "phaseSpawner",
            "function action(o) { if (o.local['a'] == null) { o.local['a'] = 1; spawn(o, 'actionChild'); } }"
            "function motion(o) { if (o.local['m'] == null) { o.local['m'] = 1; spawn(o, 'motionChild'); } }"
            "function collision(o, other) { if (o.local['c'] == null) { o.local['c'] = 1; spawn(o, 'collisionChild'); } }"
        );

        harness.addScript(
            "phaseProbe",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; }"
            "function action(o) { o.local['actionCount'] = (o.local['actionCount'] || 0) + 1; }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "phaseSpawner");
        root.childResources["actionChild"] = "actionChild";
        root.childResources["motionChild"] = "motionChild";
        root.childResources["collisionChild"] = "collisionChild";
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "circle";
        root.collisionWith.push_back("target");
        root.group = "source";
        root.origin = Vector2{ 20.0f, 20.0f };
        root.hasOrigin = true;
        root.size = Vector2{ 10.0f, 10.0f };
        root.collisionRadius = 8.0f;

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "circle";
        target.offset = Vector2{ 0.0f, 0.0f };
        target.hasOffset = true;
        target.size = Vector2{ 10.0f, 10.0f };
        target.collisionRadius = 8.0f;

        ObjectDefinition actionChild =
            objectDefinition("actionChild", "phaseProbe");
        actionChild.spawnMode = "manual";
        ObjectDefinition motionChild =
            objectDefinition("motionChild", "phaseProbe");
        motionChild.spawnMode = "manual";
        ObjectDefinition collisionChild =
            objectDefinition("collisionChild", "phaseProbe");
        collisionChild.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(target);
        harness.addObject(actionChild);
        harness.addObject(motionChild);
        harness.addObject(collisionChild);

        require(harness.load().success, "runtime should load phase spawn project");
        RuntimeObject& loadedRoot =
            requireObject(harness.world, "root");
        RuntimeObject& loadedTarget =
            requireObject(harness.world, "target");
        require(loadedRoot.collisionActive, "root collision should be active");
        require(loadedRoot.collisionType == "circle", "root collision should be circle");
        require(loadedTarget.collisionType == "circle", "target collision should be circle");
        require(loadedTarget.group == "target", "target group should be target");
        require(loadedRoot.position.x == loadedTarget.position.x, "root and target x should match");
        require(loadedRoot.position.y == loadedTarget.position.y, "root and target y should match");

        harness.update();

        RuntimeObject& updatedRoot =
            requireObject(harness.world, "root");
        require(
            localValue(updatedRoot, "c") == 1.0,
            "collision callback should run"
        );

        RuntimeObject& actionRuntime =
            requireObject(harness.world, "actionChild");
        RuntimeObject& motionRuntime =
            requireObject(harness.world, "motionChild");
        RuntimeObject& collisionRuntime =
            requireObject(harness.world, "collisionChild");

        require(localValue(actionRuntime, "bornCount") == 1.0, "action spawn should be born once");
        require(localValue(actionRuntime, "actionCount") == 0.0, "action spawn should miss current action phase");
        require(localValue(actionRuntime, "motionCount") == 1.0, "action spawn should join current motion phase");

        require(localValue(motionRuntime, "bornCount") == 1.0, "motion spawn should be born once");
        require(localValue(motionRuntime, "actionCount") == 0.0, "motion spawn should miss current action phase");
        require(localValue(motionRuntime, "motionCount") == 0.0, "motion spawn should miss current motion phase");

        require(localValue(collisionRuntime, "bornCount") == 1.0, "collision spawn should be born once");
        require(localValue(collisionRuntime, "actionCount") == 0.0, "collision spawn should miss current action phase");
        require(localValue(collisionRuntime, "motionCount") == 0.0, "collision spawn should miss current motion phase");
    }

    void testKillDuringActionMotionAndCollisionIsTerminal()
    {
        RuntimeHarness harness;

        harness.addScript(
            "rootDeadCounter",
            "function action(o) { o.local['deadCount'] = global['deadCount'] || 0; }"
        );

        harness.addScript(
            "killInAction",
            "function action(o) { kill(o); }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
            "function dead(o) { global['deadCount'] = (global['deadCount'] || 0) + 1; o.alive = true; }"
        );

        harness.addScript(
            "killInMotion",
            "function motion(o) { kill(o); }"
            "function collision(o, other) { o.local['collisionCount'] = (o.local['collisionCount'] || 0) + 1; }"
            "function dead(o) { global['deadCount'] = (global['deadCount'] || 0) + 1; o.alive = true; }"
        );

        harness.addScript(
            "killInCollision",
            "function collision(o, other) { kill(o); }"
            "function dead(o) { global['deadCount'] = (global['deadCount'] || 0) + 1; o.alive = true; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "rootDeadCounter");
        root.childResources["actionVictim"] = "actionVictim";
        root.childResources["motionVictim"] = "motionVictim";
        root.childResources["collisionVictim"] = "collisionVictim";
        root.childResources["target"] = "target";

        ObjectDefinition actionVictim =
            objectDefinition("actionVictim", "killInAction");

        ObjectDefinition motionVictim =
            objectDefinition("motionVictim", "killInMotion");
        motionVictim.collisionActive = true;
        motionVictim.collisionType = "box";
        motionVictim.collisionWith.push_back("target");
        motionVictim.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition collisionVictim =
            objectDefinition("collisionVictim", "killInCollision");
        collisionVictim.collisionActive = true;
        collisionVictim.collisionType = "box";
        collisionVictim.collisionWith.push_back("target");
        collisionVictim.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(actionVictim);
        harness.addObject(motionVictim);
        harness.addObject(collisionVictim);
        harness.addObject(target);

        require(harness.load().success, "runtime should load kill project");

        harness.update();

        require(harness.world.findByName("actionVictim") == nullptr, "kill in action should be terminal");
        require(harness.world.findByName("motionVictim") == nullptr, "kill in motion should be terminal");
        require(harness.world.findByName("collisionVictim") == nullptr, "kill in collision should be terminal");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(localValue(runtimeRoot, "deadCount") == 3.0, "dead should run exactly once per killed object");
    }

    void testAliveIsReadOnlyFromJavaScript()
    {
        RuntimeHarness harness;

        harness.addScript(
            "writeAlive",
            "function action(o) { o.alive = false; }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "writeAlive");

        harness.addObject(root);

        require(harness.load().success, "runtime should load alive readonly project");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(runtimeRoot.alive, "direct JS alive write should not kill object");
        require(localValue(runtimeRoot, "motionCount") == 1.0, "object should keep participating after ignored alive write");
    }

    void testJsFlatRuntimePropertiesAreMutable()
    {
        RuntimeHarness harness;

        harness.addScript(
            "mutateFlatProperties",
            "function action(o) {"
            "  o.x = 11;"
            "  o.y = 12;"
            "  o.speed = 13;"
            "  o.angle = 14;"
            "  o.velocityX = 15;"
            "  o.velocityY = 16;"
            "  o.width = 17;"
            "  o.height = 18;"
            "  o.layer = 19;"
            "  o.attached = true;"
            "  o.local['mark'] = 20;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "mutateFlatProperties");

        harness.addObject(root);

        require(harness.load().success, "runtime should load flat property mutation project");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(nearlyEqual(runtimeRoot.position.x, 11.0), "JS x write should update runtime position x");
        require(nearlyEqual(runtimeRoot.position.y, 12.0), "JS y write should update runtime position y");
        require(nearlyEqual(runtimeRoot.speed, 13.0), "JS speed write should update runtime speed");
        require(nearlyEqual(runtimeRoot.angle, 14.0), "JS angle write should update runtime angle");
        require(nearlyEqual(runtimeRoot.velocity.x, 15.0), "JS velocityX write should update runtime velocity x");
        require(nearlyEqual(runtimeRoot.velocity.y, 16.0), "JS velocityY write should update runtime velocity y");
        require(nearlyEqual(runtimeRoot.size.x, 17.0), "JS width write should update runtime size x");
        require(nearlyEqual(runtimeRoot.size.y, 18.0), "JS height write should update runtime size y");
        require(runtimeRoot.layer == 19, "JS layer write should update runtime layer");
        require(runtimeRoot.attached, "JS attached write should update runtime attached");
        require(localValue(runtimeRoot, "mark") == 20.0, "JS local values should roundtrip as numeric state");
    }

    void testJsMotionNestedPropertiesAreReadOnlySnapshot()
    {
        RuntimeHarness harness;

        harness.addScript(
            "mutateMotionSnapshot",
            "function action(o) {"
            "  o.motion.speed = 41;"
            "  o.motion.angle = 42;"
            "  o.motion.rotationSpeed = 43;"
            "  o.motion.acceleration = 44;"
            "  o.motion.inertia = 45;"
            "  o.motion.maxSpeed = 46;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "mutateMotionSnapshot");
        root.speed = 1.0f;
        root.angle = 2.0f;
        root.rotationSpeed = 3.0f;
        root.acceleration = 4.0f;
        root.inertia = 5.0f;
        root.maxSpeed = 6.0f;

        harness.addObject(root);

        require(harness.load().success, "runtime should load nested motion mutation project");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(nearlyEqual(runtimeRoot.speed, 1.0), "motion.speed write should not update runtime speed");
        require(nearlyEqual(runtimeRoot.angle, 2.0), "motion.angle write should not update runtime angle");
        require(nearlyEqual(runtimeRoot.rotationSpeed, 3.0), "motion.rotationSpeed write should not update runtime rotationSpeed");
        require(nearlyEqual(runtimeRoot.acceleration, 4.0), "motion.acceleration write should not update runtime acceleration");
        require(nearlyEqual(runtimeRoot.inertia, 5.0), "motion.inertia write should not update runtime inertia");
        require(nearlyEqual(runtimeRoot.maxSpeed, 6.0), "motion.maxSpeed write should not update runtime maxSpeed");
    }

    void testJsMetadataAndIdentityAreReadableButNotAppliedBack()
    {
        RuntimeHarness harness;

        harness.addScript(
            "metadataProbe",
            "function action(o) {"
            "  o.local['idIsRuntime'] = o.id != o.name ? 1 : 0;"
            "  o.local['nameRead'] = o.name == 'root' ? 1 : 0;"
            "  o.local['groupRead'] = o.group == 'actor' ? 1 : 0;"
            "  o.local['roleRead'] = o.role == 'leader' ? 1 : 0;"
            "  o.local['originRead'] = o.originX == 7 && o.originY == 8 ? 1 : 0;"
            "  o.local['originSpeedRead'] = o.originSpeed == 9 ? 1 : 0;"
            "  o.local['previousRead'] = o.previousX == 7 && o.previousY == 8 ? 1 : 0;"
            "  o.id = 'changed_id';"
            "  o.name = 'changed_name';"
            "  o.group = 'changed_group';"
            "  o.role = 'changed_role';"
            "  o.originX = 70;"
            "  o.originY = 80;"
            "  o.originSpeed = 90;"
            "  o.previousX = 700;"
            "  o.previousY = 800;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "metadataProbe");
        root.group = "actor";
        root.role = "leader";
        root.origin = Vector2{ 7.0f, 8.0f };
        root.hasOrigin = true;
        root.speed = 9.0f;

        harness.addObject(root);

        require(harness.load().success, "runtime should load metadata probe project");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(localValue(runtimeRoot, "idIsRuntime") == 1.0, "JS id should expose runtime identity, not logical name");
        require(localValue(runtimeRoot, "nameRead") == 1.0, "JS name should expose logical instance name");
        require(localValue(runtimeRoot, "groupRead") == 1.0, "JS group should be readable");
        require(localValue(runtimeRoot, "roleRead") == 1.0, "JS role should be readable");
        require(localValue(runtimeRoot, "originRead") == 1.0, "JS origin should be readable");
        require(localValue(runtimeRoot, "originSpeedRead") == 1.0, "JS originSpeed should be readable");
        require(localValue(runtimeRoot, "previousRead") == 1.0, "JS previous position should be readable");
        require(runtimeRoot.runtimeId != "changed_id", "JS id write should not update runtimeId");
        require(runtimeRoot.name == "root", "JS name write should not update runtime name");
        require(runtimeRoot.group == "actor", "JS group write should not update runtime group");
        require(runtimeRoot.role == "leader", "JS role write should not update runtime role");
        require(nearlyEqual(runtimeRoot.origin.x, 7.0), "JS originX write should not update runtime origin x");
        require(nearlyEqual(runtimeRoot.origin.y, 8.0), "JS originY write should not update runtime origin y");
        require(nearlyEqual(runtimeRoot.originSpeed, 9.0), "JS originSpeed write should not update runtime originSpeed");
    }

    void testScriptModuleVariablesAreSharedBetweenInstances()
    {
        RuntimeHarness harness;

        harness.addScript(
            "sharedModuleState",
            "var shared = 0;"
            "function born(o) {"
            "  shared = shared + 1;"
            "  o.local['sharedValue'] = shared;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "sharedModuleState");
        root.childResources["child"] = "child";

        ObjectDefinition child =
            objectDefinition("child", "sharedModuleState");

        harness.addObject(root);
        harness.addObject(child);

        require(harness.load().success, "runtime should load shared module state project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeChild =
            requireObject(harness.world, "child");

        require(localValue(runtimeRoot, "sharedValue") == 1.0, "first instance should see initial module state increment");
        require(localValue(runtimeChild, "sharedValue") == 2.0, "second instance should share the same script module state");
    }

    void testCollisionCallbackReceivesConcreteOtherReference()
    {
        RuntimeHarness harness;

        harness.addScript(
            "collisionReferenceProbe",
            "function collision(o, other) {"
            "  o.local['otherName'] = other.name == 'target' ? 1 : 0;"
            "  o.local['otherGroup'] = other.group == 'targetGroup' ? 1 : 0;"
            "  other.x = 44;"
            "  other.local['touched'] = 1;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "collisionReferenceProbe");
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "box";
        root.collisionWith.push_back("targetGroup");
        root.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "targetGroup";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(target);

        require(harness.load().success, "runtime should load collision reference project");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeTarget =
            requireObject(harness.world, "target");

        require(localValue(runtimeRoot, "otherName") == 1.0, "collision second argument should expose the concrete other name");
        require(localValue(runtimeRoot, "otherGroup") == 1.0, "collision second argument should expose the concrete other group");
        require(nearlyEqual(runtimeTarget.position.x, 44.0), "collision second argument writes should apply to the other runtime object");
        require(localValue(runtimeTarget, "touched") == 1.0, "collision second argument local state should roundtrip to the other object");
    }

    void testPublicScriptingFunctionsAreRegistered()
    {
        RuntimeHarness harness;

        harness.addScript(
            "apiSurfaceProbe",
            "function born(o) {"
            "  const functions = ["
            "    'kill','show','hide','keep_only','delta','random','probability','ray',"
            "    'exit','save','load','move_x','move_y','advance','follow_x','follow_y',"
            "    'attach','detach','attach_active','carry','bounce_x','bounce_y','accelerate',"
            "    'rotate','to_origin','draw_text','draw_pixel','draw_line','draw_rectangle',"
            "    'fade_on','fade_off','fade_set','fade_active','fade_done','fade_alpha',"
            "    'play_sound','play_music','stop_music','pause_music','music_active','music_paused',"
            "    'spawn','state_to','state_current','state_active','state_entered','state_time',"
            "    'play_timer','pause_timer','stop_timer','timer_active','timer_paused','timer_done','timer_left',"
            "    'button','direction','player','system','input_pressed','input_down','input_released'"
            "  ];"
            "  let missing = 0;"
            "  for (let i = 0; i < functions.length; i = i + 1) {"
            "    if (typeof globalThis[functions[i]] !== 'function') missing = missing + 1;"
            "  }"
            "  if (typeof globalThis.timer === 'function') missing = missing + 1;"
            "  if (typeof globalThis.timer_clear === 'function') missing = missing + 1;"
            "  if (typeof console !== 'object' || typeof console.log !== 'function') missing = missing + 1;"
            "  if (typeof globalThis.Input !== 'undefined') missing = missing + 1;"
            "  if (typeof globalThis.Key !== 'undefined') missing = missing + 1;"
            "  if (typeof globalThis.__flx_input_player_up === 'function') missing = missing + 1;"
            "  if (typeof globalThis.__flx_input_system_down === 'function') missing = missing + 1;"
            "  if (typeof NEGATIVE !== 'number') missing = missing + 1;"
            "  if (typeof POSITIVE !== 'number') missing = missing + 1;"
            "  if (typeof globalThis.state === 'function') missing = missing + 1;"
            "  const fire = button(0);"
            "  const move = direction(0);"
            "  const p1 = player(1);"
            "  const sys = system();"
            "  if (fire == null || move == null || p1 == null || sys == null) missing = missing + 1;"
            "  o.local['missing'] = missing;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "apiSurfaceProbe");

        harness.addObject(root);

        require(harness.load().success, "runtime should load API surface probe project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(localValue(runtimeRoot, "missing") == 0.0, "public scripting functions should be registered");
    }

    void testKeepOnlyPreventsOtherObjectsFromResurrecting()
    {
        RuntimeHarness harness;

        harness.addScript(
            "keepOnlyRoot",
            "function action(o) { if (o.local['done'] == null) { o.local['done'] = 1; keep_only(o); } }"
        );

        harness.addScript(
            "resurrectInDead",
            "function dead(o) { o.local['deadCount'] = (o.local['deadCount'] || 0) + 1; o.alive = true; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "keepOnlyRoot");
        root.childResources["resurrect"] = "resurrect";

        ObjectDefinition resurrect =
            objectDefinition("resurrect", "resurrectInDead");

        harness.addObject(root);
        harness.addObject(resurrect);

        require(harness.load().success, "runtime should load keep_only project");

        const std::string rootId =
            requireObject(harness.world, "root").runtimeId;

        harness.update();

        require(harness.world.findByRuntimeId(rootId) != nullptr, "keep_only should keep only exact object");
        require(harness.world.findByName("resurrect") == nullptr, "keep_only should keep non-kept object dead even if dead sets alive=true");
    }

    void testHideSuppressesDrawButKeepsRuntimePhasesAndShowRestoresDraw()
    {
        RuntimeHarness harness;

        harness.addScript(
            "visibilityProbe",
            "function born(o) { play_timer(o, 'life', 1.0); }"
            "function action(o) {"
            "  o.local['actionCount'] = (o.local['actionCount'] || 0) + 1;"
            "  if (o.local['actionCount'] == 1) hide(o);"
            "  if (o.local['actionCount'] == 2) show(o);"
            "}"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
            "function collision(o, other) { o.local['collisionCount'] = (o.local['collisionCount'] || 0) + 1; }"
            "function draw(o) { o.local['drawCount'] = (o.local['drawCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "visibilityProbe");
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "box";
        root.collisionWith.push_back("target");
        root.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(target);

        require(harness.load().success, "runtime should load visibility project");

        HiddenTestWindow window;

        harness.update(0.25f);
        harness.draw();

        RuntimeObject& hiddenRoot =
            requireObject(harness.world, "root");

        require(!hiddenRoot.visible, "hide should mark object invisible");
        require(localValue(hiddenRoot, "actionCount") == 1.0, "hidden object should run action");
        require(localValue(hiddenRoot, "motionCount") == 1.0, "hidden object should run motion");
        require(localValue(hiddenRoot, "collisionCount") == 1.0, "hidden object should still collide");
        require(localValue(hiddenRoot, "drawCount") == 0.0, "hide should suppress JS draw");
        require(nearlyEqual(hiddenRoot.timers["life"].left, 0.75), "hidden object timers should continue");

        harness.update(0.25f);
        harness.draw();

        RuntimeObject& shownRoot =
            requireObject(harness.world, "root");

        require(shownRoot.visible, "show should mark object visible");
        require(localValue(shownRoot, "drawCount") == 1.0, "show should restore JS draw");
    }

    void testHideSuppressesDeclarativeDrawing()
    {
        RuntimeHarness harness;

        harness.addScript(
            "hideOnBorn",
            "function born(o) { hide(o); }"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["hiddenBlock"] = "hiddenBlock";

        ObjectDefinition hiddenBlock =
            objectDefinition("hiddenBlock", "hideOnBorn");
        hiddenBlock.shapeType = "block";
        hiddenBlock.size = Vector2{ 4.0f, 4.0f };
        hiddenBlock.color = GREEN;
        hiddenBlock.origin = Vector2{ 4.0f, 4.0f };
        hiddenBlock.hasOrigin = true;

        harness.addObject(root);
        harness.addObject(hiddenBlock);

        require(harness.load().success, "runtime should load hidden declarative draw project");
        require(countRenderedColor(harness, GREEN) == 0, "hide should suppress declarative shape drawing");
    }

    void testVisibleIsReadOnlyFromJavaScript()
    {
        RuntimeHarness harness;

        harness.addScript(
            "writeVisible",
            "function action(o) { o.visible = false; }"
            "function draw(o) { o.local['drawCount'] = (o.local['drawCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "writeVisible");

        harness.addObject(root);

        require(harness.load().success, "runtime should load visible readonly project");

        HiddenTestWindow window;

        harness.update();
        harness.draw();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(runtimeRoot.visible, "direct JS visible write should be ignored");
        require(localValue(runtimeRoot, "drawCount") == 1.0, "object should still draw after ignored visible write");
    }

    void testDrawCallbacksFollowStableLayerOrder()
    {
        RuntimeHarness harness;

        harness.addScript(
            "drawProbe",
            "function draw(o) {"
            "  if (global['drawOrder'] == null) global['drawOrder'] = 0;"
            "  o.local['drawOrder'] = global['drawOrder'];"
            "  global['drawOrder'] = global['drawOrder'] + 1;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "drawProbe");
        root.childResources["low"] = "low";
        root.childResources["same"] = "same";
        root.childResources["high"] = "high";

        ObjectDefinition low =
            objectDefinition("low", "drawProbe");
        low.layer = -10;

        ObjectDefinition same =
            objectDefinition("same", "drawProbe");
        same.layer = 0;

        ObjectDefinition high =
            objectDefinition("high", "drawProbe");
        high.layer = 10;

        harness.addObject(root);
        harness.addObject(low);
        harness.addObject(same);
        harness.addObject(high);

        require(harness.load().success, "runtime should load draw project");

        HiddenTestWindow window;

        harness.draw();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeLow =
            requireObject(harness.world, "low");
        RuntimeObject& runtimeSame =
            requireObject(harness.world, "same");
        RuntimeObject& runtimeHigh =
            requireObject(harness.world, "high");

        require(localValue(runtimeLow, "drawOrder") == 0.0, "lower layer should draw first");
        require(localValue(runtimeRoot, "drawOrder") < localValue(runtimeSame, "drawOrder"), "same layer should keep insertion order");
        require(localValue(runtimeHigh, "drawOrder") > localValue(runtimeSame, "drawOrder"), "higher layer should draw last");
    }

    void testDeclarativeDrawAndJsDrawShareObjectLayerOrder()
    {
        RuntimeHarness harness;

        harness.addScript(
            "paintRedPixel",
            "function draw(o) { draw_pixel(5, 5, 'red'); }"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["lowPainter"] = "lowPainter";
        root.childResources["highBlock"] = "highBlock";

        ObjectDefinition lowPainter =
            objectDefinition("lowPainter", "paintRedPixel");
        lowPainter.layer = -10;

        ObjectDefinition highBlock =
            objectDefinition("highBlock");
        highBlock.layer = 10;
        highBlock.shapeType = "block";
        highBlock.size = Vector2{ 1.0f, 1.0f };
        highBlock.origin = Vector2{ 5.0f, 5.0f };
        highBlock.hasOrigin = true;
        highBlock.color = BLUE;

        harness.addObject(root);
        harness.addObject(lowPainter);
        harness.addObject(highBlock);

        require(harness.load().success, "runtime should load draw pipeline project");

        require(countRenderedColor(harness, BLUE) == 1, "higher layer declarative draw should cover lower layer JS draw");
        require(countRenderedColor(harness, RED) == 0, "lower layer JS draw should not run in a separate final overlay pipeline");
    }

    void testObjectTimeStateAndTimers()
    {
        RuntimeHarness harness;

        harness.addScript(
            "timeProbe",
            "function born(o) { play_timer(o, 'life', 1.0); }"
            "function action(o) {"
            "  if (o.local['firstStateTime'] == null) o.local['firstStateTime'] = state_time(o);"
            "  if (o.local['firstTimerLeft'] == null) o.local['firstTimerLeft'] = timer_left(o, 'life');"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "timeProbe");
        root.initialState = "start";
        root.stateTransitions["start"].push_back("next");
        root.stateTransitions["next"];

        harness.addObject(root);

        require(harness.load().success, "runtime should load time project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(runtimeRoot.stateTime == 0.0f, "state time should start at zero");
        require(runtimeRoot.timers["life"].left == 1.0f, "timer should keep born value before first update");

        harness.update(0.25f);

        RuntimeObject& updatedRoot =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(updatedRoot, "firstStateTime"), 0.0), "action sees state time before frame increment");
        require(nearlyEqual(localValue(updatedRoot, "firstTimerLeft"), 1.0), "action sees timer before frame decrement");
        require(nearlyEqual(updatedRoot.stateTime, 0.25), "state time increments after collision phase");
        require(nearlyEqual(updatedRoot.timers["life"].left, 0.75), "timer decrements after collision phase");
    }

    void testStateMachineSingleTerminalInvalidAndAbsentStates()
    {
        RuntimeHarness harness;

        harness.addScript(
            "stateProbe",
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 1) {"
            "    o.local['initialIsIntro'] = state_current(o) == 'intro' ? 1 : 0;"
            "    o.local['initialActive'] = state_active(o, 'intro') ? 1 : 0;"
            "    o.local['initialEntered'] = state_entered(o) ? 1 : 0;"
            "    o.local['initialTime'] = state_time(o);"
            "    state_to(o, 'missing');"
            "    o.local['afterInvalidStillIntro'] = state_current(o) == 'intro' ? 1 : 0;"
            "    state_to(o, 'gameover');"
            "    o.local['afterValidIsGameover'] = state_current(o) == 'gameover' ? 1 : 0;"
            "    o.local['enteredImmediatelyAfterState'] = state_entered(o) ? 1 : 0;"
            "    o.local['timeImmediatelyAfterState'] = state_time(o);"
            "  }"
            "  if (o.local['frame'] == 2) {"
            "    o.local['terminalEnteredNextFrame'] = state_entered(o) ? 1 : 0;"
            "    o.local['terminalActive'] = state_active(o, 'gameover') ? 1 : 0;"
            "    o.local['terminalTimeSeen'] = state_time(o);"
            "    state_to(o, 'intro');"
            "    o.local['afterTerminalInvalidStillGameover'] = state_current(o) == 'gameover' ? 1 : 0;"
            "  }"
            "}"
        );

        harness.addScript(
            "singleStateProbe",
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 1) {"
            "    o.local['currentIsSolo'] = state_current(o) == 'solo' ? 1 : 0;"
            "    o.local['activeSolo'] = state_active(o, 'solo') ? 1 : 0;"
            "    o.local['enteredSolo'] = state_entered(o) ? 1 : 0;"
            "    state_to(o, 'solo');"
            "    o.local['sameStateStillSolo'] = state_current(o) == 'solo' ? 1 : 0;"
            "    o.local['selfEnteredImmediately'] = state_entered(o) ? 1 : 0;"
            "  }"
            "  if (o.local['frame'] == 2) {"
            "    o.local['selfEnteredNextFrame'] = state_entered(o) ? 1 : 0;"
            "    o.local['selfTimeSeen'] = state_time(o);"
            "  }"
            "}"
        );

        harness.addScript(
            "noStateProbe",
            "function action(o) {"
            "  o.local['currentEmpty'] = state_current(o) == '' ? 1 : 0;"
            "  o.local['activeEmptyName'] = state_active(o, '') ? 1 : 0;"
            "  o.local['entered'] = state_entered(o) ? 1 : 0;"
            "  o.local['time'] = state_time(o);"
            "  state_to(o, 'anything');"
            "  o.local['stillEmpty'] = state_current(o) == '' ? 1 : 0;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "stateProbe");
        root.childResources["single"] = "single";
        root.childResources["none"] = "none";
        root.initialState = "intro";
        root.stateTransitions["intro"] = { "gameover" };
        root.stateTransitions["gameover"] = {};

        ObjectDefinition single =
            objectDefinition("single", "singleStateProbe");
        single.initialState = "solo";
        single.stateTransitions["solo"] = { "solo" };

        ObjectDefinition none =
            objectDefinition("none", "noStateProbe");

        harness.addObject(root);
        harness.addObject(single);
        harness.addObject(none);

        require(harness.load().success, "runtime should load state characterization project");

        harness.update(0.25f);

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeSingle =
            requireObject(harness.world, "single");
        RuntimeObject& runtimeNone =
            requireObject(harness.world, "none");

        require(localValue(runtimeRoot, "initialIsIntro") == 1.0, "initial state should become current state");
        require(localValue(runtimeRoot, "initialActive") == 1.0, "state_active should compare current state by name");
        require(localValue(runtimeRoot, "initialEntered") == 1.0, "initial state should be entered during first frame");
        require(nearlyEqual(localValue(runtimeRoot, "initialTime"), 0.0), "state_time should be zero before first time update");
        require(localValue(runtimeRoot, "afterInvalidStillIntro") == 1.0, "invalid state transition should leave state unchanged");
        require(localValue(runtimeRoot, "afterValidIsGameover") == 1.0, "valid transition should change current state");
        require(localValue(runtimeRoot, "enteredImmediatelyAfterState") == 0.0, "state_entered should not be true in the same frame as state_to()");
        require(nearlyEqual(localValue(runtimeRoot, "timeImmediatelyAfterState"), 0.0), "state_to() should reset state_time immediately");

        require(localValue(runtimeSingle, "currentIsSolo") == 1.0, "single-state machine should set its initial state");
        require(localValue(runtimeSingle, "activeSolo") == 1.0, "single-state machine should report active state");
        require(localValue(runtimeSingle, "enteredSolo") == 1.0, "single-state machine should enter during first frame");
        require(localValue(runtimeSingle, "sameStateStillSolo") == 1.0, "declared self-transition should leave current state name unchanged");
        require(localValue(runtimeSingle, "selfEnteredImmediately") == 0.0, "declared self-transition should not be entered immediately");

        require(localValue(runtimeNone, "currentEmpty") == 1.0, "object without states should report empty current state");
        require(localValue(runtimeNone, "activeEmptyName") == 0.0, "object without states should not report an active empty state");
        require(localValue(runtimeNone, "entered") == 0.0, "object without states should not report entered");
        require(nearlyEqual(localValue(runtimeNone, "time"), 0.0), "object without states should report zero state_time");
        require(localValue(runtimeNone, "stillEmpty") == 1.0, "state_to() should not create states when no machine exists");

        harness.update(0.25f);

        RuntimeObject& afterSecondFrameRoot =
            requireObject(harness.world, "root");

        RuntimeObject& afterSecondFrameSingle =
            requireObject(harness.world, "single");

        require(localValue(afterSecondFrameRoot, "terminalEnteredNextFrame") == 1.0, "state_entered should fire on the frame after state_to()");
        require(localValue(afterSecondFrameRoot, "terminalActive") == 1.0, "terminal state should remain active");
        require(nearlyEqual(localValue(afterSecondFrameRoot, "terminalTimeSeen"), 0.0), "first full frame after transition should see zero state_time");
        require(localValue(afterSecondFrameRoot, "afterTerminalInvalidStillGameover") == 1.0, "terminal state without next should reject outgoing transition");
        require(localValue(afterSecondFrameSingle, "selfEnteredNextFrame") == 1.0, "declared self-transition should be observable on next frame");
        require(nearlyEqual(localValue(afterSecondFrameSingle, "selfTimeSeen"), 0.0), "self-transition should reset state_time");
    }

    void testStateTransitionsFromMotionAndCollisionEnterNextFrame()
    {
        RuntimeHarness harness;

        harness.addScript(
            "stateFromMotion",
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 2) {"
            "    o.local['enteredAfterMotion'] = state_entered(o) ? 1 : 0;"
            "    o.local['currentAfterMotion'] = state_current(o) == 'b' ? 1 : 0;"
            "    o.local['timeAfterMotion'] = state_time(o);"
            "  }"
            "}"
            "function motion(o) {"
            "  if (o.local['frame'] == 1) {"
            "    state_to(o, 'b');"
            "    o.local['enteredInMotion'] = state_entered(o) ? 1 : 0;"
            "  }"
            "}"
        );

        harness.addScript(
            "stateFromCollision",
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 2) {"
            "    o.local['enteredAfterCollision'] = state_entered(o) ? 1 : 0;"
            "    o.local['currentAfterCollision'] = state_current(o) == 'b' ? 1 : 0;"
            "    o.local['timeAfterCollision'] = state_time(o);"
            "  }"
            "}"
            "function collision(o, other) {"
            "  if (o.local['frame'] == 1) {"
            "    state_to(o, 'b');"
            "    o.local['enteredInCollision'] = state_entered(o) ? 1 : 0;"
            "  }"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["motionProbe"] = "motionProbe";
        root.childResources["collisionProbe"] = "collisionProbe";
        root.childResources["target"] = "target";

        ObjectDefinition motionProbe =
            objectDefinition("motionProbe", "stateFromMotion");
        motionProbe.initialState = "a";
        motionProbe.stateTransitions["a"] = { "b" };
        motionProbe.stateTransitions["b"] = {};

        ObjectDefinition collisionProbe =
            objectDefinition("collisionProbe", "stateFromCollision");
        collisionProbe.initialState = "a";
        collisionProbe.stateTransitions["a"] = { "b" };
        collisionProbe.stateTransitions["b"] = {};
        collisionProbe.collisionActive = true;
        collisionProbe.collisionType = "box";
        collisionProbe.collisionWith.push_back("target");
        collisionProbe.size = Vector2{ 8.0f, 8.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 8.0f, 8.0f };

        harness.addObject(root);
        harness.addObject(motionProbe);
        harness.addObject(collisionProbe);
        harness.addObject(target);

        require(harness.load().success, "runtime should load state phase project");

        harness.update(0.25f);
        harness.update(0.25f);

        RuntimeObject& runtimeMotion =
            requireObject(harness.world, "motionProbe");
        RuntimeObject& runtimeCollision =
            requireObject(harness.world, "collisionProbe");

        require(localValue(runtimeMotion, "enteredInMotion") == 0.0, "motion transition should not enter immediately");
        require(localValue(runtimeMotion, "enteredAfterMotion") == 1.0, "motion transition should enter on next frame");
        require(localValue(runtimeMotion, "currentAfterMotion") == 1.0, "motion transition should change current state");
        require(nearlyEqual(localValue(runtimeMotion, "timeAfterMotion"), 0.0), "motion transition first full frame should see zero time");

        require(localValue(runtimeCollision, "enteredInCollision") == 0.0, "collision transition should not enter immediately");
        require(localValue(runtimeCollision, "enteredAfterCollision") == 1.0, "collision transition should enter on next frame");
        require(localValue(runtimeCollision, "currentAfterCollision") == 1.0, "collision transition should change current state");
        require(nearlyEqual(localValue(runtimeCollision, "timeAfterCollision"), 0.0), "collision transition first full frame should see zero time");
    }

    void testAttachmentsApplyAfterMotionBeforeCollision()
    {
        RuntimeHarness harness;

        harness.addScript(
            "parentMotion",
            "function motion(o) { o.x = o.x + 10; o.y = o.y + 5; o.angle = o.angle + 15; }"
        );

        harness.addScript(
            "attachedChild",
            "function motion(o) { o.local['motionX'] = o.x; o.local['motionY'] = o.y; o.local['motionAngle'] = o.angle; }"
            "function collision(o, other) { o.local['collisionX'] = o.x; o.local['collisionY'] = o.y; o.local['collisionAngle'] = o.angle; }"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["parent"] = "parent";

        ObjectDefinition parent =
            objectDefinition("parent", "parentMotion");
        parent.childResources["child"] = "child";
        parent.origin = Vector2{ 30.0f, 40.0f };
        parent.hasOrigin = true;
        parent.angle = 20.0f;

        ObjectDefinition child =
            objectDefinition("child", "attachedChild");
        child.offset = Vector2{ 3.0f, 4.0f };
        child.hasOffset = true;
        child.attachOnCreate = true;
        child.attachFollowX = true;
        child.attachFollowY = false;
        child.attachFollowAngle = true;
        child.collisionActive = true;
        child.collisionType = "box";
        child.collisionWith.push_back("parent");
        child.size = Vector2{ 10.0f, 10.0f };
        child.group = "child";

        parent.group = "parent";
        parent.collisionType = "box";
        parent.size = Vector2{ 30.0f, 30.0f };

        harness.addObject(root);
        harness.addObject(parent);
        harness.addObject(child);

        require(harness.load().success, "runtime should load attachment project");

        harness.update();

        RuntimeObject& runtimeChild =
            requireObject(harness.world, "child");
        RuntimeObject& runtimeParent =
            requireObject(harness.world, "parent");

        require(nearlyEqual(localValue(runtimeChild, "motionX"), runtimeChild.origin.x), "child motion sees pre-attachment x");
        require(nearlyEqual(localValue(runtimeChild, "collisionX"), runtimeChild.position.x), "collision sees post-attachment x");
        require(nearlyEqual(runtimeChild.position.x, runtimeParent.position.x + runtimeChild.originalOffset.x), "attached x follows parent plus stored original offset");
        require(nearlyEqual(runtimeChild.position.y, runtimeChild.origin.y), "unattached y remains unchanged");
        require(nearlyEqual(runtimeChild.angle, 35.0), "attached angle follows parent angle");
    }

    void testFindByRuntimeIdNameDuplicatesAndPendingLookup()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnOnce",
            "function action(o) { if (o.local['done'] == null) { o.local['done'] = 1; spawn(o, 'dup'); } }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnOnce");
        root.childResources["dup"] = "dup";

        ObjectDefinition duplicate =
            objectDefinition("dup");
        duplicate.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(duplicate);

        require(harness.load().success, "runtime should load identity project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        const std::string rootRuntimeId =
            runtimeRoot.runtimeId;

        require(harness.world.findByRuntimeId(runtimeRoot.runtimeId) == &runtimeRoot, "findByRuntimeId should be exact");

        harness.update();
        RuntimeObject& firstDuplicate =
            requireObject(harness.world, "dup");
        const std::string firstDuplicateRuntimeId =
            firstDuplicate.runtimeId;

        RuntimeObject* rootAfterSpawn =
            harness.world.findByRuntimeId(rootRuntimeId);
        require(rootAfterSpawn != nullptr, "root should still exist after first spawn flush");
        harness.world.spawn(
            *rootAfterSpawn,
            rootAfterSpawn->childResources.at("dup"),
            harness.scripts
        );

        require(harness.world.findByName("dup")->runtimeId == firstDuplicateRuntimeId, "findByName should return first live duplicate before pending flush");

        harness.update();

        require(harness.world.findByName("dup")->runtimeId == firstDuplicateRuntimeId, "findByName should keep first live duplicate after second spawn");

        RuntimeObject* firstDuplicateAfterSecondSpawn =
            harness.world.findByRuntimeId(firstDuplicateRuntimeId);
        require(firstDuplicateAfterSecondSpawn != nullptr, "first duplicate should still exist before cleanup");
        firstDuplicateAfterSecondSpawn->alive = false;
        harness.update();

        require(harness.world.findByRuntimeId(firstDuplicateRuntimeId) == nullptr, "dead object should not be found after cleanup");
    }

    void testScriptErrorsAreLoggedAndRuntimeContinues()
    {
        RuntimeHarness harness;

        harness.addScript(
            "throwEverywhere",
            "function born(o) { throw new Error('born failure'); }"
            "function action(o) { o.local['actionBeforeThrow'] = 1; throw new Error('action failure'); }"
            "function motion(o) { o.local['motionBeforeThrow'] = 1; throw new Error('motion failure'); }"
            "function collision(o, other) { o.local['collisionBeforeThrow'] = 1; throw new Error('collision failure'); }"
            "function draw(o) { o.local['drawBeforeThrow'] = 1; throw new Error('draw failure'); }"
            "function dead(o) { o.local['deadBeforeThrow'] = 1; throw new Error('dead failure'); }"
        );

        ObjectDefinition root =
            objectDefinition("root", "throwEverywhere");
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "box";
        root.collisionWith.push_back("target");
        root.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(target);

        RuntimeLoadResult loadResult =
            harness.load();

        require(loadResult.success, "script exception in born should not fail runtime load currently");

        HiddenTestWindow window;

        harness.update();
        harness.draw();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(localValue(runtimeRoot, "actionBeforeThrow") == 1.0, "action should apply object changes before logged exception cleanup");
        require(localValue(runtimeRoot, "motionBeforeThrow") == 1.0, "motion should apply object changes before logged exception cleanup");
        require(localValue(runtimeRoot, "collisionBeforeThrow") == 1.0, "collision should apply object changes before logged exception cleanup");
        require(localValue(runtimeRoot, "drawBeforeThrow") == 1.0, "draw should apply object changes before logged exception cleanup");

        runtimeRoot.alive = false;
        harness.update();

        require(harness.world.findByName("root") == nullptr, "dead exception should not prevent cleanup");
    }

    void testAutomaticInstantiationCycleFailsRuntimeLoad()
    {
        RuntimeHarness harness;

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["a"] = "a";

        ObjectDefinition a =
            objectDefinition("a");
        a.childResources["b"] = "b";

        ObjectDefinition b =
            objectDefinition("b");
        b.childResources["a"] = "a";

        harness.addObject(root);
        harness.addObject(a);
        harness.addObject(b);

        RuntimeLoadResult result =
            harness.load();

        require(!result.success, "runtime load should reject automatic instantiation cycle");
        require(result.diagnostics.hasErrors(), "automatic instantiation cycle should produce diagnostics");
    }

    void testManualInstantiationCycleDoesNotFailRuntimeLoad()
    {
        RuntimeHarness harness;

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["a"] = "a";

        ObjectDefinition a =
            objectDefinition("a");
        a.spawnMode = "manual";
        a.childResources["b"] = "b";

        ObjectDefinition b =
            objectDefinition("b");
        b.spawnMode = "manual";
        b.childResources["a"] = "a";

        harness.addObject(root);
        harness.addObject(a);
        harness.addObject(b);

        RuntimeLoadResult result =
            harness.load();

        require(result.success, "runtime load should accept manual-only instantiation cycles");
    }

    void testMixedManualEdgeDoesNotFailRuntimeLoad()
    {
        RuntimeHarness harness;

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["a"] = "a";

        ObjectDefinition a =
            objectDefinition("a");
        a.childResources["b"] = "b";

        ObjectDefinition b =
            objectDefinition("b");
        b.spawnMode = "manual";
        b.childResources["a"] = "a";

        harness.addObject(root);
        harness.addObject(a);
        harness.addObject(b);

        RuntimeLoadResult result =
            harness.load();

        require(result.success, "runtime load should accept a cycle broken by a manual edge");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "initial load born order and identity", testInitialLoadBornOrderAndIdentity },
        { "spawn during born is stabilized before load returns", testSpawnDuringBornIsStabilizedBeforeLoadReturns },
        { "recursive spawn during born is stabilized before load returns", testRecursiveSpawnDuringBornIsStabilizedBeforeLoadReturns },
        { "spawn during born limit fails load", testSpawnDuringBornLimitFailsLoad },
        { "spawn during action motion and collision phases", testSpawnDuringActionMotionAndCollisionPhases },
        { "kill during action motion and collision is terminal", testKillDuringActionMotionAndCollisionIsTerminal },
        { "alive is read-only from JavaScript", testAliveIsReadOnlyFromJavaScript },
        { "JS flat runtime properties are mutable", testJsFlatRuntimePropertiesAreMutable },
        { "JS motion nested properties are read-only snapshot", testJsMotionNestedPropertiesAreReadOnlySnapshot },
        { "JS metadata and identity are readable but not applied back", testJsMetadataAndIdentityAreReadableButNotAppliedBack },
        { "script module variables are shared between instances", testScriptModuleVariablesAreSharedBetweenInstances },
        { "collision callback receives concrete other reference", testCollisionCallbackReceivesConcreteOtherReference },
        { "public scripting functions are registered", testPublicScriptingFunctionsAreRegistered },
        { "keep_only prevents other objects from resurrecting", testKeepOnlyPreventsOtherObjectsFromResurrecting },
        { "hide suppresses draw but keeps runtime phases and show restores draw", testHideSuppressesDrawButKeepsRuntimePhasesAndShowRestoresDraw },
        { "hide suppresses declarative drawing", testHideSuppressesDeclarativeDrawing },
        { "visible is read-only from JavaScript", testVisibleIsReadOnlyFromJavaScript },
        { "draw callbacks follow stable layer order", testDrawCallbacksFollowStableLayerOrder },
        { "declarative draw and JS draw share object layer order", testDeclarativeDrawAndJsDrawShareObjectLayerOrder },
        { "object time state and timers", testObjectTimeStateAndTimers },
        { "state machine single terminal invalid and absent states", testStateMachineSingleTerminalInvalidAndAbsentStates },
        { "state transitions from motion and collision enter next frame", testStateTransitionsFromMotionAndCollisionEnterNextFrame },
        { "attachments apply after motion before collision", testAttachmentsApplyAfterMotionBeforeCollision },
        { "find by runtime id name duplicates and pending lookup", testFindByRuntimeIdNameDuplicatesAndPendingLookup },
        { "script errors are logged and runtime continues", testScriptErrorsAreLoggedAndRuntimeContinues },
        { "automatic instantiation cycle fails runtime load", testAutomaticInstantiationCycleFailsRuntimeLoad },
        { "manual instantiation cycle does not fail runtime load", testManualInstantiationCycleDoesNotFailRuntimeLoad },
        { "mixed manual edge does not fail runtime load", testMixedManualEdgeDoesNotFailRuntimeLoad }
    };

    for (const auto& test : tests)
    {
        try
        {
            std::cout << "[RUN] " << test.first << std::endl;
            test.second();
            std::cout << "[PASS] " << test.first << std::endl;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";
            return 1;
        }
    }

    return 0;
}
