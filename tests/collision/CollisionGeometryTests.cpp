#include "../support/TestSupport.h"
#include "../../engine/collision/EffectiveColliderBuilder.h"
#include "../../engine/collision/CollisionGeometry.h"
#include "../../engine/collision/CollisionSystem.h"
#include "../../engine/runtime/RuntimeObject.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace flx::test;

namespace
{
    bool nearlyEqual(float left, float right, float epsilon = 0.001f)
    {
        return std::abs(left - right) <= epsilon;
    }

    EffectiveCollider collider(
        const std::string& type,
        Vector2 center,
        Vector2 size,
        float angle = 0.0f
    )
    {
        EffectiveCollider result;
        result.name = "body";
        result.type = type;
        result.effective = true;
        result.declaredEnabled = true;
        result.stateAvailable = true;
        result.center = center;
        result.halfSize = Vector2{ size.x / 2.0f, size.y / 2.0f };
        result.angle = angle;
        result.broadBounds = Rectangle{
            center.x - size.x * 2.0f,
            center.y - size.y * 2.0f,
            size.x * 4.0f,
            size.y * 4.0f
        };

        return result;
    }

    EffectiveCollider namedCollider(
        EffectiveCollider value,
        const std::string& name
    )
    {
        value.name =
            name;

        return value;
    }

    bool hasContact(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        CollisionContact* output = nullptr
    )
    {
        CollisionContact contact;

        const bool result =
            CollisionGeometry::contact(source, target, contact);

        if (output != nullptr)
        {
            *output = contact;
        }

        return result;
    }

    EffectiveCollider topLeftBox(Vector2 topLeft, Vector2 size, Vector2 offset)
    {
        return collider(
            "box",
            Vector2{ topLeft.x + offset.x, topLeft.y + offset.y },
            size
        );
    }

    EffectiveCollider circle(Vector2 center, float radius)
    {
        return collider(
            "ellipse",
            center,
            Vector2{ radius * 2.0f, radius * 2.0f }
        );
    }

    void requireSymmetricContact(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        const std::string& message
    )
    {
        CollisionContact ab;
        CollisionContact ba;

        require(hasContact(source, target, &ab), message + " should hit from source to target");
        require(hasContact(target, source, &ba), message + " should hit from target to source");

        require(nearlyEqual(ab.point.x, ba.point.x, 0.02f), message + " should preserve contact point x when reversed");
        require(nearlyEqual(ab.point.y, ba.point.y, 0.02f), message + " should preserve contact point y when reversed");
        require(nearlyEqual(ab.penetration, ba.penetration, 0.02f), message + " should preserve penetration when reversed");
        require(nearlyEqual(ab.normal.x, -ba.normal.x, 0.02f), message + " should invert normal x when reversed");
        require(nearlyEqual(ab.normal.y, -ba.normal.y, 0.02f), message + " should invert normal y when reversed");
    }

    Vector2 rotateForTest(Vector2 value, float angle)
    {
        const float radians =
            angle * DEG2RAD;

        const float cosine =
            std::cos(radians);

        const float sine =
            std::sin(radians);

        return Vector2{
            value.x * cosine - value.y * sine,
            value.x * sine + value.y * cosine
        };
    }

    bool pointInsideBoxForTest(
        Vector2 point,
        const EffectiveCollider& box,
        float epsilon = 0.05f
    )
    {
        const Vector2 xAxis =
            rotateForTest(Vector2{ 1.0f, 0.0f }, box.angle);

        const Vector2 yAxis =
            rotateForTest(Vector2{ 0.0f, 1.0f }, box.angle);

        const Vector2 delta =
            Vector2{
                point.x - box.center.x,
                point.y - box.center.y
            };

        const float localX =
            delta.x * xAxis.x + delta.y * xAxis.y;

        const float localY =
            delta.x * yAxis.x + delta.y * yAxis.y;

        return
            std::abs(localX) <= box.halfSize.x + epsilon &&
            std::abs(localY) <= box.halfSize.y + epsilon;
    }

    RuntimeObject runtimeBox(
        const std::string& name,
        Vector2 position,
        Vector2 size,
        const std::string& group
    )
    {
        RuntimeObject object(name, position, size, WHITE);
        object.runtimeId = name;
        object.group = group;
        object.collisions["body"].type = "box";
        return object;
    }

    RuntimeObject pongBall(Vector2 position, float angle)
    {
        RuntimeObject object("ball", position, Vector2{ 5.0f, 5.0f }, WHITE);
        object.runtimeId = "ball";
        object.group = "ball";
        object.mechanicsType = MechanicsType::Polar;
        object.speed = 90.0f;
        object.originSpeed = 90.0f;
        object.angle = angle;
        object.resolvedScriptPaths.push_back("pongBounce");

        ColliderDefinition& collider =
            object.collisions["body"];

        collider.type = "box";
        collider.with.push_back("wall");

        return object;
    }

