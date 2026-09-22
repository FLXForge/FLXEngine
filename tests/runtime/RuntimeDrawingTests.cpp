#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/compiler/ProjectCompiler.h"
#include "../../engine/graphics/DrawingContext.h"
#include "../../engine/graphics/DrawingRenderer.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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
            "orderProbe",
            "function draw(o) {"
            "  if (read_global('drawOrder') == null) write_global('drawOrder', 0);"
            "  write_local(o, 'order', read_global('drawOrder'));"
            "  write_global('drawOrder', read_global('drawOrder') + 1);"
            "}"
        );

        ObjectDefinition root = objectDefinition("root");
        root.childResources["sameA"] = "sameA";
        root.childResources["low"] = "low";
        root.childResources["sameB"] = "sameB";
        root.childResources["high"] = "high";

        ObjectDefinition sameA = objectDefinition("sameA", "orderProbe");
        ObjectDefinition low = objectDefinition("low", "orderProbe");
        setVisualDepth(low, -10);
        ObjectDefinition sameB = objectDefinition("sameB", "orderProbe");
        ObjectDefinition high = objectDefinition("high", "orderProbe");
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
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "Drawing context rejects calls outside draw turn", testDrawingContextRejectsCallsOutsideDrawTurn },
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
