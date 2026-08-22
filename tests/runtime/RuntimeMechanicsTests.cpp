#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/debug/Logger.h"
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

    const ObjectDefinition& requireCompiledObject(
        const CompiledProject& project,
        const std::string& id
    )
    {
        const ObjectDefinition* object =
            project.resources.findObject(id);

        require(object != nullptr, "compiled object should exist: " + id);
        return *object;
    }

    const ObjectDefinition& requireCompiledObjectBySource(
        const CompiledProject& project,
        const std::string& sourcePath
    )
    {
        for (const auto& pair : project.resources.allObjects())
        {
            if (pair.second.sourcePath.find(sourcePath) != std::string::npos)
            {
                return pair.second;
            }
        }

        throw std::runtime_error("compiled object source should exist: " + sourcePath);
    }

    CompilationResult compileMechanicsProject(
        const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& files
    )
    {
        const std::filesystem::path root =
            testRoot() / name;

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Mechanics\n"
            "path=game\n"
            "root=root\n"
        );

        for (const auto& file : files)
        {
            writeFile(
                root / "game" / file.first,
                file.second
            );
        }

        return compile(root / "game.flx");
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

    void testPolarApplyAndRestorePreserveAdditionalVelocity()
    {
        RuntimeObject laser =
            mechanicsObject(MechanicsType::Polar, 300.0f, 90.0f);
        laser.velocity = Vector2{ 20.0f, 5.0f };

        RuntimeHelpers::applySpeed(laser, 400.0f);

        require(nearlyEqual(laser.speed, 400.0f), "polar apply_speed should update own polar speed");
        require(nearlyEqual(laser.velocity.x, 20.0f), "polar apply_speed should preserve additional velocity x");
        require(nearlyEqual(laser.velocity.y, 5.0f), "polar apply_speed should preserve additional velocity y");

        RuntimeHelpers::advance(laser, Delta);

        require(nearlyEqual(laser.position.x, 420.0f), "polar advance should add own speed and preserved velocity once");
        require(nearlyEqual(laser.position.y, 5.0f), "polar advance should preserve inherited vertical velocity");

        RuntimeHelpers::restoreSpeed(laser);

        require(nearlyEqual(laser.speed, 300.0f), "polar restore_speed should restore own polar speed");
        require(nearlyEqual(laser.velocity.x, 20.0f), "polar restore_speed should preserve additional velocity x");
        require(nearlyEqual(laser.velocity.y, 5.0f), "polar restore_speed should preserve additional velocity y");
    }

    void testApplySpeedLimitStillClamps()
    {
        RuntimeObject object =
            mechanicsObject(MechanicsType::Direct, 10.0f, 0.0f);
        object.mechanicsMotion.speed.limit = 15.0f;

        RuntimeHelpers::applySpeed(object, 20.0f);
        require(nearlyEqual(object.speed, 15.0f), "apply_speed should clamp to mechanics motion speed limit");
    }

    void testApplyVelocitySetsLinearVector()
    {
        RuntimeObject object =
            mechanicsObject(MechanicsType::Direct, 0.0f, 10.0f);
        object.rotationSpeed = 30.0f;
        object.mechanicsMotion.speed.limit = 0.0f;

        RuntimeHelpers::applyVelocity(object, 90.0f, 50.0f);

        require(nearlyEqual(object.velocity.x, 50.0f), "apply_velocity should set velocity x from direction");
        require(nearlyEqual(object.velocity.y, 0.0f), "apply_velocity should set velocity y from direction");
        require(nearlyEqual(object.angle, 10.0f), "apply_velocity should not modify angle");
        require(nearlyEqual(object.rotationSpeed, 30.0f), "apply_velocity should not modify rotation speed");
        require(nearlyEqual(object.speed, 0.0f), "apply_velocity should not modify live speed");
    }

    void testApplyVelocityLimit()
    {
        RuntimeObject object =
            mechanicsObject(MechanicsType::Direct, 0.0f, 0.0f);
        object.mechanicsMotion.speed.limit = 100.0f;

        Logger::setConsoleEnabled(true);
        StreamCapture capture;

        RuntimeHelpers::applyVelocity(object, 90.0f, 200.0f);

        const float magnitude =
            std::sqrt(
                object.velocity.x * object.velocity.x +
                object.velocity.y * object.velocity.y
            );

        require(nearlyEqual(magnitude, 100.0f), "apply_velocity should clamp vector magnitude to speed limit");
        require(capture.output.str().find("apply_velocity requested value exceeds mechanics.motion.speed.limit") != std::string::npos, "apply_velocity should warn when speed exceeds limit");

        RuntimeObject unlimited =
            mechanicsObject(MechanicsType::Direct, 0.0f, 0.0f);
        unlimited.mechanicsMotion.speed.limit = 0.0f;

        RuntimeHelpers::applyVelocity(unlimited, 90.0f, 200.0f);

        const float unlimitedMagnitude =
            std::sqrt(
                unlimited.velocity.x * unlimited.velocity.x +
                unlimited.velocity.y * unlimited.velocity.y
            );

        require(nearlyEqual(unlimitedMagnitude, 200.0f), "apply_velocity should treat limit zero as unlimited");
    }

    void testCommonInertiaResolvesToAxes()
    {
        CompilationResult result =
            compileMechanicsProject(
                "runtime-mechanics-common-inertia",
                {
                    {
                        "root.json",
                        "{\n"
                        "  \"children\": {\n"
                        "    \"common\": { \"like\": \"common\" },\n"
                        "    \"horizontal\": { \"like\": \"horizontal\" },\n"
                        "    \"vertical\": { \"like\": \"vertical\" }\n"
                        "  }\n"
                        "}\n"
                    },
                    {
                        "common.json",
                        "{\n"
                        "  \"mechanics\": {\n"
                        "    \"type\": \"direct\",\n"
                        "    \"motion\": { \"inertia\": 0.82 }\n"
                        "  }\n"
                        "}\n"
                    },
                    {
                        "horizontal.json",
                        "{\n"
                        "  \"mechanics\": {\n"
                        "    \"type\": \"direct\",\n"
                        "    \"motion\": {\n"
                        "      \"inertia\": 0.82,\n"
                        "      \"horizontal\": { \"inertia\": 0.3 }\n"
                        "    }\n"
                        "  }\n"
                        "}\n"
                    },
                    {
                        "vertical.json",
                        "{\n"
                        "  \"mechanics\": {\n"
                        "    \"type\": \"direct\",\n"
                        "    \"motion\": {\n"
                        "      \"inertia\": 0.82,\n"
                        "      \"vertical\": { \"inertia\": 0.4 }\n"
                        "    }\n"
                        "  }\n"
                        "}\n"
                    }
                }
            );

        require(result.success, "common inertia project should compile");

        const ObjectDefinition& common =
            requireCompiledObjectBySource(result.project, "common.json");
        const ObjectDefinition& horizontal =
            requireCompiledObjectBySource(result.project, "horizontal.json");
        const ObjectDefinition& vertical =
            requireCompiledObjectBySource(result.project, "vertical.json");

        RuntimeObject commonObject =
            RuntimeObjectBuilder::build(common, "common", "root");
        RuntimeObject horizontalObject =
            RuntimeObjectBuilder::build(horizontal, "horizontal", "root");
        RuntimeObject verticalObject =
            RuntimeObjectBuilder::build(vertical, "vertical", "root");

        require(nearlyEqual(commonObject.mechanicsMotion.horizontal.inertia, 0.82f), "common motion.inertia should apply to horizontal");
        require(nearlyEqual(commonObject.mechanicsMotion.vertical.inertia, 0.82f), "common motion.inertia should apply to vertical");
        require(nearlyEqual(horizontalObject.mechanicsMotion.horizontal.inertia, 0.3f), "horizontal override should replace common inertia");
        require(nearlyEqual(horizontalObject.mechanicsMotion.vertical.inertia, 0.82f), "horizontal override should keep common vertical inertia");
        require(nearlyEqual(verticalObject.mechanicsMotion.horizontal.inertia, 0.82f), "vertical override should keep common horizontal inertia");
        require(nearlyEqual(verticalObject.mechanicsMotion.vertical.inertia, 0.4f), "vertical override should replace common inertia");

        commonObject.velocity = Vector2{ 100.0f, 50.0f };
        RuntimeHelpers::applyFreeMechanics(commonObject, Delta);

        require(commonObject.position.x > 0.0f, "common inertia direct object should move freely on x");
        require(commonObject.position.y > 0.0f, "common inertia direct object should move freely on y");
        require(commonObject.velocity.x > 0.0f && commonObject.velocity.x < 100.0f, "common inertia should reduce x progressively");
        require(commonObject.velocity.y > 0.0f && commonObject.velocity.y < 50.0f, "common inertia should reduce y progressively");
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

    void testAccelerateThenAdvanceDoesNotDoubleCountMomentum()
    {
        RuntimeObject ship =
            mechanicsObject(MechanicsType::Polar, 0.0f, 0.0f);
        ship.mechanicsMotion.acceleration = 100.0f;

        RuntimeHelpers::accelerate(ship, 1.0f, Delta);
        require(nearlyEqual(ship.position.y, -100.0f), "accelerate should move by generated momentum");

        RuntimeHelpers::beginMechanicsFrame(ship);
        RuntimeHelpers::advance(ship, Delta);

        require(nearlyEqual(ship.position.x, 0.0f), "advance should not add extra polar movement after acceleration");
        require(nearlyEqual(ship.position.y, -200.0f), "advance should continue with existing momentum once");
    }

    void testAccelerateRotateThenAdvanceKeepsMomentumDirection()
    {
        RuntimeObject ship =
            mechanicsObject(MechanicsType::Polar, 0.0f, 0.0f);
        ship.mechanicsMotion.acceleration = 100.0f;
        ship.mechanicsRotation.speed.start = 90.0f;

        RuntimeHelpers::accelerate(ship, 1.0f, Delta);
        RuntimeHelpers::beginMechanicsFrame(ship);
        RuntimeHelpers::rotate(ship, 1.0f, Delta);
        RuntimeHelpers::advance(ship, Delta);

        require(nearlyEqual(ship.angle, 90.0f), "rotate should change orientation");
        require(nearlyEqual(ship.velocity.x, 0.0f), "rotation should not rotate existing momentum x");
        require(nearlyEqual(ship.velocity.y, -100.0f), "rotation should not rotate existing momentum y");
        require(nearlyEqual(ship.position.x, 0.0f), "advance after rotate should not add movement in new orientation");
        require(nearlyEqual(ship.position.y, -200.0f), "advance after rotate should keep previous momentum direction");
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
        root.mechanics.motion.inertia = 1.0f;

        ObjectDefinition fragment;
        fragment.id = "fragment";
        fragment.spawnMode = "manual";
        fragment.mechanics.type = MechanicsType::Direct;
        fragment.mechanics.motion.inertia = 1.0f;
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

    void testAsteroidsFragmentInheritsVelocityAndKeepsDirection()
    {
        CompilationResult result =
            compileMechanicsProject(
                "runtime-mechanics-asteroids-fragment",
                {
                    {
                        "root.json",
                        "{\n"
                        "  \"children\": {\n"
                        "    \"fragment\": {\n"
                        "      \"like\": \"fragment\",\n"
                        "      \"spawn\": \"manual\"\n"
                        "    }\n"
                        "  }\n"
                        "}\n"
                    },
                    {
                        "fragment.json",
                        "{\n"
                        "  \"mechanics\": {\n"
                        "    \"type\": \"direct\",\n"
                        "    \"motion\": { \"inertia\": 0.82 },\n"
                        "    \"rotation\": { \"speed\": { \"start\": 30 } }\n"
                        "  },\n"
                        "  \"inherit\": {\n"
                        "    \"creation\": { \"velocity\": \"copy\" }\n"
                        "  },\n"
                        "  \"behavior\": { \"scripts\": [\"fragment\"] }\n"
                        "}\n"
                    },
                    {
                        "fragment.js",
                        "function born(fragment) {\n"
                        "  fragment.rotationSpeed = 180;\n"
                        "}\n"
                    }
                }
            );

        require(result.success, "fragment mechanics project should compile");

        RuntimeHarness harness;
        harness.project =
            result.project;
        harness.scripts.setFindObjectFunction(
            [&harness](const std::string& name)
            {
                return harness.world.findByName(name);
            }
        );
        harness.scripts.setFindObjectByIdFunction(
            [&harness](const std::string& runtimeId)
            {
                return harness.world.findByRuntimeId(runtimeId);
            }
        );
        harness.scripts.setSpawnObjectFunction(
            [&harness](RuntimeObject& source, const std::string& resourceId)
            {
                harness.world.spawn(source, resourceId, harness.scripts);
            }
        );

        require(harness.load().success, "fragment runtime should load");

        RuntimeObject& root =
            requireObject(harness.world, "root");
        root.velocity = Vector2{ 100.0f, 50.0f };
        root.mechanicsMotion.horizontal.inertia = 1.0f;
        root.mechanicsMotion.vertical.inertia = 1.0f;

        const auto fragmentResource =
            root.childResources.find("fragment");
        require(fragmentResource != root.childResources.end(), "root should expose fragment child resource");

        harness.world.spawn(root, fragmentResource->second, harness.scripts);
        harness.update(0.0f);

        RuntimeObject& fragment =
            requireObject(harness.world, "fragment");

        require(nearlyEqual(fragment.velocity.x, 100.0f), "fragment should copy parent velocity x at creation");
        require(nearlyEqual(fragment.velocity.y, 50.0f), "fragment should copy parent velocity y at creation");
        require(nearlyEqual(fragment.rotationSpeed, 180.0f), "fragment born should configure visual rotation");

        RuntimeHelpers::beginMechanicsFrame(fragment);
        RuntimeHelpers::rotate(fragment, 1.0f, Delta);
        RuntimeHelpers::applyFreeMechanics(fragment, Delta);

        require(fragment.angle > 0.0f, "fragment should rotate visually");
        require(fragment.position.x > 0.0f, "fragment should move from inherited velocity x");
        require(fragment.position.y > 0.0f, "fragment should move from inherited velocity y");
        require(fragment.velocity.x > 0.0f && fragment.velocity.x < 100.0f, "fragment x velocity should decay progressively");
        require(fragment.velocity.y > 0.0f && fragment.velocity.y < 50.0f, "fragment y velocity should decay progressively");
        require(nearlyEqual(fragment.velocity.x / fragment.velocity.y, 2.0f), "fragment rotation should not change velocity direction");
    }

    void testDirectAsteroidRotationDoesNotCurveTrajectory()
    {
        RuntimeObject asteroid =
            mechanicsObject(MechanicsType::Direct, 0.0f, 0.0f);
        asteroid.mechanicsRotation.speed.start = 90.0f;

        RuntimeHelpers::applyVelocity(asteroid, 45.0f, 50.0f);

        const Vector2 initialVelocity =
            asteroid.velocity;

        for (int frame = 0; frame < 3; ++frame)
        {
            RuntimeHelpers::beginMechanicsFrame(asteroid);
            RuntimeHelpers::rotate(asteroid, 1.0f, Delta);
            RuntimeHelpers::advance(asteroid, Delta);
        }

        require(asteroid.angle > 0.0f, "direct asteroid should rotate visually");
        require(nearlyEqual(asteroid.velocity.x, initialVelocity.x), "direct asteroid rotation should not change velocity x");
        require(nearlyEqual(asteroid.velocity.y, initialVelocity.y), "direct asteroid rotation should not change velocity y");
        require(asteroid.position.x > 0.0f, "direct asteroid should move along applied x trajectory");
        require(asteroid.position.y < 0.0f, "direct asteroid should move along applied y trajectory");
        require(nearlyEqual(asteroid.position.x / -asteroid.position.y, 1.0f), "direct asteroid trajectory should remain 45 degrees");
    }

    void testInputDirectionAxesDoNotCross()
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

        harness.provider.setKeys({ KEY_W });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), 0.0f), "4way up should not leak into horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), 1.0f), "4way up should be positive vertical");
        require(nearlyEqual(static_cast<float>(rootObject.local["system"]), 0.0f), "system subject should reject directional intent");

        harness.provider.setKeys({ KEY_D });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), 1.0f), "4way right should be positive horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), 0.0f), "4way right should not leak into vertical");

        harness.provider.setKeys({ KEY_S });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), 0.0f), "4way down should not leak into horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), -1.0f), "4way down should be negative vertical");

        harness.provider.setKeys({ KEY_A });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), -1.0f), "4way left should be negative horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), 0.0f), "4way left should not leak into vertical");

        harness.provider.setKeys({ KEY_E });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayHorizontal"]), 1.0f), "2way positive should project as positive horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayVertical"]), 1.0f), "2way positive should project as positive vertical");

        harness.provider.setKeys({ KEY_Q });
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayHorizontal"]), -1.0f), "2way negative should project as negative horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["twoWayVertical"]), -1.0f), "2way negative should project as negative vertical");

        harness.provider.setKeys({});
        harness.input.update(0.016f);
        harness.update(0.016f);

        require(nearlyEqual(static_cast<float>(rootObject.local["horizontal"]), 0.0f), "neutral should return zero horizontal");
        require(nearlyEqual(static_cast<float>(rootObject.local["vertical"]), 0.0f), "neutral should return zero vertical");
    }

    void testAsteroidsInputIntentDoesNotCrossAxes()
    {
        RuntimeHarness harness;

        harness.project.context.machine.input.players = 1;
        harness.project.context.machine.input.directions = {
            InputDirectionDefinition{ "4way", "last", 0.0f }
        };

        harness.input.configure(harness.project.context.machine.input);
        harness.input.setPhysicalInputProvider(&harness.provider);
        harness.input.loadMappingContent(
            "asteroids.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.down=KEY_S\n"
            "players.1.directions.0.left=KEY_A\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        harness.addScript(
            "shipInput",
            "const MOVE = direction(0);"
            "function action(ship) {"
            "  const thrust = input_direction(ship, MOVE, VERTICAL);"
            "  const rotation = input_direction(ship, MOVE, HORIZONTAL);"
            "  if (thrust > 0) accelerate(ship, thrust);"
            "  if (rotation != 0) rotate(ship, rotation);"
            "}"
        );

        ObjectDefinition ship;
        ship.id = "root";
        ship.controlPlayer = 1;
        ship.resolvedScriptPaths.push_back("shipInput");
        ship.mechanics.type = MechanicsType::Polar;
        ship.mechanics.motion.acceleration = 100.0f;
        ship.mechanics.rotation.speed.start = 90.0f;

        harness.addObject(ship);
        require(harness.load().success, "runtime should load asteroids input regression");
        RuntimeObject& runtimeShip = requireObject(harness.world, "root");

        harness.provider.setKeys({ KEY_W });
        harness.input.update(1.0f);
        harness.update(1.0f);
        require(nearlyEqual(runtimeShip.angle, 0.0f), "UP should not rotate Asteroids ship");
        require(nearlyEqual(runtimeShip.velocity.y, -100.0f), "UP should accelerate Asteroids ship");

        runtimeShip.position = Vector2{ 0.0f, 0.0f };
        runtimeShip.velocity = Vector2{ 0.0f, 0.0f };
        runtimeShip.angle = 0.0f;

        harness.provider.setKeys({ KEY_D });
        harness.input.update(1.0f);
        harness.update(1.0f);
        require(nearlyEqual(runtimeShip.angle, 90.0f), "RIGHT should rotate Asteroids ship");
        require(nearlyEqual(runtimeShip.velocity.x, 0.0f), "RIGHT should not accelerate Asteroids ship x");
        require(nearlyEqual(runtimeShip.velocity.y, 0.0f), "RIGHT should not accelerate Asteroids ship y");

        runtimeShip.angle = 0.0f;

        harness.provider.setKeys({ KEY_A });
        harness.input.update(1.0f);
        harness.update(1.0f);
        require(nearlyEqual(runtimeShip.angle, -90.0f), "LEFT should rotate Asteroids ship");
        require(nearlyEqual(runtimeShip.velocity.x, 0.0f), "LEFT should not accelerate Asteroids ship x");
        require(nearlyEqual(runtimeShip.velocity.y, 0.0f), "LEFT should not accelerate Asteroids ship y");

        runtimeShip.angle = 0.0f;

        harness.provider.setKeys({ KEY_S });
        harness.input.update(1.0f);
        harness.update(1.0f);
        require(nearlyEqual(runtimeShip.angle, 0.0f), "DOWN should not rotate Asteroids ship");
        require(nearlyEqual(runtimeShip.velocity.x, 0.0f), "DOWN should not reverse thrust Asteroids ship x");
        require(nearlyEqual(runtimeShip.velocity.y, 0.0f), "DOWN should not reverse thrust Asteroids ship y");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "polar speed does not double count velocity", testPolarSpeedDoesNotDoubleCountVelocity },
        { "direct apply_speed and restore_speed drive axis movement", testDirectApplySpeedAndRestoreSpeedDriveAxisMovement },
        { "polar apply_speed and restore_speed preserve additional velocity", testPolarApplyAndRestorePreserveAdditionalVelocity },
        { "apply_speed limit still clamps", testApplySpeedLimitStillClamps },
        { "apply_velocity sets linear vector", testApplyVelocitySetsLinearVector },
        { "apply_velocity limit", testApplyVelocityLimit },
        { "common inertia resolves to axes", testCommonInertiaResolvesToAxes },
        { "acceleration uses delta", testAccelerationUsesDelta },
        { "accelerate then advance does not double count momentum", testAccelerateThenAdvanceDoesNotDoubleCountMomentum },
        { "accelerate rotate then advance keeps momentum direction", testAccelerateRotateThenAdvanceKeepsMomentumDirection },
        { "polar inertia does not rotate existing momentum", testPolarInertiaDoesNotRotateExistingMomentum },
        { "inertia zero clears stale velocity", testInertiaZeroClearsStaleVelocity },
        { "inertia partial and perpetual", testInertiaPartialAndPerpetual },
        { "inherit velocity copy and compose", testInheritVelocityCopyAndCompose },
        { "Asteroids fragment inherits velocity and keeps direction", testAsteroidsFragmentInheritsVelocityAndKeepsDirection },
        { "direct asteroid rotation does not curve trajectory", testDirectAsteroidRotationDoesNotCurveTrajectory },
        { "input_direction axes do not cross", testInputDirectionAxesDoNotCross },
        { "Asteroids input intent does not cross axes", testAsteroidsInputIntentDoesNotCrossAxes }
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