    RuntimeObject pongWall(
        const std::string& name,
        Vector2 position
    )
    {
        RuntimeObject object(name, position, Vector2{ 320.0f, 10.0f }, WHITE);
        object.runtimeId = name;
        object.group = "wall";

        ColliderDefinition& collider =
            object.collisions["body"];

        collider.type = "box";

        return object;
    }

    double localNumber(const RuntimeObject& object, const std::string& key)
    {
        const auto it =
            object.local.find(key);

        if (it == object.local.end())
        {
            return 0.0;
        }

        if (const auto* number = std::get_if<double>(&it->second))
        {
            return *number;
        }

        if (const auto* boolean = std::get_if<bool>(&it->second))
        {
            return *boolean ? 1.0 : 0.0;
        }

        return 0.0;
    }

    void testPongGameplayGeometry()
    {
        const EffectiveCollider ballLeftTop =
            topLeftBox(Vector2{ 20.0f, -5.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f });
        const EffectiveCollider ballRightTop =
            topLeftBox(Vector2{ 300.0f, -5.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f });
        const EffectiveCollider topWall =
            topLeftBox(Vector2{ 0.0f, -10.0f }, Vector2{ 320.0f, 10.0f }, Vector2{ 160.0f, 5.0f });

        require(hasContact(ballLeftTop, topWall), "Pong top wall should cover the left side");
        require(hasContact(ballRightTop, topWall), "Pong top wall should cover the right side");

        require(
            hasContact(
                topLeftBox(Vector2{ 120.0f, 175.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f }),
                topLeftBox(Vector2{ 0.0f, 180.0f }, Vector2{ 320.0f, 10.0f }, Vector2{ 160.0f, 5.0f })
            ),
            "Pong bottom wall should match the visual surface"
        );

        require(
            hasContact(
                topLeftBox(Vector2{ 21.0f, 100.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f }),
                topLeftBox(Vector2{ 16.0f, 90.0f }, Vector2{ 5.0f, 30.0f }, Vector2{ 2.5f, 15.0f })
            ),
            "Pong ball should contact the player paddle at its drawn edge"
        );

        require(
            hasContact(
                topLeftBox(Vector2{ 294.0f, 100.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f }),
                topLeftBox(Vector2{ 299.0f, 90.0f }, Vector2{ 5.0f, 30.0f }, Vector2{ 2.5f, 15.0f })
            ),
            "Pong ball should contact the enemy paddle at its drawn edge"
        );

        require(
            hasContact(
                topLeftBox(Vector2{ -5.0f, 80.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f }),
                topLeftBox(Vector2{ -10.0f, 0.0f }, Vector2{ 10.0f, 180.0f }, Vector2{ 5.0f, 90.0f })
            ),
            "Pong left goal should match the full logical height"
        );

        require(
            hasContact(
                topLeftBox(Vector2{ 320.0f, 80.0f }, Vector2{ 5.0f, 5.0f }, Vector2{ 2.5f, 2.5f }),
                topLeftBox(Vector2{ 320.0f, 0.0f }, Vector2{ 10.0f, 180.0f }, Vector2{ 5.0f, 90.0f })
            ),
            "Pong right goal should match the full logical height"
        );
    }

    void testArkanoidGameplayGeometry()
    {
        require(
            hasContact(circle(Vector2{ 335.0f, 275.0f }, 5.0f),
                topLeftBox(Vector2{ 300.0f, 280.0f }, Vector2{ 70.0f, 10.0f }, Vector2{ 35.0f, 5.0f })),
            "Arkanoid ball should contact the horizontal paddle surface"
        );

        require(
            hasContact(circle(Vector2{ 500.0f, 60.0f }, 5.0f),
                topLeftBox(Vector2{ 40.0f, 45.0f }, Vector2{ 560.0f, 10.0f }, Vector2{ 280.0f, 5.0f })),
            "Arkanoid top wall should match the drawn width"
        );

        require(
            hasContact(circle(Vector2{ 55.0f, 120.0f }, 5.0f),
                topLeftBox(Vector2{ 40.0f, 45.0f }, Vector2{ 10.0f, 250.0f }, Vector2{ 5.0f, 125.0f })),
            "Arkanoid left wall should match the drawn height"
        );

        require(
            hasContact(circle(Vector2{ 595.0f, 120.0f }, 5.0f),
                topLeftBox(Vector2{ 600.0f, 45.0f }, Vector2{ 10.0f, 250.0f }, Vector2{ 5.0f, 125.0f })),
            "Arkanoid right wall should match the drawn height"
        );

        const EffectiveCollider brick =
            topLeftBox(Vector2{ 100.0f, 100.0f }, Vector2{ 30.0f, 10.0f }, Vector2{ 15.0f, 5.0f });

        require(hasContact(circle(Vector2{ 115.0f, 95.0f }, 5.0f), brick), "Arkanoid ball should contact the top face of a brick");
        require(hasContact(circle(Vector2{ 95.0f, 105.0f }, 5.0f), brick), "Arkanoid ball should contact the lateral face of a brick");
        require(!hasContact(circle(Vector2{ 82.0f, 82.0f }, 5.0f), brick), "Arkanoid ball should not touch when passing outside a brick corner");
        require(hasContact(circle(Vector2{ 96.4645f, 96.4645f }, 5.0f), brick), "Arkanoid ball should contact when tangent to a brick corner");
    }

