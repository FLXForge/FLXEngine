#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/core/FlxSession.h"
#include "../../engine/compiler/ProjectCompiler.h"
#include "../../engine/graphics/DrawingContext.h"
#include "../../engine/graphics/DrawingRenderer.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

void testAnimationGeometryCPU();
void testAnimationRendering();

using namespace flx::test;

namespace
{
    constexpr float ScreenWidth = 40.0f;
    constexpr float ScreenHeight = 30.0f;

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

            ObjectDefinition testWorld;
            testWorld.id = "__test_world_extent";
            testWorld.delimit = true;
            testWorld.visible = false;
            testWorld.origin = Vector2{ ScreenWidth / 2.0f, ScreenHeight / 2.0f };
            testWorld.hasOrigin = true;
            testWorld.size = Vector2{ ScreenWidth, ScreenHeight };
            testWorld.hasSize = true;
            addObject(testWorld);

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

            scripts.setFindObjectsByNameFunction(
                [this](const std::string& name)
                {
                    return world.findAllLiveByName(name);
                }
            );

            scripts.setFindParentFunction(
                [this](const std::string& runtimeId)
                {
                    return world.findLiveParent(runtimeId);
                }
            );

            scripts.setFindChildrenFunction(
                [this](const std::string& runtimeId)
                {
                    return world.findLiveChildren(runtimeId);
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
            if (definition.id == "root" && !definition.delimit)
            {
                definition.childResources["__test_world_extent"] =
                    "__test_world_extent";
            }

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

        void draw(int scale = 1)
        {
            world.draw(scripts, scale, ScreenWidth, ScreenHeight, false);
        }
    };

    class HiddenTestWindow
    {
    public:
        HiddenTestWindow()
        {
            if (!IsWindowReady())
            {
                SetTraceLogLevel(LOG_WARNING);
                SetConfigFlags(FLAG_WINDOW_HIDDEN);
                InitWindow(1, 1, "FLX runtime drawing test");
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

    ObjectDefinition objectDefinition(const std::string& id, const std::string& script = "")
    {
        ObjectDefinition definition;
        definition.id = id;
        definition.visible = true;

        if (!script.empty())
        {
            definition.resolvedScriptPaths.push_back(script);
        }

        return definition;
    }

    void setRectangleVisual(ObjectDefinition& definition, Color color)
    {
        RepresentationElementDefinition element;
        element.kind = RepresentationElementKind::Primitive;
        element.primitive = "rectangle";

        definition.hasVisual = true;
        definition.visual.color = color;
        definition.visual.hasColor = true;
        definition.visual.representation = { element };
        definition.visual.hasRepresentation = true;
    }

    void setVisualDepth(ObjectDefinition& definition, int depth)
    {
        definition.hasVisual = true;
        definition.visual.depth = depth;
    }

    RuntimeObject runtimeObject(const std::string& name)
    {
        return RuntimeObject(
            name,
            Vector2{ 0.0f, 0.0f },
            Vector2{ 2.0f, 2.0f },
            WHITE
        );
    }

    RuntimeObject& requireObject(RuntimeWorld& world, const std::string& name)
    {
        RuntimeObject* object = world.findByName(name);
        require(object != nullptr, "runtime object should exist: " + name);
        return *object;
    }

    double localValue(RuntimeObject& object, const std::string& key)
    {
        const auto it = object.local.find(key);

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

    double globalValue(ScriptEngine& scripts, const std::string& key)
    {
        const auto value = scripts.readGlobalValue(key);

        if (!value)
        {
            return 0.0;
        }

        if (const auto* number = std::get_if<double>(&*value))
        {
            return *number;
        }

        return 0.0;
    }

    bool sameColor(Color left, Color right)
    {
        return left.r == right.r &&
            left.g == right.g &&
            left.b == right.b &&
            left.a == right.a;
    }

    struct ColorBounds
    {
        bool found = false;
        int minX = 0;
        int minY = 0;
        int maxX = 0;
        int maxY = 0;
        int count = 0;
    };

    void includePixel(ColorBounds& bounds, int x, int y)
    {
        if (!bounds.found)
        {
            bounds.found = true;
            bounds.minX = x;
            bounds.maxX = x;
            bounds.minY = y;
            bounds.maxY = y;
        }
        else
        {
            bounds.minX = std::min(bounds.minX, x);
            bounds.maxX = std::max(bounds.maxX, x);
            bounds.minY = std::min(bounds.minY, y);
            bounds.maxY = std::max(bounds.maxY, y);
        }

        ++bounds.count;
    }

    float centerX(const ColorBounds& bounds)
    {
        return
            (static_cast<float>(bounds.minX) + static_cast<float>(bounds.maxX)) /
            2.0f;
    }

    int width(const ColorBounds& bounds)
    {
        return bounds.maxX - bounds.minX + 1;
    }

    int height(const ColorBounds& bounds)
    {
        return bounds.maxY - bounds.minY + 1;
    }

    int countRenderedColor(
        RuntimeHarness& harness,
        Color color,
        int scale = 1,
        int width = 80,
        int height = 60
    )
    {
        HiddenTestWindow window;

        RenderTexture2D target = LoadRenderTexture(width, height);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        harness.draw(scale);
        EndTextureMode();

        Image image = LoadImageFromTexture(target.texture);

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

    bool nearlyEqual(float left, float right, float epsilon = 0.001f)
    {
        return std::abs(left - right) <= epsilon;
    }

    bool diagnosticsContain(const Diagnostics& diagnostics, const std::string& text)
    {
        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            if (diagnostic.message.find(text) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    ColorBounds renderedBounds(
        const std::vector<VisualPrimitive>& primitives,
        Color color,
        int renderWidth,
        int renderHeight,
        int scale = 1
    )
    {
        HiddenTestWindow window;

        std::vector<RuntimeObject> objects;
        RenderTexture2D target =
            LoadRenderTexture(renderWidth, renderHeight);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawingRenderer::render(primitives, objects, scale, ScreenWidth, ScreenHeight);
        EndTextureMode();

        Image image =
            LoadImageFromTexture(target.texture);

        ColorBounds bounds;

        for (int y = 0; y < image.height; ++y)
        {
            for (int x = 0; x < image.width; ++x)
            {
                if (sameColor(GetImageColor(image, x, y), color))
                {
                    includePixel(bounds, x, y);
                }
            }
        }

        UnloadImage(image);
        UnloadRenderTexture(target);

        return bounds;
    }

    std::pair<ColorBounds, ColorBounds> splitRenderedBoundsByY(
        const std::vector<VisualPrimitive>& primitives,
        Color color,
        int splitY,
        int renderWidth,
        int renderHeight,
        int scale
    )
    {
        HiddenTestWindow window;

        std::vector<RuntimeObject> objects;
        RenderTexture2D target =
            LoadRenderTexture(renderWidth, renderHeight);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawingRenderer::render(primitives, objects, scale, ScreenWidth, ScreenHeight);
        EndTextureMode();

        Image image =
            LoadImageFromTexture(target.texture);

        ColorBounds first;
        ColorBounds second;

        for (int y = 0; y < image.height; ++y)
        {
            for (int x = 0; x < image.width; ++x)
            {
                if (!sameColor(GetImageColor(image, x, y), color))
                {
                    continue;
                }

                if (y < splitY)
                {
                    includePixel(first, x, y);
                }
                else
                {
                    includePixel(second, x, y);
                }
            }
        }

        UnloadImage(image);
        UnloadRenderTexture(target);

        return { first, second };
    }

    void testDrawingContextRejectsCallsOutsideDrawTurn()
    {
        DrawingContext context;
        std::vector<VisualPrimitive> primitives;

        require(!context.emitWorldPixel(Vector2{ 1.0f, 1.0f }, WHITE), "inactive drawing context should reject world pixel");
        require(!context.emitWorldText(Vector2{ 1.0f, 1.0f }, "x", 8, WHITE), "inactive drawing context should reject world text");
        require(primitives.empty(), "inactive drawing context should not collect primitives");
    }

    void testTextBindingsPreserveLogicalSizesAndCoordinates()
    {
        for (int outputScale : { 1, 3 })
        {
            ScriptEngine scripts;
            scripts.setScreenScale(outputScale);
            scripts.loadScript("textSizes",
                "function draw(o) {"
                "  for (const size of [6, 8, 10, 11, 12]) {"
                "    draw_text(1.25, 2.75, 'MWil1 0123456789', size, 'red');"
                "    draw_text(o, 1.25, 2.75, 'MWil1 0123456789', size, 'red');"
                "  }"
                "  draw_text(1, 2, 'default');"
                "}");
            RuntimeObject object = runtimeObject("text");
            object.runtimeId = "text#1";
            object.position = { 10, 20 };
            object.angle = 90;
            DrawingContext context;
            std::vector<VisualPrimitive> primitives;
            scripts.setDrawingContext(&context);
            context.begin(object, primitives);
            scripts.callScriptFunction("textSizes", "draw", object);
            context.end();
            scripts.setDrawingContext(nullptr);
            require(primitives.size() == 11, "both text overloads and default size should emit");
            size_t i = 0;
            for (int size : { 6, 8, 10, 11, 12 })
            {
                const auto& world = primitives[i++];
                const auto& local = primitives[i++];
                require(world.fontSize == size && local.fontSize == size, "binding must not clamp or physically scale text size");
                require(nearlyEqual(world.a.x, 1.25f) && nearlyEqual(world.a.y, 2.75f), "world text preserves fractional logical coordinates");
                require(nearlyEqual(local.a.x, 7.25f) && nearlyEqual(local.a.y, 21.25f), "local text captures reference rotation");
                require(nearlyEqual(local.angle, 90), "local text retains rotation");
                require(sameColor(world.color, RED) && sameColor(local.color, RED), "bindings preserve text color");
            }
            require(primitives.back().fontSize == 10 && sameColor(primitives.back().color, WHITE), "default size and color remain unchanged");
        }
    }

    void testWorldAndLocalPrimitivesCaptureOwnerReferenceAndTransform()
    {
        RuntimeObject owner = runtimeObject("owner");
        RuntimeObject reference = runtimeObject("reference");
        owner.runtimeId = "owner#1";
        reference.runtimeId = "reference#1";
        reference.position = Vector2{ 10.0f, 20.0f };
        reference.angle = 90.0f;
        reference.size = Vector2{ 100.0f, 100.0f };
        reference.hasSize = true;

        DrawingContext context;
        std::vector<VisualPrimitive> primitives;
        context.begin(owner, primitives);

        require(context.emitWorldPixel(Vector2{ 2.0f, 3.0f }, RED), "world pixel should emit");
        require(context.emitLocalPixel(reference, Vector2{ 2.0f, 0.0f }, BLUE), "local pixel should emit");
        require(context.emitLocalRectangle(reference, Vector2{ 1.0f, 0.0f }, Vector2{ 4.0f, 6.0f }, GREEN), "local rectangle should emit");
        require(context.emitLocalText(reference, Vector2{ 0.0f, 1.0f }, "T", 9, YELLOW), "local text should emit");
        context.end();

        require(primitives.size() == 4, "four primitives should be collected");
        require(primitives[0].ownerRuntimeId == "owner#1", "world primitive should record draw owner");
        require(primitives[0].presentationSourceRuntimeId.empty(), "world primitive should not have presentation source");
        require(nearlyEqual(primitives[0].a.x, 2.0f) && nearlyEqual(primitives[0].a.y, 3.0f), "world primitive should keep world coordinates");

        require(primitives[1].ownerRuntimeId == "owner#1", "local primitive should still record draw owner");
        require(primitives[1].presentationSourceRuntimeId == "reference#1", "local primitive should record coordinate reference as presentation source");
        require(nearlyEqual(primitives[1].a.x, 10.0f) && nearlyEqual(primitives[1].a.y, 22.0f), "local primitive should rotate point through reference pivot");
        require(nearlyEqual(primitives[2].angle, 90.0f), "local rectangle should capture reference angle");
        require(nearlyEqual(primitives[2].size.x, 4.0f) && nearlyEqual(primitives[2].size.y, 6.0f), "reference width and height should not scale local rectangle size");
        require(nearlyEqual(primitives[3].angle, 90.0f), "local text should capture reference angle");
    }

    void testLocalPrimitiveCapturesTransformAtEmission()
    {
        RuntimeObject owner = runtimeObject("owner");
        RuntimeObject reference = runtimeObject("reference");
        owner.runtimeId = "owner#1";
        reference.runtimeId = "reference#1";
        reference.position = Vector2{ 5.0f, 5.0f };
        reference.size = Vector2{ 6.0f, 6.0f };
        reference.hasSize = true;
        reference.boundsMode = "wrap";
        reference.boundsOverflow = true;

        DrawingContext context;
        std::vector<VisualPrimitive> primitives;
        context.begin(owner, primitives);
        require(context.emitLocalPixel(reference, Vector2{ 0.0f, 0.0f }, RED), "first local pixel should emit");

        reference.position = Vector2{ 15.0f, 5.0f };
        reference.boundsOverflow = false;
        require(context.emitLocalPixel(reference, Vector2{ 0.0f, 0.0f }, BLUE), "second local pixel should emit");
        context.end();

        require(nearlyEqual(primitives[0].a.x, 5.0f), "first primitive should keep first reference position");
        require(nearlyEqual(primitives[1].a.x, 15.0f), "second primitive should use later reference position");
        require(primitives[0].presentationOverflow, "first primitive should keep first wrap overflow state");
        require(!primitives[1].presentationOverflow, "second primitive should keep later wrap overflow state");
        require(nearlyEqual(primitives[0].presentationPosition.x, 5.0f), "first primitive should snapshot presentation position");
        require(nearlyEqual(primitives[1].presentationPosition.x, 15.0f), "second primitive should snapshot later presentation position");
    }

    void testRendererScalesLogicalPixelToPhysicalPixels()
    {
        HiddenTestWindow window;

        std::vector<VisualPrimitive> primitives;
        VisualPrimitive pixel;
        pixel.kind = VisualPrimitiveKind::Pixel;
        pixel.a = Vector2{ 2.0f, 3.0f };
        pixel.color = RED;
        primitives.push_back(pixel);

        std::vector<RuntimeObject> objects;
        RenderTexture2D target = LoadRenderTexture(20, 20);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawingRenderer::render(primitives, objects, 3, ScreenWidth, ScreenHeight);
        EndTextureMode();

        Image image = LoadImageFromTexture(target.texture);

        int count = 0;
        for (int y = 0; y < image.height; ++y)
        {
            for (int x = 0; x < image.width; ++x)
            {
                if (sameColor(GetImageColor(image, x, y), RED))
                {
                    ++count;
                }
            }
        }

        UnloadImage(image);
        UnloadRenderTexture(target);

        require(count == 9, "one logical pixel at scale 3 should draw nine physical pixels");
    }

    void testRendererStretchesWorldExtentToMachineRaster()
    {
        HiddenTestWindow window;

        std::vector<VisualPrimitive> primitives;
        VisualPrimitive pixel;
        pixel.kind = VisualPrimitiveKind::Pixel;
        pixel.a = Vector2{ 160.0f, 90.0f };
        pixel.color = RED;
        primitives.push_back(pixel);

        std::vector<RuntimeObject> objects;
        RenderTexture2D target = LoadRenderTexture(640, 480);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawingRenderer::render(
            primitives,
            objects,
            WorldExtent{ 0.0f, 0.0f, 320.0f, 180.0f },
            640.0f,
            480.0f
        );
        EndTextureMode();

        Image image = LoadImageFromTexture(target.texture);

        ColorBounds bounds;

        for (int y = 0; y < image.height; ++y)
        {
            for (int x = 0; x < image.width; ++x)
            {
                if (sameColor(GetImageColor(image, x, y), RED))
                {
                    includePixel(bounds, x, y);
                }
            }
        }

        UnloadImage(image);
        UnloadRenderTexture(target);

        require(bounds.found, "stretched world pixel should render");
        require(nearlyEqual(centerX(bounds), 320.0f, 2.0f), "world center x should stretch to machine raster center");
        require(nearlyEqual(static_cast<float>(bounds.minY + bounds.maxY) / 2.0f, 240.0f, 2.0f), "world center y should stretch to machine raster center");
        require(nearlyEqual(pixel.a.x, 160.0f), "renderer stretch must not mutate world x");
        require(nearlyEqual(pixel.a.y, 90.0f), "renderer stretch must not mutate world y");
    }

    void testLocalPrimitiveWrapsWithReferenceButWorldPrimitiveDoesNot()
    {
        RuntimeHarness harness;
        harness.addScript(
            "wrapPaint",
            "function draw(o) {"
            "  draw_pixel(10, 5, 'red');"
            "  draw_pixel(o, 0, 0, 'blue');"
            "}"
        );

        ObjectDefinition root = objectDefinition("root", "wrapPaint");
        root.origin = Vector2{ 1.0f, 5.0f };
        root.hasOrigin = true;
        root.size = Vector2{ 6.0f, 6.0f };
        root.hasSize = true;
        root.boundsMode = "wrap";
        root.boundsOverflow = true;

        harness.addObject(root);
        require(harness.load().success, "runtime should load wrap drawing project");

        require(countRenderedColor(harness, RED, 1, 80, 60) == 1, "world primitive should not create wrap presentation copies");
        require(countRenderedColor(harness, BLUE, 1, 80, 60) == 2, "local primitive should share reference wrap presentation");
    }

    void testRepresentationPresentationSnapshotDoesNotChangeAfterDrawMutation()
    {
        RuntimeHarness harness;
        harness.addScript(
            "moveDuringDraw",
            "function draw(o) { position(o, 20, 5); }"
        );

        ObjectDefinition root = objectDefinition("root", "moveDuringDraw");
        root.origin = Vector2{ 1.0f, 5.0f };
        root.hasOrigin = true;
        root.size = Vector2{ 6.0f, 6.0f };
        root.hasSize = true;
        setRectangleVisual(root, RED);
        root.boundsMode = "wrap";
        root.boundsOverflow = true;

        harness.addObject(root);
        require(harness.load().success, "runtime should load representation snapshot project");

        require(countRenderedColor(harness, RED, 1, 80, 60) > 36, "representation should use snapshot captured before draw mutation");
    }

    void testLocalPrimitivePresentationSnapshotDoesNotChangeAfterReferenceMutation()
    {
        RuntimeHarness harness;
        harness.addScript(
            "moveBetweenPrimitives",
            "function draw(o) {"
            "  draw_pixel(o, 0, 0, 'red');"
            "  position(o, 20, 5);"
            "  draw_pixel(o, 0, 0, 'blue');"
            "}"
        );

        ObjectDefinition root = objectDefinition("root", "moveBetweenPrimitives");
        root.origin = Vector2{ 1.0f, 5.0f };
        root.hasOrigin = true;
        root.size = Vector2{ 6.0f, 6.0f };
        root.hasSize = true;
        root.boundsMode = "wrap";
        root.boundsOverflow = true;

        harness.addObject(root);
        require(harness.load().success, "runtime should load local primitive snapshot project");

        require(countRenderedColor(harness, RED, 1, 80, 60) == 2, "first local primitive should keep first wrap presentation snapshot");
        require(countRenderedColor(harness, BLUE, 1, 80, 60) == 1, "second local primitive should use later non-overflow presentation snapshot");
    }

    void testRepresentationAndLocalPrimitiveWrapUseSameSnapshot()
    {
        RuntimeHarness harness;
        harness.addScript(
            "paintSamePoint",
            "function draw(o) { draw_pixel(o, 0, 0, 'red'); }"
        );

        ObjectDefinition root = objectDefinition("root", "paintSamePoint");
        root.origin = Vector2{ 1.0f, 5.0f };
        root.hasOrigin = true;
        root.size = Vector2{ 6.0f, 6.0f };
        root.hasSize = true;
        setRectangleVisual(root, BLUE);
        root.boundsMode = "wrap";
        root.boundsOverflow = true;

        harness.addObject(root);
        require(harness.load().success, "runtime should load matching wrap snapshot project");

        require(countRenderedColor(harness, RED, 1, 80, 60) == 2, "local primitive should cover both representation presentation instances from the same snapshot");
        require(countRenderedColor(harness, BLUE, 1, 80, 60) > 36, "representation should use the same wrap presentation snapshot as the local primitive");
    }

    void testDepthControlsStableObjectVisualTurnOrder()
    {
        RuntimeHarness harness;
        harness.addScript(
            "createInOrder",
            // Equal depth preserves world entry order, not dictionary insertion.
            "function born(o) {"
            "  spawn(o, 'sameA'); spawn(o, 'low'); spawn(o, 'sameB'); spawn(o, 'high');"
            "}"
        );
        harness.addScript(
            "orderProbe",
            "function draw(o) {"
            "  if (read_global('drawOrder') == null) write_global('drawOrder', 0);"
            "  write_local(o, 'order', read_global('drawOrder'));"
            "  write_global('drawOrder', read_global('drawOrder') + 1);"
            "}"
        );

        ObjectDefinition root = objectDefinition("root", "createInOrder");
        root.childResources["sameA"] = "sameA";
        root.childResources["low"] = "low";
        root.childResources["sameB"] = "sameB";
        root.childResources["high"] = "high";

        ObjectDefinition sameA = objectDefinition("sameA", "orderProbe");
        sameA.spawnMode = "manual";
        ObjectDefinition low = objectDefinition("low", "orderProbe");
        low.spawnMode = "manual";
        setVisualDepth(low, -10);
        ObjectDefinition sameB = objectDefinition("sameB", "orderProbe");
        sameB.spawnMode = "manual";
        ObjectDefinition high = objectDefinition("high", "orderProbe");
        high.spawnMode = "manual";
        setVisualDepth(high, 10);

        harness.addObject(root);
        harness.addObject(sameA);
        harness.addObject(low);
        harness.addObject(sameB);
        harness.addObject(high);

        require(harness.load().success, "runtime should load depth order project");
        harness.draw();

        require(localValue(requireObject(harness.world, "low"), "order") == 0.0, "lower depth should draw first");
        require(localValue(requireObject(harness.world, "sameA"), "order") < localValue(requireObject(harness.world, "sameB"), "order"), "same depth should keep creation order");
        require(localValue(requireObject(harness.world, "high"), "order") > localValue(requireObject(harness.world, "sameB"), "order"), "higher depth should draw last");
    }

    void testDepthChangeIsLiveButAffectsNextDrawPassOrder()
    {
        RuntimeHarness harness;
        harness.addScript(
            "raiseSelf",
            "function draw(o) {"
            "  if (read_global('order') == null) write_global('order', 0);"
            "  write_local(o, 'firstPassOrder', read_global('order'));"
            "  write_global('order', read_global('order') + 1);"
            "  depth(o, 10);"
            "  write_local(o, 'liveDepth', o.depth);"
            "}"
        );
        harness.addScript(
            "stableProbe",
            "function draw(o) {"
            "  if (read_global('order') == null) write_global('order', 0);"
            "  write_local(o, 'lastOrder', read_global('order'));"
            "  write_global('order', read_global('order') + 1);"
            "}"
        );

        ObjectDefinition root = objectDefinition("root");
        root.childResources["first"] = "first";
        root.childResources["second"] = "second";

        ObjectDefinition first = objectDefinition("first", "raiseSelf");
        setVisualDepth(first, 0);
        ObjectDefinition second = objectDefinition("second", "stableProbe");
        setVisualDepth(second, 5);

        harness.addObject(root);
        harness.addObject(first);
        harness.addObject(second);

        require(harness.load().success, "runtime should load depth mutation project");
        harness.draw();

        RuntimeObject& firstRuntime = requireObject(harness.world, "first");
        RuntimeObject& secondRuntime = requireObject(harness.world, "second");
        require(localValue(firstRuntime, "firstPassOrder") == 0.0, "depth mutation should not reorder current draw pass");
        require(localValue(firstRuntime, "liveDepth") == 10.0, "depth mutation should be visible through same JS object");
        require(localValue(secondRuntime, "lastOrder") == 1.0, "second object should keep current pass order");

        harness.scripts.writeGlobalValue("order", 0.0);
        harness.draw();

        require(localValue(secondRuntime, "lastOrder") == 0.0, "new depth should affect next draw pass");
    }

    void testEarlierVisualTurnCanHideLaterObject()
    {
        RuntimeHarness harness;
        harness.addScript(
            "hider",
            "function draw(o) {"
            "  let targets = find_name('target');"
            "  if (targets.length > 0) hide(targets[0]);"
            "}"
        );
        harness.addScript(
            "targetDraw",
            "function draw(o) { write_local(o, 'drawn', 1); }"
        );

        ObjectDefinition root = objectDefinition("root");
        root.childResources["hider"] = "hider";
        root.childResources["target"] = "target";

        ObjectDefinition hider = objectDefinition("hider", "hider");
        setVisualDepth(hider, -1);
        ObjectDefinition target = objectDefinition("target", "targetDraw");
        setVisualDepth(target, 1);

        harness.addObject(root);
        harness.addObject(hider);
        harness.addObject(target);

        require(harness.load().success, "runtime should load visibility drawing project");
        harness.draw();

        require(localValue(requireObject(harness.world, "target"), "drawn") == 0.0, "hidden later object should not receive visual turn");
    }

    void testEarlierVisualTurnCanKillLaterObject()
    {
        RuntimeHarness harness;
        harness.addScript(
            "killer",
            "function draw(o) {"
            "  let targets = find_name('target');"
            "  if (targets.length > 0) kill(targets[0]);"
            "}"
        );
        harness.addScript(
            "targetDraw",
            "function draw(o) { write_local(o, 'drawn', 1); }"
        );

        ObjectDefinition root = objectDefinition("root");
        root.childResources["killer"] = "killer";
        root.childResources["target"] = "target";

        ObjectDefinition killer = objectDefinition("killer", "killer");
        setVisualDepth(killer, -1);
        ObjectDefinition target = objectDefinition("target", "targetDraw");
        setVisualDepth(target, 1);

        harness.addObject(root);
        harness.addObject(killer);
        harness.addObject(target);

        require(harness.load().success, "runtime should load kill drawing project");
        harness.draw();

        require(localValue(requireObject(harness.world, "target"), "drawn") == 0.0, "killed later object should not receive visual turn");
    }

    void testImmediateDrawingOutsideDrawCallbackDoesNotRender()
    {
        RuntimeHarness harness;
        harness.addScript(
            "actionPaint",
            "function action(o) { draw_pixel(2, 2, 'red'); }"
        );

        ObjectDefinition root = objectDefinition("root", "actionPaint");
        harness.addObject(root);

        require(harness.load().success, "runtime should load action draw project");
        harness.scripts.setFrameDelta(0.016f);
        harness.world.update(harness.scripts, 0.016f);

        require(countRenderedColor(harness, RED, 1, 80, 60) == 0, "draw_pixel outside draw callback should not render");
    }

    void testHudPatternReadsStateInDrawAfterAction()
    {
        RuntimeHarness harness;
        harness.addScript(
            "hud",
            "function action(o) { write_global('hud', 1); }"
            "function draw(o) {"
            "  if (read_global('hud') == 1) { draw_pixel(3, 3, 'red'); }"
            "}"
        );

        ObjectDefinition root = objectDefinition("root", "hud");
        harness.addObject(root);

        require(harness.load().success, "runtime should load HUD pattern project");
        harness.scripts.setFrameDelta(0.016f);
        harness.world.update(harness.scripts, 0.016f);

        require(countRenderedColor(harness, RED, 1, 80, 60) == 1, "HUD should be drawn from draw() using state produced by action()");
    }

    void testRepresentationAndImmediateDrawShareSameObjectDepth()
    {
        RuntimeHarness harness;
        harness.addScript(
            "paintRed",
            "function draw(o) { draw_pixel(5, 5, 'red'); }"
        );

        ObjectDefinition root = objectDefinition("root");
        root.childResources["lowPainter"] = "lowPainter";
        root.childResources["highBlock"] = "highBlock";

        ObjectDefinition lowPainter = objectDefinition("lowPainter", "paintRed");
        setVisualDepth(lowPainter, -5);

        ObjectDefinition highBlock = objectDefinition("highBlock");
        setVisualDepth(highBlock, 5);
        highBlock.origin = Vector2{ 5.0f, 5.0f };
        highBlock.hasOrigin = true;
        highBlock.size = Vector2{ 3.0f, 3.0f };
        highBlock.hasSize = true;
        setRectangleVisual(highBlock, BLUE);

        harness.addObject(root);
        harness.addObject(lowPainter);
        harness.addObject(highBlock);

        require(harness.load().success, "runtime should load shared depth project");
        require(countRenderedColor(harness, BLUE, 1, 80, 60) == 9, "higher depth representation should cover lower depth immediate primitive");
        require(countRenderedColor(harness, RED, 1, 80, 60) == 0, "immediate drawing should not run as a final overlay pipeline");
    }

    void testJsonDepthIsVisualLevelAndRootDepthIsRejected()
    {
        const std::filesystem::path root = testRoot() / "drawing_depth_contract";
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=DrawingDepth\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"size\": { \"width\": 2, \"height\": 2 },\n"
            "  \"visual\": {\n"
            "    \"depth\": 7,\n"
            "    \"color\": \"white\",\n"
            "    \"representation\": [{ \"primitive\": \"rectangle\" }]\n"
            "  }\n"
            "}\n"
        );

        CompilationResult valid = flx::test::compile(root / "game.flx");
        require(valid.success, "visual depth should compile");
        const ObjectDefinition* compiledRoot = valid.project.resources.findObject(valid.project.rootId);
        require(compiledRoot != nullptr, "compiled root should exist");
        require(compiledRoot->visual.depth == 7, "visual depth should reach compiled definition");

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"depth\": 7\n"
            "}\n"
        );

        CompilationResult invalid = flx::test::compile(root / "game.flx");
        require(!invalid.success, "root depth should be rejected");
        require(diagnosticsContain(invalid.diagnostics, "visual.depth"), "root depth rejection should point to visual.depth");
    }

    void testDepthPropertyIsReadonlyFromJavaScript()
    {
        RuntimeHarness harness;
        harness.addScript(
            "writeDepth",
            "function draw(o) {"
            "  o.depth = 50;"
            "  write_local(o, 'depthAfterDirectWrite', o.depth);"
            "}"
        );

        ObjectDefinition root = objectDefinition("root", "writeDepth");
        setVisualDepth(root, 7);
        harness.addObject(root);

        require(harness.load().success, "runtime should load readonly depth project");
        harness.draw();

        RuntimeObject& runtimeRoot = requireObject(harness.world, "root");
        require(runtimeRoot.depth == 7, "direct JS write should not modify runtime depth");
        require(localValue(runtimeRoot, "depthAfterDirectWrite") == 7.0, "readonly depth view should still expose runtime value");
    }

    void testCircleOutlineScalesWithOutputScale()
    {
        HiddenTestWindow window;

        std::vector<VisualPrimitive> primitives;
        VisualPrimitive circle;
        circle.kind = VisualPrimitiveKind::Representation;
        circle.primitive = "ellipse";
        circle.primitiveMode = "outline";
        circle.a = Vector2{ 10.0f, 10.0f };
        circle.size = Vector2{ 10.0f, 10.0f };
        circle.color = RED;
        primitives.push_back(circle);

        std::vector<RuntimeObject> objects;
        RenderTexture2D target = LoadRenderTexture(80, 80);

        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawingRenderer::render(primitives, objects, 3, ScreenWidth, ScreenHeight);
        EndTextureMode();

        Image image = LoadImageFromTexture(target.texture);

        int count = 0;
        for (int y = 0; y < image.height; ++y)
        {
            for (int x = 0; x < image.width; ++x)
            {
                if (sameColor(GetImageColor(image, x, y), RED))
                {
                    ++count;
                }
            }
        }

        UnloadImage(image);
        UnloadRenderTexture(target);

        require(count > 120, "circle outline at scale 3 should draw a logical-width outline, not a one-pixel physical line");
    }

    void testOnePointGeometryBuildsAndDrawsAsLogicalPoint()
    {
        RuntimeObject object =
            runtimeObject("point");
        object.runtimeId = "point#1";
        object.position = Vector2{ 5.0f, 5.0f };
        object.hasVisualColor = true;
        object.visualColor = RED;

        for (const std::string& mode : std::vector<std::string>{ "open", "close", "fill" })
        {
            RepresentationElementDefinition element;
            element.kind = RepresentationElementKind::Geometry;
            element.geometryMode = mode;
            element.geometry = { Vector2{ 1.0f, 2.0f } };

            object.representation = { element };

            const std::vector<VisualPrimitive> primitives =
                RepresentationPrimitiveBuilder::build(object, WHITE);

            require(primitives.size() == 1, "one-point geometry should build one presentation primitive");
            require(primitives[0].primitive == "geometry", "one-point geometry should remain a geometry primitive");
            require(primitives[0].points.size() == 1, "builder should preserve the single geometry point");

            const ColorBounds bounds =
                renderedBounds(primitives, RED, 40, 40, 1);

            require(bounds.found, "one-point geometry should draw one logical point");
            require(bounds.count == 1, "one-point geometry should not generate artificial area");
        }
    }

    void testRotatedEllipseFillAndOutlineRespectAngle()
    {
        std::vector<VisualPrimitive> filledPrimitives;

        VisualPrimitive filled;
        filled.kind = VisualPrimitiveKind::Representation;
        filled.primitive = "ellipse";
        filled.primitiveMode = "fill";
        filled.a = Vector2{ 30.0f, 30.0f };
        filled.size = Vector2{ 30.0f, 10.0f };
        filled.angle = 90.0f;
        filled.color = RED;
        filledPrimitives.push_back(filled);

        const ColorBounds filledBounds =
            renderedBounds(filledPrimitives, RED, 80, 80, 1);

        require(filledBounds.found, "rotated filled ellipse should draw");
        require(height(filledBounds) > width(filledBounds) * 2, "filled ellipse should rotate its long axis with object angle");

        std::vector<VisualPrimitive> outlinedPrimitives;

        VisualPrimitive outlined =
            filled;
        outlined.primitiveMode = "outline";
        outlined.color = GREEN;
        outlinedPrimitives.push_back(outlined);

        const ColorBounds outlineBounds =
            renderedBounds(outlinedPrimitives, GREEN, 80, 80, 1);

        require(outlineBounds.found, "rotated outlined ellipse should draw");
        require(height(outlineBounds) > width(outlineBounds) * 2, "outlined ellipse should rotate its long axis with object angle");
    }

    void testMultilineTextCentersEachLine()
    {
        std::vector<VisualPrimitive> primitives;

        VisualPrimitive text;
        text.kind = VisualPrimitiveKind::Representation;
        text.primitive = "text";
        text.text = "LONG\nX";
        text.fontSize = 8;
        text.a = Vector2{ 30.0f, 30.0f };
        text.color = RED;
        primitives.push_back(text);

        const ColorBounds fullBounds =
            renderedBounds(primitives, RED, 80, 80, 1);

        require(fullBounds.found, "multiline text should draw visible glyphs");

        const int splitY =
            (fullBounds.minY + fullBounds.maxY + 1) / 2;

        const auto [top, bottom] =
            splitRenderedBoundsByY(primitives, RED, splitY, 80, 80, 1);

        require(top.found, "multiline text should draw the first line");
        require(bottom.found, "multiline text should draw the second line");
        const int wideLine =
            std::max(width(top), width(bottom));
        const int narrowLine =
            std::min(width(top), width(bottom));

        require(
            wideLine > narrowLine * 2,
            "test fixture should use lines with clearly different widths"
        );
        require(
            std::abs(centerX(top) - centerX(bottom)) <= 1.5f,
            "each multiline text line should be centered on the same local X axis: top=" +
                std::to_string(centerX(top)) +
                " bottom=" +
                std::to_string(centerX(bottom))
        );
    }

    void testTextCasesDoNotDependOnRuntimeObjectSize()
    {
        std::vector<VisualPrimitive> primitives;

        for (const std::string& value : std::vector<std::string>{ "A", "A\nB", "A\n\nB" })
        {
            VisualPrimitive text;
            text.kind = VisualPrimitiveKind::Representation;
            text.primitive = "text";
            text.text = value;
            text.fontSize = 8;
            text.a = Vector2{ 20.0f, 20.0f };
            text.angle = 25.0f;
            text.color = BLUE;
            primitives = { text };

            const ColorBounds bounds =
                renderedBounds(primitives, BLUE, 60, 60, 1);

            require(bounds.found, "single and multiline text should draw without object size");
        }

        VisualPrimitive empty;
        empty.kind = VisualPrimitiveKind::Representation;
        empty.primitive = "text";
        empty.text = "";
        empty.fontSize = 8;
        empty.a = Vector2{ 20.0f, 20.0f };
        empty.color = YELLOW;

        const ColorBounds emptyBounds =
            renderedBounds({ empty }, YELLOW, 60, 60, 1);

        require(!emptyBounds.found, "empty text should remain valid without visible glyphs");
    }
    void testBitmapTextureTransformTransparencyAndOrder()
    {
        HiddenTestWindow window;
        auto data=std::make_shared<BitmapDefinition>();
        data->width=2;data->height=2;data->pixels={RED,GREEN,BLUE,BLANK};
        RuntimeObject object("bitmap",{20,15},{8,4},GREEN);
        object.position={20,15};object.hasSize=true;
        object.hasVisualColor=true;object.visualColor=GREEN;
        RepresentationElementDefinition background;
        background.kind=RepresentationElementKind::Primitive;background.primitive="rectangle";
        background.hasColor=true;background.color=YELLOW;
        RepresentationElementDefinition bitmap;bitmap.kind=RepresentationElementKind::Bitmap;bitmap.bitmap=data;
        object.representation={background,bitmap};
        BitmapTextureCache cache;
        std::vector<RuntimeObject> objects;
        auto render=[&](int width)
        {
            RenderTexture2D target=LoadRenderTexture(width,30);
            BeginTextureMode(target);ClearBackground(MAGENTA);
            DrawingRenderer::render(RepresentationPrimitiveBuilder::build(object,WHITE),objects,
                WorldExtent{0,0,40,30},static_cast<float>(width),30,&cache);
            EndTextureMode();
            Image image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            UnloadRenderTexture(target);return image;
        };
        Image image=render(40);
        require(sameColor(GetImageColor(image,18,14),RED),"bitmap top-left and no visual.color tint");
        require(sameColor(GetImageColor(image,22,14),GREEN),"bitmap top-right orientation");
        require(sameColor(GetImageColor(image,18,16),BLUE),"bitmap bottom-left orientation");
        require(sameColor(GetImageColor(image,22,16),YELLOW),"blank uses normal alpha blending over earlier representation");
        UnloadImage(image);
        const unsigned texture=cache.get(data).id;
        image=render(40);UnloadImage(image);
        require(cache.get(data).id==texture,"immutable bitmap texture reused across frames");
        object.angle=90;
        image=render(80);
        ColorBounds red;
        for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x)
            if(sameColor(GetImageColor(image,x,y),RED))includePixel(red,x,y);
        require(red.found && width(red)==4 && height(red)==4,
            "bitmap rotates in world space before anisotropic raster scaling");
        UnloadImage(image);
        object.angle=0;
        auto opaque=std::make_shared<BitmapDefinition>(*data);opaque->pixels[3]=BLACK;
        object.representation[1].bitmap=opaque;
        image=render(40);
        require(sameColor(GetImageColor(image,22,16),BLACK),"opaque Machine-projected blank is drawn, never skipped");
        UnloadImage(image);
        object.representation={bitmap,background};
        image=render(40);
        require(sameColor(GetImageColor(image,18,14),YELLOW),"later primitive covers earlier bitmap");
        UnloadImage(image);
    }

    void testBitmapMicroexamplesSourceAndCompiledRendering()
    {
        HiddenTestWindow window;
        const auto previews=testRoot()/"bitmap-previews";
        std::filesystem::create_directories(previews);
        const std::vector<std::string> examples={
            "pixel-art/pixel-art", "palette-composition/palette-composition",
            "bitmap-patterns/bitmap-patterns", "mixed-representations/mixed-representations",
            "machine-colors/machine-colors", "machine-colors/machine-colors-opaque",
            "machine-colors/machine-colors-restricted"};
        for(const auto& name:examples)
        {
            const auto path=std::filesystem::path(FLX_SOURCE_DIR)/"microexamples/drawing/bitmap"/(name+".flx");
            const auto result=compile(path);
            require(result.success,"microexample should compile: "+name);
            const auto stem=std::filesystem::path(name).filename().string();
            const auto compiled=previews/(stem+".flxc");Diagnostics diagnostics;
            require(CompiledProjectWriter::write(compiled.string(),result.project,diagnostics),"write bitmap showcase");
            const auto decoded=CompiledProjectReader::read(compiled.string());
            require(decoded.success && decoded.metadata.formatVersion==14,"read bitmap showcase format 14");
            auto render=[&](const CompiledProject& project,bool capture)
            {
                FlxSession session;FlxSessionOptions options;options.enableAudio=false;
                require(session.load(project,options),"load bitmap showcase without audio: "+name);
                for(int frame=0;frame<3;++frame)require(session.update(1.0f/60.0f)&&session.render(),"render bitmap showcase: "+name);
                Image image=LoadImageFromTexture(session.output());ImageFlipVertical(&image);
                require(image.width==640 && image.height==480,"showcase output dimensions");
                if(capture)require(ExportImage(image,(previews/(stem+".png")).string().c_str()),"export visual QA preview");
                std::vector<unsigned char> pixels;
                for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x)
                {
                    const Color c=GetImageColor(image,x,y);
                    pixels.insert(pixels.end(),{c.r,c.g,c.b,c.a});
                }
                UnloadImage(image);session.unload();return pixels;
            };
            require(render(result.project,true)==render(decoded.project,false),"source and compiled image must match: "+name);
        }
    }

    void testAnimationJavaScriptSnapshots()
    {
        HiddenTestWindow window;
        RuntimeHarness h;
        h.addScript("probe",R"(function draw(o) {
            const frame=read_local(o,'draws')||0;
            if(frame===0) play_animation(o,'pose');
            if(frame===1) {pause_animation(o);spawn(o,'child');}
            if(frame===2) stop_animation(o);
            write_local(o,'draws',frame+1);
            write_local(o,'left',animation_left(o));
            write_local(o,'active',animation_active(o));
            draw_pixel(o,0,0,'red');
        })");
        h.addScript("childProbe","function born(o){play_animation(o,'pose');}");
        auto root=objectDefinition("root","probe");root.origin={20,15};root.hasOrigin=true;root.size={6,6};root.hasSize=true;
        root.hasVisual=true;RepresentationElementDefinition rectangle;rectangle.primitive="rectangle";
        root.visual.representation={rectangle};root.hasAnimations=true;
        AnimationDefinition a;a.duration=1;AnimationKeyDefinition k;k.at=0;k.transform.emplace();k.transform->offsetX=8;k.color=BLUE;a.keys={k};root.animations["pose"]=a;
        root.childResources["child"]="child";
        auto child=objectDefinition("child","childProbe");child.spawnMode="manual";child.hasAnimations=true;child.animations=root.animations;
        h.addObject(root);h.addObject(child);require(h.load().success,"load JS draw snapshot");
        auto target=LoadRenderTexture(40,30);
        for(int frame=0;frame<4;++frame) {
            BeginTextureMode(target);ClearBackground(BLACK);h.draw();EndTextureMode();
            auto image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            require(sameColor(GetImageColor(image,20,15),RED),"immediate local pixel ignores animated offset");
            require(sameColor(GetImageColor(image,28,15),frame==1||frame==2?BLUE:BLACK),"draw play/stop respects previous visual snapshot");
            require(sameColor(GetImageColor(image,21,15),frame==0||frame==3?WHITE:BLACK),"base returns only on next capture after draw stop");
            UnloadImage(image);
            auto& o=requireObject(h.world,"root");
            if(frame==0)require(o.animation.left()==1&&localValue(o,"active")==1,"draw play updates queries immediately without time advance");
            if(frame==1)require(o.animation.paused()&&o.animation.left()==1,"draw pause freezes");
            if(frame==2)require(!o.animation.current()&&localValue(o,"active")==0,"draw stop queries immediate");
        }
        h.scripts.setFrameDelta(.25);h.world.update(h.scripts,.25);
        require(requireObject(h.world,"child").animation.left()==.75,"draw-created object starts advancing only next update");
        UnloadRenderTexture(target);
    }

    void testAnimationMicroexamplesGraphics()
    {
        HiddenTestWindow window;
        for(const std::string name:{"bitmap-invader","interpolation","loop-base","loop-current","pause-resume","representation","color"}) {
            const auto compiled=flx::test::compile(std::filesystem::path(FLX_SOURCE_DIR)/"microexamples/animation"/name/(name+".flx"));
            require(compiled.success,"compile animation visual microexample "+name);
            Diagnostics diagnostics;const auto binary=testRoot()/(name+"-visual.flxc");
            require(CompiledProjectWriter::write(binary.string(),compiled.project,diagnostics),"write example");
            const auto decoded=CompiledProjectReader::read(binary.string());require(decoded.success,"read example");
            std::vector<std::vector<unsigned char>> expected;int mode=0;
            for(const auto* project:{&compiled.project,&decoded.project}) {
                FlxSession session;FlxSessionOptions options;options.enableAudio=false;options.savePath=testRoot()/"animation-example-saves";
                require(session.load(*project,options),"load animation example");
                BitmapTextureCache invaderCache;
                std::array<unsigned int,5> invaderTextures{};
                for(int frame=0;frame<=16;++frame) {
                    require(session.render(),"render Animation example");
                    auto image=LoadImageFromTexture(session.output());ImageFlipVertical(&image);
                    std::vector<unsigned char> pixels;pixels.reserve(image.width*image.height*4);
                    for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x) {
                        const auto c=GetImageColor(image,x,y);pixels.insert(pixels.end(),{c.r,c.g,c.b,c.a});
                    }
                    if(mode==0) expected.push_back(pixels);else require(pixels==expected[frame],"open/compiled rendered pixels "+name);
                    if(name=="bitmap-invader") {
                        auto* o=session.testObject("sample");require(o,"bitmap sample");
                        const auto appearance=o->animation.appearance(AnimationBaseAppearance::capture(o->representation));
                        const auto bitmap=appearance.replaceable->at(0).bitmap;
                        require(bitmap&&bitmap->width==11&&bitmap->height==8,"invader bitmap dimensions");
                        for(int y=0;y<8;++y)for(int x=0;x<11;++x) {
                            const auto expectedColor=bitmap->pixels[y*11+x];
                            const int px=static_cast<int>((94+(x+.5)*12)*image.width/320);
                            const int py=static_cast<int>((72+(y+.5)*12)*image.height/240);
                            require(sameColor(GetImageColor(image,px,py),expectedColor.a?expectedColor:BLACK),"visible invader palette/transparency/frame/orientation");
                        }
                        const auto texture=invaderCache.get(bitmap);
                        if(frame<5) invaderTextures[frame]=texture.id;
                        else require(texture.id==invaderTextures[frame%5],"warm loop reuses the five frame textures");
                        if(frame>=5)require(pixels==expected[frame%5],"five-frame loop returns to exact rendered frame");
                    }
                    if(name=="color") {
                        const auto explicitColor=GetImageColor(image,image.width*120/320,image.height/2);
                        require(sameColor(explicitColor,WHITE),"explicit white stays white under animated inherited color");
                        if(frame==4)require(!sameColor(GetImageColor(image,image.width*129/320,image.height/2),WHITE),"inherited outer color actually animates");
                    }
                    if(name=="representation")require(sameColor(GetImageColor(image,image.width*120/320,image.height/2),GREEN),"persistent last stays above representation changes");
                    if(frame==2&&name=="interpolation") {
                        require(sameColor(GetImageColor(image,image.width*140/320,image.height/2),WHITE),"JS-driven interpolated rectangle has moved to expected center");
                        require(sameColor(GetImageColor(image,image.width*120/320,image.height/2),BLACK),"old position cleared without spatial mutation");
                    }
                    UnloadImage(image);if(frame<16)require(session.update(.25),"advance Animation example");
                }
                if(name=="bitmap-invader")
                    for(int a=0;a<5;++a)for(int b=a+1;b<5;++b)
                        require(expected[a]!=expected[b],"all five invader frames are visibly distinct");
                require(expected[0]!=expected[4],"microexample visibly changes: "+name);
                session.unload();++mode;
            }
        }
    }

    void testAnimationAcceptanceOpenCompiled()
    {
        HiddenTestWindow window;
        const auto manifest=createMinimalProject("animation_audit_snapshot");
        writeFile(manifest.parent_path()/"game/root.json",R"({
          "delimit":true,"origin":{"x":32,"y":32},"size":{"width":64,"height":64},
          "visual":{"representation":[{"primitive":"rectangle","size":{"width":8,"height":8}}]},
          "behavior":{"scripts":["audit"]},
          "animations":{
            "pose":{"duration":1,"keys":[{"at":0,"transform":{"offset":{"x":8,"y":2},"rotation":45,"scale":{"x":1.25,"y":0.75}},"color":"#ff0000","representation":[{"primitive":"ellipse","size":{"width":8,"height":8}}]}]},
            "turn":{"duration":1,"from":"current","keys":[{"at":0,"transform":{"rotation":0}},{"at":2,"transform":{"offset":{"y":1000}}}]},
            "huge":{"duration":1,"keys":[{"at":0,"transform":{"scale":{"x":1e300}},"color":"blue"}]}
          }
        })");
        writeFile(manifest.parent_path()/"game/audit.js",R"(
          function draw(o) {
            const frame=read_local(o,'draws')||0;
            if(frame===0) play_animation(o,'pose');
            if(frame===1) play_animation(o,'turn');
            if(frame===2) play_animation(o,'huge');
            if(frame===3) pause_animation(o);
            if(frame===4) {play_animation(o,'huge');stop_animation(o);}
            write_local(o,'draws',frame+1);
            write_local(o,'current',animation_current(o)===null?'absent':animation_current(o));
            write_local(o,'left',animation_left(o));
          }
        )");
        const auto source=flx::test::compile(manifest);require(source.success,"compile acceptance snapshot/recovery source");
        Diagnostics diagnostics;const auto binary=manifest.parent_path()/"audit.flxc";
        require(CompiledProjectWriter::write(binary.string(),source.project,diagnostics),"write acceptance binary");
        std::filesystem::remove_all(manifest.parent_path()/"game");
        const auto decoded=CompiledProjectReader::read(binary.string());require(decoded.success,"load acceptance without source files");
        std::vector<std::vector<unsigned char>> expected;int mode=0;
        for(const auto* project:{&source.project,&decoded.project}) {
            StreamCapture capture;
            FlxSession session;FlxSessionOptions options;options.enableAudio=false;options.savePath=testRoot()/"audit-saves";
            require(session.load(*project,options),"acceptance load");
            for(int frame=0;frame<6;++frame) {
                require(session.render(),"numeric graphical failure must not fail session");
                auto* o=session.testObject("root");require(o,"acceptance root");
                const auto current=o->animation.current();
                if(frame==0)require(current=="pose"&&o->animation.left()==1,"draw play queries immediately, time zero");
                if(frame==1) {
                    const auto base=AnimationBaseAppearance::capture(o->representation,o->hasVisualColor?o->visualColor:WHITE,o->hasVisualColor);
                    const auto pose=o->animation.appearance(base);
                    require(current=="turn"&&o->animation.reference().transform.rotation==45,"draw current captures effective controlled rotation");
                    require(pose.transform.offsetX==0&&pose.transform.offsetY==0&&pose.transform.scaleX==1&&pose.transform.scaleY==1,"current drops uncontrolled axes and scales, inactive keys do not control");
                    require(sameColor(pose.color,WHITE)&&pose.replaceable->at(0).primitive=="rectangle","current drops previous color and representation");
                }
                if(frame==3)require(o->animation.paused()&&o->animation.left()==1,"numeric fallback pause retains clock");
                if(frame>=4)require(!current,"draw stop immediate query");
                auto image=LoadImageFromTexture(session.output());ImageFlipVertical(&image);
                auto pixel=[&](int x,int y){return GetImageColor(image,image.width*x/64,image.height*y/64);};
                require(sameColor(pixel(frame==1?40:32,32),frame==1?Color{255,0,0,255}:WHITE),"snapshot/substitution/atomic numeric fallback exact pixels");
                require(sameColor(pixel(frame==1?32:40,32),BLACK),"old offset does not leak through current or recovery");
                std::vector<unsigned char> pixels;
                for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x){auto c=GetImageColor(image,x,y);pixels.insert(pixels.end(),{c.r,c.g,c.b,c.a});}
                if(mode==0)expected.push_back(pixels);else require(pixels==expected[frame],"acceptance source/binary pixels");
                UnloadImage(image);
            }
            const auto log=capture.output.str()+capture.error.str();
            require(countOccurrences(log,"FLX-RUNTIME-00021")==1,"one graphical numeric diagnostic over repeated renders and pause/resume");
            session.unload();++mode;
        }
    }

}

