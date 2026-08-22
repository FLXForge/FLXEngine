#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/input/InputSystem.h"
#include "../../engine/runtime/RuntimeHelpers.h"
#include "../../engine/runtime/RuntimeObjectBuilder.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    constexpr float Delta = 1.0f;

    class FakeInputProvider : public PhysicalInputProvider
    {
    public:
        void setKeys(const std::set<int>& nextKeys)
        {
            previousKeys = currentKeys;
            currentKeys = nextKeys;
        }

        bool keyDown(int key) const override
        {
            return currentKeys.contains(key);
        }

        bool keyPressed(int key) const override
        {
            return currentKeys.contains(key) &&
                !previousKeys.contains(key);
        }

        bool gamepadButtonDown(int, int) const override
        {
            return false;
        }

        bool gamepadButtonPressed(int, int) const override
        {
            return false;
        }

    private:
        std::set<int> previousKeys;
        std::set<int> currentKeys;
    };

    bool nearlyEqual(float left, float right, float epsilon = 0.001f)
    {
        return std::abs(left - right) <= epsilon;
    }

    RuntimeObject mechanicsObject(
        MechanicsType type,
        float speed,
        float angle
    )
    {
        ObjectDefinition definition;
        definition.id = "object";
        definition.mechanics.type = type;
        definition.mechanics.motion.speed.start = speed;
        definition.mechanics.rotation.angle = angle;

        return RuntimeObjectBuilder::build(definition, "object", "");
    }

    struct RuntimeHarness
    {
        CompiledProject project;
        RuntimeWorld world;
        ScriptEngine scripts;
        InputSystem input;
        FakeInputProvider provider;

        RuntimeHarness()
        {
            project.rootId = "root";
            project.context.machine.video.screenWidth = 320;
            project.context.machine.video.screenHeight = 180;
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

            scripts.setInputSystem(&input);
        }

        void addScript(const std::string& id, const std::string& code)
        {
            ScriptResource script;
            script.id = id;
            script.sourceName = id + ".js";
            script.code = code;

            require(project.resources.addScript(id, script), "script should be added: " + id);
        }

        void addObject(ObjectDefinition definition)
        {
            if (definition.sourcePath.empty())
            {
                definition.sourcePath = definition.id + ".json";
            }

            require(project.resources.addObject(definition.id, definition), "object should be added: " + definition.id);
        }

        RuntimeLoadResult load()
        {
            return world.load(project, scripts);
        }

        void update(float delta = 0.1f)
        {
            scripts.setFrameDelta(delta);
            world.update(scripts, 320.0f, 180.0f, delta);
        }
    };

    RuntimeObject& requireObject(RuntimeWorld& world, const std::string& name)
    {
        RuntimeObject* object = world.findByName(name);
        require(object != nullptr, "runtime object should exist: " + name);
        return *object;
    }

    void testPolarSpeedDoesNotDoubleCountVelocity()
    {
        RuntimeObject ball =
            mechanicsObject(MechanicsType::Polar, 90.0f, 45.0f);

        RuntimeHelpers::advance(ball, Delta);
        const float initialX = ball.position.x;
        const float initialY = ball.position.y;
        const float initialDistance =
            std::sqrt(initialX * initialX + initialY * initialY);

        require(nearlyEqual(initialDistance, 90.0f), "polar advance should use initial live speed");
        require(nearlyEqual(ball.velocity.x, 0.0f), "polar advance should not write own polar speed into velocity x");
        require(nearlyEqual(ball.velocity.y, 0.0f), "polar advance should not write own polar speed into velocity y");

        ball.position = Vector2{ 0.0f, 0.0f };
        RuntimeHelpers::applySpeed(ball, 95.0f);
        RuntimeHelpers::advance(ball, Delta);
        const float boostedDistance =
            std::sqrt(ball.position.x * ball.position.x + ball.position.y * ball.position.y);

        require(nearlyEqual(boostedDistance, 95.0f), "apply_speed should set polar effective speed without double counting");
        require(nearlyEqual(ball.velocity.x, 0.0f), "apply_speed should not duplicate polar speed into velocity x");
        require(nearlyEqual(ball.velocity.y, 0.0f), "apply_speed should not duplicate polar speed into velocity y");

        ball.position = Vector2{ 0.0f, 0.0f };
        RuntimeHelpers::restoreSpeed(ball);
        RuntimeHelpers::advance(ball, Delta);
        const float restoredDistance =
            std::sqrt(ball.position.x * ball.position.x + ball.position.y * ball.position.y);

        require(nearlyEqual(restoredDistance, initialDistance), "restore_speed should restore original polar effective speed");
    }

    void testDirectApplySpeedAndRestoreSpeedDriveAxisMovement()
    {
        RuntimeObject object =
            mechanicsObject(MechanicsType::Direct, 10.0f, 0.0f);

        RuntimeHelpers::applySpeed(object, 20.0f);
        RuntimeHelpers::moveHorizontal(object, 1.0f, Delta);
        require(nearlyEqual(object.position.x, 20.0f), "direct move_horizontal should use applied live speed");

        object.position = Vector2{ 0.0f, 0.0f };
        RuntimeHelpers::restoreSpeed(object);
        RuntimeHelpers::beginMechanicsFrame(object);
        RuntimeHelpers::moveHorizontal(object, 1.0f, Delta);
        require(nearlyEqual(object.position.x, 10.0f), "direct move_horizontal should use restored live speed");
    }

    void testApplySpeedLimitStillClamps()
    {
        RuntimeObject object =
            mechanicsObject(MechanicsType::Direct, 10.0f, 0.0f);
        object.mechanicsMotion.speed.limit = 15.0f;

        RuntimeHelpers::applySpeed(object, 20.0f);
        require(nearlyEqual(object.speed, 15.0f), "apply_speed should clamp to mechanics motion speed limit");
    }

    void testAccelerationUsesDelta()
    {
        RuntimeObject ship =
            mechanicsObject(MechanicsType::Polar, 0.0f, 90.0f);
        ship.mechanicsMotion.acceleration = 100.0f;
        ship.mechanicsMotion.speed.limit = 0.0f;

        RuntimeHelpers::accelerate(ship, 1.0f, 0.5f);

        require(nearlyEqual(ship.velocity.x, 50.0f), "acceleration should add velocity using delta");
        require(nearlyEqual(ship.position.x, 25.0f), "acceleration should move by the generated velocity during the frame");
    }

    void testPolarInertiaDoesNotRotateExistingMomentum()
    {
        RuntimeObject ship =
            mechanicsObject(MechanicsType::Polar, 0.0f, 90.0f);
        ship.mechanicsMotion.acceleration = 100.0f;
        ship.mechanicsMotion.inertia = 1.0f;
        ship.mechanicsRotation.speed.start = 90.0f;

        RuntimeHelpers::accelerate(ship, 1.0f, Delta);
        RuntimeHelpers::beginMechanicsFrame(ship);
        RuntimeHelpers::rotate(ship, 1.0f, Delta);
        RuntimeHelpers::applyFreeMechanics(ship, Delta);

        require(ship.angle > 90.0f, "rotation should change orientation");
        require(nearlyEqual(ship.velocity.x, 100.0f), "free inertia should preserve existing momentum x");
        require(nearlyEqual(ship.velocity.y, 0.0f), "free inertia should not rotate existing momentum y");
    }

    void testInertiaZeroClearsStaleVelocity()
    {
        RuntimeObject object =
            mechanicsObject(MechanicsType::Direct, 10.0f, 0.0f);
        object.velocity = Vector2{ 40.0f, 0.0f };
        object.mechanicsMotion.horizontal.inertia = 0.0f;
        object.mechanicsMotion.vertical.inertia = 0.0f;

        RuntimeHelpers::applyFreeMechanics(object, Delta);

        require(nearlyEqual(object.velocity.x, 0.0f), "direct free mechanics with inertia zero should clear stale velocity x");
        require(nearlyEqual(object.velocity.y, 0.0f), "direct free mechanics with inertia zero should clear stale velocity y");

        object.position = Vector2{ 0.0f, 0.0f };
        RuntimeHelpers::advance(object, Delta);
        require(nearlyEqual(object.position.x, 0.0f), "cleared stale velocity should not reappear through advance");
    }

    void testInertiaPartialAndPerpetual()
    {
        RuntimeObject partial =
            mechanicsObject(MechanicsType::Direct, 0.0f, 0.0f);
        partial.velocity = Vector2{ 100.0f, 0.0f };
        partial.mechanicsMotion.horizontal.inertia = 0.25f;
        partial.mechanicsMotion.vertical.inertia = 0.25f;

        RuntimeHelpers::applyFreeMechanics(partial, Delta);
        require(partial.velocity.x > 0.0f && partial.velocity.x < 100.0f, "partial inertia should reduce velocity");

        RuntimeObject perpetual =
            mechanicsObject(MechanicsType::Direct, 0.0f, 0.0f);
        perpetual.velocity = Vector2{ 100.0f, 0.0f };
        perpetual.mechanicsMotion.horizontal.inertia = 1.0f;
        perpetual.mechanicsMotion.vertical.inertia = 1.0f;

        RuntimeHelpers::applyFreeMechanics(perpetual, Delta);
        require(nearlyEqual(perpetual.velocity.x, 100.0f), "inertia one should preserve velocity");
    }

    void testInheritVelocityCopyAndCompose()
    {
        RuntimeHarness harness;

        ObjectDefinition root;
        root.id = "root";
        root.childResources["fragment"] = "fragment";
        root.childResources["laser"] = "laser";

        ObjectDefinition fragment;
        fragment.id = "fragment";
        fragment.spawnMode = "manual";
        fragment.mechanics.type = MechanicsType::Direct;
        fragment.mechanics.motion.horizontal.inertia = 1.0f;
        fragment.mechanics.motion.vertical.inertia = 1.0f;
        fragment.inherit.creationVelocity = InheritCreationMode::Copy;

        ObjectDefinition laser;
        laser.id = "laser";
        laser.spawnMode = "manual";
        laser.mechanics.type = MechanicsType::Polar;
        laser.mechanics.motion.speed.start = 300.0f;
        laser.mechanics.motion.inertia = 1.0f;
        laser.mechanics.rotation.angle = 90.0f;
        laser.inherit.creationVelocity = InheritCreationMode::Compose;

        harness.addObject(root);
        harness.addObject(fragment);
        harness.addObject(laser);

        require(harness.load().success, "runtime should load inherit velocity project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        runtimeRoot.velocity = Vector2{ 20.0f, 5.0f };

        harness.world.spawn(runtimeRoot, "fragment", harness.scripts);
        harness.world.spawn(runtimeRoot, "laser", harness.scripts);
        harness.update(0.0f);

        RuntimeObject& runtimeFragment =
            requireObject(harness.world, "fragment");
        RuntimeObject& runtimeLaser =
            requireObject(harness.world, "laser");

        require(nearlyEqual(runtimeFragment.velocity.x, 20.0f), "copy should copy parent velocity x");
        require(nearlyEqual(runtimeFragment.velocity.y, 5.0f), "copy should copy parent velocity y");
        require(nearlyEqual(runtimeLaser.speed, 300.0f), "compose should preserve laser own polar speed");
        require(nearlyEqual(runtimeLaser.velocity.x, 20.0f), "compose should add parent velocity x as inherited momentum");
        require(nearlyEqual(runtimeLaser.velocity.y, 5.0f), "compose should add parent velocity y as inherited momentum");

        RuntimeHelpers::advance(runtimeLaser, Delta);
        require(nearlyEqual(runtimeLaser.position.x, 320.0f), "laser should move by own polar speed plus inherited x velocity");
        require(nearlyEqual(runtimeLaser.position.y, 5.0f), "laser should include inherited y velocity without altering own polar direction");
    }

    void testInputDirectionSignsAndTwoWayProjection()
    {
        RuntimeHarness harness;

        harness.project.context.machine.input.players = 1;
        harness.project.context.machine.input.directions = {
            InputDirectionDefinition{ "4way", "last", 0.0f },
            InputDirectionDefinition{ "2way", "last", 0.0f }
        };

        harness.input.configure(harness.project.context.machine.input);
        harness.input.setPhysicalInputProvider(&harness.provider);
        harness.input.loadMappingContent(
            "controls.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.down=KEY_S\n"
            "players.1.directions.0.left=KEY_A\n"
            "players.1.directions.0.right=KEY_D\n"
            "players.1.directions.1.negative=KEY_Q\n"
            "players.1.directions.1.positive=KEY_E\n"
        );

        harness.addScript(
            "inputProbe",
            "const MOVE = direction(0);"
            "const THROTTLE = direction(1);"
            "function action(o) {"
            "  o.local['horizontal'] = input_direction(o, MOVE, HORIZONTAL);"
            "  o.local['vertical'] = input_direction(o, MOVE, VERTICAL);"
            "  o.local['twoWayHorizontal'] = input_direction(o, THROTTLE, HORIZONTAL);"
            "  o.local['twoWayVertical'] = input_direction(o, THROTTLE, VERTICAL);"
            "  o.local['system'] = input_direction(system(), MOVE, HORIZONTAL);"
            "}"
        );

        ObjectDefinition root;
        root.id = "root";
        root.controlPlayer = 1;
        root.resolvedScriptPaths.push_back("inputProbe");

        harness.addObject(root);
        require(harness.load().success, "runtime should load input direction probe");
        RuntimeObject& rootObject = requireObject(harness.world, "root");

        harness.provider.setKeys({ KEY_D, KEY_W, KEY_E });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), 1.0f), "4way right should be positive horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), 1.0f), "4way up should be positive vertical");
        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayHorizontal"]), 1.0f), "2way positive should project as positive horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayVertical"]), 1.0f), "2way positive should project as positive vertical");
        require(nearlyEqual(static_cast<float>(rootObject.local["system"]), 0.0f), "system subject should reject directional intent");

        harness.provider.setKeys({ KEY_A, KEY_S, KEY_Q });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), -1.0f), "4way left should be negative horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), -1.0f), "4way down should be negative vertical");
        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayHorizontal"]), -1.0f), "2way negative should project as negative horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayVertical"]), -1.0f), "2way negative should project as negative vertical");

        harness.provider.setKeys({});
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), 0.0f), "neutral should return zero horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), 0.0f), "neutral should return zero vertical");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "polar speed does not double count velocity", testPolarSpeedDoesNotDoubleCountVelocity },
        { "direct apply_speed and restore_speed drive axis movement", testDirectApplySpeedAndRestoreSpeedDriveAxisMovement },
        { "apply_speed limit still clamps", testApplySpeedLimitStillClamps },
        { "acceleration uses delta", testAccelerationUsesDelta },
        { "polar inertia does not rotate existing momentum", testPolarInertiaDoesNotRotateExistingMomentum },
        { "inertia zero clears stale velocity", testInertiaZeroClearsStaleVelocity },
        { "inertia partial and perpetual", testInertiaPartialAndPerpetual },
        { "inherit velocity copy and compose", testInheritVelocityCopyAndCompose },
        { "input_direction signs and 2way projection", testInputDirectionSignsAndTwoWayProjection }
    };

    for (const auto& test : tests)
    {
        try
        {
            test.second();
            std::cout << "[PASS] " << test.first << "\n";
        }
        catch (const std::exception& error)
        {
            std::cerr << "[FAIL] " << test.first << ": " << error.what() << "\n";
            return 1;
        }
    }

    return 0;
}