    void testEllipseCornerContacts()
    {
        const EffectiveCollider box =
            collider("box", Vector2{ 100.0f, 100.0f }, Vector2{ 20.0f, 20.0f });

        require(!hasContact(circle(Vector2{ 82.0f, 82.0f }, 5.0f), box), "circle outside a box corner should not contact");
        require(hasContact(circle(Vector2{ 86.4645f, 86.4645f }, 5.0f), box), "circle tangent to a box corner should contact");

        CollisionContact contact;
        require(hasContact(circle(Vector2{ 87.0f, 87.0f }, 5.0f), box, &contact), "circle penetrating a box corner should contact");
        require(contact.normal.x < -0.4f && contact.normal.y < -0.4f, "corner penetration normal should be diagonal");
        require(contact.point.x > 88.5f && contact.point.x < 91.5f, "corner contact point should stay near the box corner x");
        require(contact.point.y > 88.5f && contact.point.y < 91.5f, "corner contact point should stay near the box corner y");

        const EffectiveCollider ellipse =
            collider("ellipse", Vector2{ 83.0f, 100.0f }, Vector2{ 24.0f, 8.0f }, 45.0f);

        require(hasContact(ellipse, box), "oriented non-circular ellipse should contact a box when its real surface reaches it");
    }

    void testCircleBoxFaceContactPointUsesWitnessEdge()
    {
        const EffectiveCollider box =
            collider("box", Vector2{ 100.0f, 100.0f }, Vector2{ 20.0f, 20.0f });

        CollisionContact topContact;
        require(hasContact(circle(Vector2{ 100.0f, 86.0f }, 5.0f), box, &topContact), "circle should hit top box face");
        require(nearlyEqual(topContact.normal.x, 0.0f, 0.02f), "top face normal x should be vertical");
        require(nearlyEqual(topContact.normal.y, -1.0f, 0.02f), "top face normal should point upward from circle to box");
        require(nearlyEqual(topContact.penetration, 1.0f, 0.02f), "top face penetration should be approximately one");
        require(nearlyEqual(topContact.point.x, 100.0f, 0.05f), "top face point should stay near the circle center x, not a box corner");
        require(nearlyEqual(topContact.point.y, 90.5f, 0.05f), "top face point should sit between circle and box surfaces");

        requireSymmetricContact(
            circle(Vector2{ 100.0f, 86.0f }, 5.0f),
            box,
            "circle top face against box"
        );
    }

    void testCircleBoxSideContactPointUsesWitnessEdge()
    {
        const EffectiveCollider box =
            collider("box", Vector2{ 100.0f, 100.0f }, Vector2{ 20.0f, 20.0f });

        CollisionContact sideContact;
        require(hasContact(circle(Vector2{ 86.0f, 100.0f }, 5.0f), box, &sideContact), "circle should hit left box face");
        require(nearlyEqual(sideContact.normal.x, -1.0f, 0.02f), "left face normal should point left from circle to box");
        require(nearlyEqual(sideContact.normal.y, 0.0f, 0.02f), "left face normal y should be horizontal");
        require(nearlyEqual(sideContact.penetration, 1.0f, 0.02f), "left face penetration should be approximately one");
        require(nearlyEqual(sideContact.point.x, 90.5f, 0.05f), "left face point should sit between circle and box surfaces");
        require(nearlyEqual(sideContact.point.y, 100.0f, 0.05f), "left face point should stay near the circle center y, not a box corner");

        requireSymmetricContact(
            circle(Vector2{ 86.0f, 100.0f }, 5.0f),
            box,
            "circle side face against box"
        );
    }