bool requiresGraphics(const std::string& name)
{
    return
        name == "Bitmap microexamples source and compiled rendering" ||
        name == "Bitmap texture transforms transparency and order" ||
        name == "Renderer scales logical pixel to physical pixels" ||
        name == "Renderer stretches world extent to machine raster" ||
        name == "Local primitive wraps with reference but world primitive does not" ||
        name == "Representation presentation snapshot does not change after draw mutation" ||
        name == "Local primitive presentation snapshot does not change after reference mutation" ||
        name == "Representation and local primitive wrap use same snapshot" ||
        name == "Immediate drawing outside draw callback does not render" ||
        name == "HUD pattern reads state in draw after action" ||
        name == "Representation and immediate draw share same object depth" ||
        name == "Circle outline scales with output scale" ||
        name == "One-point geometry builds and draws as logical point" ||
        name == "Rotated ellipse fill and outline respect angle" ||
        name == "Multiline text centers each line" ||
        name == "Text cases do not depend on RuntimeObject size" ||
        name == "Animation renderer observable pixels" ||
        name == "Animation audited source compiled acceptance" ||
        name == "Animation JavaScript draw snapshots" ||
        name == "Animation microexamples open and compiled pixels";
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "Animation geometry CPU and numeric recovery", testAnimationGeometryCPU },
        { "Animation renderer observable pixels", testAnimationRendering },
        { "Animation audited source compiled acceptance", testAnimationAcceptanceOpenCompiled },
        { "Animation JavaScript draw snapshots", testAnimationJavaScriptSnapshots },
        { "Animation microexamples open and compiled pixels", testAnimationMicroexamplesGraphics },
        { "Bitmap microexamples source and compiled rendering", testBitmapMicroexamplesSourceAndCompiledRendering },
        { "Bitmap texture transforms transparency and order", testBitmapTextureTransformTransparencyAndOrder },
        { "Drawing context rejects calls outside draw turn", testDrawingContextRejectsCallsOutsideDrawTurn },
        { "Text bindings preserve logical sizes and coordinates", testTextBindingsPreserveLogicalSizesAndCoordinates },
        { "World and local primitives capture owner reference and transform", testWorldAndLocalPrimitivesCaptureOwnerReferenceAndTransform },
        { "Local primitive captures transform at emission", testLocalPrimitiveCapturesTransformAtEmission },
        { "Renderer scales logical pixel to physical pixels", testRendererScalesLogicalPixelToPhysicalPixels },
        { "Renderer stretches world extent to machine raster", testRendererStretchesWorldExtentToMachineRaster },
        { "Local primitive wraps with reference but world primitive does not", testLocalPrimitiveWrapsWithReferenceButWorldPrimitiveDoesNot },
        { "Representation presentation snapshot does not change after draw mutation", testRepresentationPresentationSnapshotDoesNotChangeAfterDrawMutation },
        { "Local primitive presentation snapshot does not change after reference mutation", testLocalPrimitivePresentationSnapshotDoesNotChangeAfterReferenceMutation },
        { "Representation and local primitive wrap use same snapshot", testRepresentationAndLocalPrimitiveWrapUseSameSnapshot },
        { "Depth controls stable object visual turn order", testDepthControlsStableObjectVisualTurnOrder },
        { "Depth change is live but affects next draw pass order", testDepthChangeIsLiveButAffectsNextDrawPassOrder },
        { "Earlier visual turn can hide later object", testEarlierVisualTurnCanHideLaterObject },
        { "Earlier visual turn can kill later object", testEarlierVisualTurnCanKillLaterObject },
        { "Immediate drawing outside draw callback does not render", testImmediateDrawingOutsideDrawCallbackDoesNotRender },
        { "HUD pattern reads state in draw after action", testHudPatternReadsStateInDrawAfterAction },
        { "Representation and immediate draw share same object depth", testRepresentationAndImmediateDrawShareSameObjectDepth },
        { "JSON depth is visual-level and root depth is rejected", testJsonDepthIsVisualLevelAndRootDepthIsRejected },
        { "Depth property is readonly from JavaScript", testDepthPropertyIsReadonlyFromJavaScript },
        { "Circle outline scales with output scale", testCircleOutlineScalesWithOutputScale },
        { "One-point geometry builds and draws as logical point", testOnePointGeometryBuildsAndDrawsAsLogicalPoint },
        { "Rotated ellipse fill and outline respect angle", testRotatedEllipseFillAndOutlineRespectAngle },
        { "Multiline text centers each line", testMultilineTextCentersEachLine },
        { "Text cases do not depend on RuntimeObject size", testTextCasesDoNotDependOnRuntimeObjectSize }
    };

    for (const auto& test : tests)
    {
        if (
            isHeadlessTestEnvironment() &&
            requiresGraphics(test.first)
            )
        {
            std::cout
                << "[SKIP] "
                << test.first
                << " (headless environment)"
                << std::endl;

            continue;
        }

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
