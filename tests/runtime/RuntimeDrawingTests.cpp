#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/compiler/ProjectCompiler.h"
#include "../../engine/graphics/DrawingContext.h"
#include "../../engine/graphics/DrawingRenderer.h"
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
        definition.shapeType = "none";

        if (!script.empty())
        {
            definition.resolvedScriptPaths.push_back(script);
        }

        return definition;
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

        DrawingContext context;
        std::vector<VisualPrimitive> primitives;
        context.begin(owner, primitives);
        require(context.emitLocalPixel(reference, Vector2{ 0.0f, 0.0f }, RED), "first local pixel should emit");

        reference.position = Vector2{ 15.0f, 5.0f };
        require(context.emitLocalPixel(reference, Vector2{ 0.0f, 0.0f }, BLUE), "second local pixel should emit");
        context.end();

        require(nearlyEqual(primitives[0].a.x, 5.0f), "first primitive should keep first reference position");
        require(nearlyEqual(primitives[1].a.x, 15.0f), "second primitive should use later reference position");
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
        root.boundsMode = "wrap";
        root.boundsOverflow = true;

        harness.addObject(root);
        require(harness.load().success, "runtime should load wrap drawing project");

        require(countRenderedColor(harness, RED, 1, 80, 60) == 1, "world primitive should not create wrap presentation copies");
        require(countRenderedColor(harness, BLUE, 1, 80, 60) == 2, "local primitive should share reference wrap presentation");
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
        low.depth = -10;
        ObjectDefinition sameB = objectDefinition("sameB", "orderProbe");
        ObjectDefinition high = objectDefinition("high", "orderProbe");
        high.depth = 10;

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
        first.depth = 0;
        ObjectDefinition second = objectDefinition("second", "stableProbe");
        second.depth = 5;

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

    void testEarlierVisualTurnCanHideOrKillLaterObject()
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
        hider.depth = -1;
        ObjectDefinition target = objectDefinition("target", "targetDraw");
        target.depth = 1;

        harness.addObject(root);
        harness.addObject(hider);
        harness.addObject(target);

        require(harness.load().success, "runtime should load visibility drawing project");
        harness.draw();

        require(localValue(requireObject(harness.world, "target"), "drawn") == 0.0, "hidden later object should not receive visual turn");
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
        harness.world.update(harness.scripts, ScreenWidth, ScreenHeight, 0.016f);

        require(countRenderedColor(harness, RED, 1, 80, 60) == 0, "draw_pixel outside draw callback should not render");
    }

    void testShapeAndImmediateDrawShareSameObjectDepth()
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
        lowPainter.depth = -5;

        ObjectDefinition highBlock = objectDefinition("highBlock");
        highBlock.depth = 5;
        highBlock.shapeType = "block";
        highBlock.origin = Vector2{ 5.0f, 5.0f };
        highBlock.hasOrigin = true;
        highBlock.size = Vector2{ 3.0f, 3.0f };
        highBlock.color = BLUE;

        harness.addObject(root);
        harness.addObject(lowPainter);
        harness.addObject(highBlock);

        require(harness.load().success, "runtime should load shared depth project");
        require(countRenderedColor(harness, BLUE, 1, 80, 60) == 9, "higher depth shape should cover lower depth immediate primitive");
        require(countRenderedColor(harness, RED, 1, 80, 60) == 0, "immediate drawing should not run as a final overlay pipeline");
    }

    void testJsonDepthIsRootLevelAndShapeLayerIsRejected()
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
            "  \"depth\": 7,\n"
            "  \"shape\": {\n"
            "    \"type\": \"block\",\n"
            "    \"color\": \"white\",\n"
            "    \"size\": { \"width\": 2, \"height\": 2 }\n"
            "  }\n"
            "}\n"
        );

        CompilationResult valid = flx::test::compile(root / "game.flx");
        require(valid.success, "root depth should compile");
        const ObjectDefinition* compiledRoot = valid.project.resources.findObject(valid.project.rootId);
        require(compiledRoot != nullptr, "compiled root should exist");
        require(compiledRoot->depth == 7, "root depth should reach compiled definition");

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"shape\": {\n"
            "    \"type\": \"block\",\n"
            "    \"layer\": 7,\n"
            "    \"color\": \"white\",\n"
            "    \"size\": { \"width\": 2, \"height\": 2 }\n"
            "  }\n"
            "}\n"
        );

        CompilationResult invalid = flx::test::compile(root / "game.flx");
        require(!invalid.success, "shape.layer should be rejected");
        require(diagnosticsContain(invalid.diagnostics, "shape.layer"), "shape.layer rejection should be clear");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "Drawing context rejects calls outside draw turn", testDrawingContextRejectsCallsOutsideDrawTurn },
        { "World and local primitives capture owner reference and transform", testWorldAndLocalPrimitivesCaptureOwnerReferenceAndTransform },
        { "Local primitive captures transform at emission", testLocalPrimitiveCapturesTransformAtEmission },
        { "Renderer scales logical pixel to physical pixels", testRendererScalesLogicalPixelToPhysicalPixels },
        { "Local primitive wraps with reference but world primitive does not", testLocalPrimitiveWrapsWithReferenceButWorldPrimitiveDoesNot },
        { "Depth controls stable object visual turn order", testDepthControlsStableObjectVisualTurnOrder },
        { "Depth change is live but affects next draw pass order", testDepthChangeIsLiveButAffectsNextDrawPassOrder },
        { "Earlier visual turn can hide or kill later object", testEarlierVisualTurnCanHideOrKillLaterObject },
        { "Immediate drawing outside draw callback does not render", testImmediateDrawingOutsideDrawCallbackDoesNotRender },
        { "Shape and immediate draw share same object depth", testShapeAndImmediateDrawShareSameObjectDepth },
        { "JSON depth is root-level and shape layer is rejected", testJsonDepthIsRootLevelAndShapeLayerIsRejected }
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