    void testNonCircularEllipseBoxContactPoint()
    {
        const EffectiveCollider box =
            collider("box", Vector2{ 100.0f, 100.0f }, Vector2{ 20.0f, 20.0f });

        const EffectiveCollider ellipse =
            collider("ellipse", Vector2{ 100.0f, 86.0f }, Vector2{ 30.0f, 10.0f });

        CollisionContact contact;
        require(hasContact(ellipse, box, &contact), "non-circular ellipse should hit top box face");
        require(nearlyEqual(contact.normal.x, 0.0f, 0.03f), "non-circular ellipse top normal x should be vertical");
        require(nearlyEqual(contact.normal.y, -1.0f, 0.03f), "non-circular ellipse top normal should point upward");
        require(contact.point.x > 99.0f && contact.point.x < 101.0f, "non-circular ellipse point should remain near face center x");
        require(contact.point.y > 90.0f && contact.point.y < 91.0f, "non-circular ellipse point should lie between surfaces");

        requireSymmetricContact(
            ellipse,
            box,
            "non-circular ellipse against box"
        );
    }

    void testOrientedEllipseBoxContactPoint()
    {
        const EffectiveCollider box =
            collider("box", Vector2{ 100.0f, 100.0f }, Vector2{ 20.0f, 20.0f });

        const EffectiveCollider ellipse =
            collider("ellipse", Vector2{ 92.0f, 89.0f }, Vector2{ 24.0f, 8.0f }, 45.0f);

        CollisionContact contact;
        require(hasContact(ellipse, box, &contact), "oriented ellipse should hit box");
        require(contact.point.x > 89.0f && contact.point.x < 101.0f, "oriented ellipse point should remain near the contact region x");
        require(contact.point.y > 88.0f && contact.point.y < 96.0f, "oriented ellipse point should remain near the contact region y");
        require(contact.penetration > 0.0f, "oriented ellipse should report positive penetration");

        requireSymmetricContact(
            ellipse,
            box,
            "oriented ellipse against box"
        );
    }

    void testBoxBoxHorizontalFaceContactPointUsesOverlapRegion()
    {
        const EffectiveCollider ball =
            collider("box", Vector2{ 40.0f, -1.5f }, Vector2{ 5.0f, 5.0f });

        const EffectiveCollider wall =
            collider("box", Vector2{ 160.0f, -5.0f }, Vector2{ 320.0f, 10.0f });

        CollisionContact contact;
        require(hasContact(ball, wall, &contact), "small box should hit long horizontal face");
        require(nearlyEqual(contact.point.x, 40.0f, 0.05f), "horizontal face contact point x should follow the small box overlap");
        require(contact.point.y > -4.1f && contact.point.y < 0.1f, "horizontal face contact point y should remain inside the overlap region");
    }

    void testBoxBoxVerticalFaceContactPointUsesOverlapRegion()
    {
        const EffectiveCollider ball =
            collider("box", Vector2{ -1.5f, 40.0f }, Vector2{ 5.0f, 5.0f });

        const EffectiveCollider wall =
            collider("box", Vector2{ -5.0f, 90.0f }, Vector2{ 10.0f, 180.0f });

        CollisionContact contact;
        require(hasContact(ball, wall, &contact), "small box should hit long vertical face");
        require(contact.point.x > -4.1f && contact.point.x < 0.1f, "vertical face contact point x should remain inside the overlap region");
        require(nearlyEqual(contact.point.y, 40.0f, 0.05f), "vertical face contact point y should follow the small box overlap");
    }

    void testBoxBoxContactSymmetry()
    {
        const EffectiveCollider ball =
            namedCollider(
                collider("box", Vector2{ 40.0f, -1.5f }, Vector2{ 5.0f, 5.0f }),
                "ball"
            );

        const EffectiveCollider wall =
            namedCollider(
                collider("box", Vector2{ 160.0f, -5.0f }, Vector2{ 320.0f, 10.0f }),
                "wall"
            );

        CollisionContact ab;
        CollisionContact ba;

        require(hasContact(ball, wall, &ab), "box contact should hit from ball to wall");
        require(hasContact(wall, ball, &ba), "box contact should hit from wall to ball");
        require(ab.collider == "ball", "forward contact should preserve source collider name");
        require(ab.otherCollider == "wall", "forward contact should preserve target collider name");
        require(ba.collider == "wall", "reverse contact should preserve source collider name");
        require(ba.otherCollider == "ball", "reverse contact should preserve target collider name");
        require(nearlyEqual(ab.point.x, ba.point.x, 0.02f), "box contact point x should be symmetric");
        require(nearlyEqual(ab.point.y, ba.point.y, 0.02f), "box contact point y should be symmetric");
        require(nearlyEqual(ab.penetration, ba.penetration, 0.02f), "box penetration should be symmetric");
        require(nearlyEqual(ab.normal.x, -ba.normal.x, 0.02f), "box normal x should invert");
        require(nearlyEqual(ab.normal.y, -ba.normal.y, 0.02f), "box normal y should invert");
    }

    void testRotatedBoxBoxContactPointUsesOverlapRegion()
    {
        const EffectiveCollider source =
            collider("box", Vector2{ 100.0f, 100.0f }, Vector2{ 40.0f, 20.0f }, 25.0f);

        const EffectiveCollider target =
            collider("box", Vector2{ 112.0f, 104.0f }, Vector2{ 30.0f, 18.0f }, -20.0f);

        CollisionContact ab;
        CollisionContact ba;

        require(hasContact(source, target, &ab), "rotated box contact should hit");
        require(hasContact(target, source, &ba), "rotated box contact should hit when reversed");
        require(pointInsideBoxForTest(ab.point, source), "rotated box contact point should lie inside source box");
        require(pointInsideBoxForTest(ab.point, target), "rotated box contact point should lie inside target box");
        require(nearlyEqual(ab.point.x, ba.point.x, 0.02f), "rotated box contact point x should be symmetric");
        require(nearlyEqual(ab.point.y, ba.point.y, 0.02f), "rotated box contact point y should be symmetric");
        require(nearlyEqual(ab.penetration, ba.penetration, 0.02f), "rotated box penetration should be symmetric");
        require(nearlyEqual(ab.normal.x, -ba.normal.x, 0.02f), "rotated box normal x should invert");
        require(nearlyEqual(ab.normal.y, -ba.normal.y, 0.02f), "rotated box normal y should invert");
    }

    void testPongBoxWallContactPointTracksBall()
    {
        const EffectiveCollider wall =
            collider("box", Vector2{ 160.0f, -5.0f }, Vector2{ 320.0f, 10.0f });

        const std::vector<float> positions =
            { 20.0f, 160.0f, 300.0f };

        for (float x : positions)
        {
            const EffectiveCollider ball =
                collider("box", Vector2{ x, -1.5f }, Vector2{ 5.0f, 5.0f });

            CollisionContact contact;
            require(hasContact(ball, wall, &contact), "Pong ball should hit top wall at every sampled x");
            require(nearlyEqual(contact.point.x, x, 0.05f), "Pong box-box contact point should track the ball x");
            require(contact.point.y > -4.1f && contact.point.y < 0.1f, "Pong box-box contact point should stay in the wall overlap");
        }
    }

    void testCollisionSystemRebuildsAfterPositionMutation()
    {
        ScriptEngine scripts;
        scripts.loadScript(
            "mover",
            "function collision(o, other) {"
            "  write_local(o, 'count', (read_local(o, 'count') || 0) + 1);"
            "  if (other.name == 'b') { position_x(o, 100); }"
            "  if (other.name == 'c') { write_local(o, 'hitC', 1); }"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(runtimeBox("a", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "source"));
        objects.push_back(runtimeBox("b", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "target"));
        objects.push_back(runtimeBox("c", Vector2{ 100.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "target"));

        objects[0].resolvedScriptPaths.push_back("mover");
        objects[0].collisions["body"].with.push_back("target");

        CollisionSystem::run(objects, scripts);

        require(nearlyEqual(objects[0].position.x, 100.0f), "collision callback should move source object");
        require(localNumber(objects[0], "count") == 2.0, "later directed interaction should use rebuilt source geometry");
        require(localNumber(objects[0], "hitC") == 1.0, "source should collide with C after moving during A to B");
    }

    void testCollisionSystemRebuildsAfterColliderMutation()
    {
        ScriptEngine scripts;
        scripts.loadScript(
            "disabler",
            "function collision(o, other) {"
            "  write_local(o, 'count', (read_local(o, 'count') || 0) + 1);"
            "  if (other.name == 'b') { collider_off(o, 'body'); }"
            "  if (other.name == 'c') { write_local(o, 'hitC', 1); }"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(runtimeBox("a", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "source"));
        objects.push_back(runtimeBox("b", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "target"));
        objects.push_back(runtimeBox("c", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "target"));

        objects[0].resolvedScriptPaths.push_back("disabler");
        objects[0].collisions["body"].with.push_back("target");

        CollisionSystem::run(objects, scripts);

        require(!objects[0].collisions["body"].enabled, "collision callback should disable source collider");
        require(localNumber(objects[0], "count") == 1.0, "disabled collider should not participate in later directed interactions");
        require(localNumber(objects[0], "hitC") == 0.0, "C should not collide after collider_off during A to B");
    }

    void testCollisionSystemRebuildsAfterStateMutation()
    {
        std::unordered_map<std::string, ObjectDefinition> definitions;
        definitions["a"].id = "a";
        definitions["a"].initialState = "attack";
        definitions["a"].stateTransitions["attack"] = { "idle" };
        definitions["a"].stateTransitions["idle"] = {};

        ScriptEngine scripts;
        scripts.setRuntimeFrame(1);
        scripts.setFindObjectDefinitionFunction(
            [&definitions](const std::string& id)
            {
                const auto it =
                    definitions.find(id);

                return it == definitions.end()
                    ? nullptr
                    : &it->second;
            }
        );

        scripts.loadScript(
            "stateChanger",
            "function collision(o, other) {"
            "  write_local(o, 'count', (read_local(o, 'count') || 0) + 1);"
            "  if (other.name == 'b') { state_to(o, 'idle'); }"
            "  if (other.name == 'c') { write_local(o, 'hitC', 1); }"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(runtimeBox("a", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "source"));
        objects.push_back(runtimeBox("b", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "target"));
        objects.push_back(runtimeBox("c", Vector2{ 0.0f, 0.0f }, Vector2{ 10.0f, 10.0f }, "target"));

        objects[0].definitionId = "a";
        objects[0].state = "attack";
        objects[0].resolvedScriptPaths.push_back("stateChanger");
        objects[0].collisions["body"].with.push_back("target");
        objects[0].collisions["body"].states.push_back("attack");

        CollisionSystem::run(objects, scripts);

        require(objects[0].state == "idle", "collision callback should change source state");
        require(localNumber(objects[0], "count") == 1.0, "source collider state restriction should be rebuilt before later directed interactions");
        require(localNumber(objects[0], "hitC") == 0.0, "C should not collide after state_to removes source collider availability");
    }

    void testPongTopWallBoxBounceSeparatesAndMovesAway()
    {
        ScriptEngine scripts;
        scripts.setFrameDelta(1.0f / 60.0f);
        scripts.loadScript(
            "pongBounce",
            "function motion(ball) { advance(ball); }"
            "function collision(ball, other, contacts) {"
            "  const contact = contacts[0];"
            "  write_local(ball, 'normalX', contact.normalX);"
            "  write_local(ball, 'normalY', contact.normalY);"
            "  write_local(ball, 'penetration', contact.penetration);"
            "  position(ball, ball.x + contact.normalX * contact.penetration, ball.y + contact.normalY * contact.penetration);"
            "  write_local(ball, 'separatedX', ball.x);"
            "  write_local(ball, 'separatedY', ball.y);"
            "  reflect_y(ball);"
            "  write_local(ball, 'count', (read_local(ball, 'count') || 0) + 1);"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(pongBall(Vector2{ 102.5f, -1.5f }, 0.0f));
        objects.push_back(pongWall("top_wall", Vector2{ 160.0f, -5.0f }));

        const float originalY =
            objects[0].position.y;

        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "top wall penetration should produce one callback");
        require(localNumber(objects[0], "normalY") > 0.9, "top wall contact normal should separate the ball downward");
        require(std::abs(localNumber(objects[0], "normalX")) < 0.1, "top wall contact normal should be vertical");
        require(localNumber(objects[0], "penetration") > 0.0, "top wall contact should report penetration");
        require(objects[0].position.y > originalY, "top wall script should separate ball out of the wall");

        const float separatedY =
            objects[0].position.y;

        scripts.callScriptFunction("pongBounce", "motion", objects[0]);
        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "separated top wall bounce should not produce a second callback");
        require(objects[0].position.y > separatedY, "ball should move away from the top wall on the next frame");
    }

    void testPongBottomWallBoxBounceSeparatesAndMovesAway()
    {
        ScriptEngine scripts;
        scripts.setFrameDelta(1.0f / 60.0f);
        scripts.loadScript(
            "pongBounce",
            "function motion(ball) { advance(ball); }"
            "function collision(ball, other, contacts) {"
            "  const contact = contacts[0];"
            "  write_local(ball, 'normalX', contact.normalX);"
            "  write_local(ball, 'normalY', contact.normalY);"
            "  write_local(ball, 'penetration', contact.penetration);"
            "  position(ball, ball.x + contact.normalX * contact.penetration, ball.y + contact.normalY * contact.penetration);"
            "  write_local(ball, 'separatedX', ball.x);"
            "  write_local(ball, 'separatedY', ball.y);"
            "  reflect_y(ball);"
            "  write_local(ball, 'count', (read_local(ball, 'count') || 0) + 1);"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(pongBall(Vector2{ 102.5f, 179.5f }, 180.0f));
        objects.push_back(pongWall("bottom_wall", Vector2{ 160.0f, 185.0f }));

        const float originalY =
            objects[0].position.y;

        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "bottom wall penetration should produce one callback");
        require(localNumber(objects[0], "normalY") < -0.9, "bottom wall contact normal should separate the ball upward");
        require(std::abs(localNumber(objects[0], "normalX")) < 0.1, "bottom wall contact normal should be vertical");
        require(localNumber(objects[0], "penetration") > 0.0, "bottom wall contact should report penetration");
        require(objects[0].position.y < originalY, "bottom wall script should separate ball out of the wall");

        const float separatedY =
            objects[0].position.y;

        scripts.callScriptFunction("pongBounce", "motion", objects[0]);
        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "separated bottom wall bounce should not produce a second callback");
        require(objects[0].position.y < separatedY, "ball should move away from the bottom wall on the next frame");
    }

    void testScriptSeparationPreventsRepeatedWallCallback()
    {
        ScriptEngine scripts;
        scripts.setFrameDelta(0.1f);
        scripts.loadScript(
            "ballBounce",
            "function motion(o) { advance(o); }"
            "function collision(o, other, contacts) {"
            "  let contact = contacts[0];"
            "  position(o, o.x + contact.normalX * contact.penetration, o.y + contact.normalY * contact.penetration);"
            "  reflect_y(o);"
            "  write_local(o, 'count', (read_local(o, 'count') || 0) + 1);"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(runtimeBox("ball", Vector2{ 0.0f, -4.0f }, Vector2{ 10.0f, 10.0f }, "ball"));
        objects.push_back(runtimeBox("wall", Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, 10.0f }, "wall"));

        objects[0].mechanicsType = MechanicsType::Polar;
        objects[0].speed = 20.0f;
        objects[0].originSpeed = 20.0f;
        objects[0].angle = 180.0f;
        objects[0].resolvedScriptPaths.push_back("ballBounce");
        objects[0].collisions["body"].type = "ellipse";
        objects[0].collisions["body"].size.width = 10.0f;
        objects[0].collisions["body"].size.height = 10.0f;
        objects[0].collisions["body"].size.hasWidth = true;
        objects[0].collisions["body"].size.hasHeight = true;
        objects[0].collisions["body"].with.push_back("wall");

        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "initial penetration should produce exactly one wall callback");

        scripts.callScriptFunction("ballBounce", "motion", objects[0]);
        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "separation plus reflection should avoid a second spurious wall callback");
        require(objects[0].position.y < -9.0f, "ball should move away from the wall on the next frame");
    }

    void testScriptBoxSeparationPreventsRepeatedWallCallback()
    {
        ScriptEngine scripts;
        scripts.setFrameDelta(0.1f);
        scripts.loadScript(
            "boxBounce",
            "function motion(o) { advance(o); }"
            "function collision(o, other, contacts) {"
            "  const contact = contacts[0];"
            "  position(o, o.x + contact.normalX * contact.penetration, o.y + contact.normalY * contact.penetration);"
            "  reflect_y(o);"
            "  write_local(o, 'count', (read_local(o, 'count') || 0) + 1);"
            "}"
        );

        std::vector<RuntimeObject> objects;
        objects.push_back(runtimeBox("ball", Vector2{ 0.0f, -4.0f }, Vector2{ 10.0f, 10.0f }, "ball"));
        objects.push_back(runtimeBox("wall", Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, 10.0f }, "wall"));

        objects[0].mechanicsType = MechanicsType::Polar;
        objects[0].speed = 20.0f;
        objects[0].originSpeed = 20.0f;
        objects[0].angle = 180.0f;
        objects[0].resolvedScriptPaths.push_back("boxBounce");
        objects[0].collisions["body"].with.push_back("wall");

        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "initial box penetration should produce exactly one wall callback");

        scripts.callScriptFunction("boxBounce", "motion", objects[0]);
        CollisionSystem::run(objects, scripts);

        require(localNumber(objects[0], "count") == 1.0, "box separation plus reflection should avoid a second spurious wall callback");
        require(objects[0].position.y < -9.0f, "box should move away from the wall on the next frame");
    }

    EffectiveCollider requireSingleCollider(
        const RuntimeObject& object,
        ScriptEngine& scripts
    )
    {
        const std::vector<EffectiveCollider> colliders =
            EffectiveColliderBuilder::build(object, scripts);

        require(colliders.size() == 1, "object should expose one effective collider declaration");

        return colliders.front();
    }

    void testColliderInheritsLiveSizePerAxis()
    {
        ScriptEngine scripts;

        RuntimeObject object("body", Vector2{ 100.0f, 50.0f }, Vector2{ 20.0f, 10.0f }, WHITE);
        object.runtimeId = "body";

        ColliderDefinition& collider =
            object.collisions["body"];

        collider.type = "box";
        collider.size.width = 8.0f;
        collider.size.hasWidth = true;

        EffectiveCollider effective =
            requireSingleCollider(object, scripts);

        require(nearlyEqual(effective.halfSize.x, 4.0f), "explicit collider width should be preserved");
        require(nearlyEqual(effective.halfSize.y, 5.0f), "omitted collider height should inherit object height");

        object.size = Vector2{ 40.0f, 30.0f };

        effective =
            requireSingleCollider(object, scripts);

        require(nearlyEqual(effective.halfSize.x, 4.0f), "explicit collider width should not change after resize");
        require(nearlyEqual(effective.halfSize.y, 15.0f), "inherited collider height should follow live object height");
    }

    void testColliderOffsetUsesObjectLocalSpace()
    {
        ScriptEngine scripts;

        RuntimeObject object("body", Vector2{ 100.0f, 50.0f }, Vector2{ 20.0f, 10.0f }, WHITE);
        object.runtimeId = "body";

        ColliderDefinition& collider =
            object.collisions["body"];

        collider.type = "box";
        collider.offset = Vector2{ 20.0f, 0.0f };
        collider.angle = 15.0f;

        EffectiveCollider effective =
            requireSingleCollider(object, scripts);

        require(nearlyEqual(effective.center.x, 120.0f), "angle 0 should place collider at position plus local offset x");
        require(nearlyEqual(effective.center.y, 50.0f), "angle 0 should place collider at position plus local offset y");
        require(nearlyEqual(effective.angle, 15.0f), "collider angle should include local collider angle");

        object.angle = 90.0f;

        effective =
            requireSingleCollider(object, scripts);

        require(nearlyEqual(effective.center.x, 100.0f), "object rotation should rotate local collider offset x");
        require(nearlyEqual(effective.center.y, 70.0f), "object rotation should rotate local collider offset y");
        require(nearlyEqual(effective.angle, 105.0f), "collider angle should be object angle plus local collider angle");

        object.size = Vector2{ 40.0f, 30.0f };

        effective =
            requireSingleCollider(object, scripts);

        require(nearlyEqual(effective.center.x, 100.0f), "resize should not scale local collider offset x");
        require(nearlyEqual(effective.center.y, 70.0f), "resize should not scale local collider offset y");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "Pong gameplay geometry", testPongGameplayGeometry },
        { "Arkanoid gameplay geometry", testArkanoidGameplayGeometry },
        { "ellipse corner contacts", testEllipseCornerContacts },
        { "circle box face contact point uses witness edge", testCircleBoxFaceContactPointUsesWitnessEdge },
        { "circle box side contact point uses witness edge", testCircleBoxSideContactPointUsesWitnessEdge },
        { "non-circular ellipse box contact point", testNonCircularEllipseBoxContactPoint },
        { "oriented ellipse box contact point", testOrientedEllipseBoxContactPoint },
        { "box box horizontal face contact point uses overlap region", testBoxBoxHorizontalFaceContactPointUsesOverlapRegion },
        { "box box vertical face contact point uses overlap region", testBoxBoxVerticalFaceContactPointUsesOverlapRegion },
        { "box box contact symmetry", testBoxBoxContactSymmetry },
        { "rotated box box contact point uses overlap region", testRotatedBoxBoxContactPointUsesOverlapRegion },
        { "Pong box wall contact point tracks ball", testPongBoxWallContactPointTracksBall },
        { "CollisionSystem rebuilds after position mutation", testCollisionSystemRebuildsAfterPositionMutation },
        { "CollisionSystem rebuilds after collider mutation", testCollisionSystemRebuildsAfterColliderMutation },
        { "CollisionSystem rebuilds after state mutation", testCollisionSystemRebuildsAfterStateMutation },
        { "Pong exact top wall box bounce", testPongTopWallBoxBounceSeparatesAndMovesAway },
        { "Pong exact bottom wall box bounce", testPongBottomWallBoxBounceSeparatesAndMovesAway },
        { "script separation prevents repeated wall callback", testScriptSeparationPreventsRepeatedWallCallback },
        { "script box separation prevents repeated wall callback", testScriptBoxSeparationPreventsRepeatedWallCallback },
        { "collider inherits live size per axis", testColliderInheritsLiveSizePerAxis },
        { "collider offset uses object local space", testColliderOffsetUsesObjectLocalSpace }
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
