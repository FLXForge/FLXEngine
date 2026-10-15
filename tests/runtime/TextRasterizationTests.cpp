#include "../support/TestSupport.h"
#include "../../engine/graphics/DrawingRenderer.h"
#include "../../engine/graphics/DrawingContext.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/runtime/RuntimeObject.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace flx::test;
namespace
{
    const std::string sample = "MWil1 0123456789";
    bool equal(Color a, Color b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }
    bool equal(Image a, Image b)
    {
        if (a.width != b.width || a.height != b.height) return false;
        for (int y = 0; y < a.height; ++y)
        for (int x = 0; x < a.width; ++x)
            if (!equal(GetImageColor(a, x, y), GetImageColor(b, x, y))) return false;
        return true;
    }
    struct Stats { int ink = 0, gray = 0; };
    Stats stats(Image image)
    {
        Stats result;
        for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
        {
            const Color pixel = GetImageColor(image, x, y);
            if (pixel.r > 0) ++result.ink;
            if (pixel.r > 0 && pixel.r < 255) ++result.gray;
        }
        return result;
    }
    int unsampledInkRows(Font font, int size)
    {
        // Integer top-left, independent of FLX centering: a destination pixel
        // samples exactly one native texel with POINT. Count ink-bearing native
        // rows which no destination pixel can sample at this scale.
        const float scale = static_cast<float>(size) / font.baseSize;
        int lost = 0;
        for (char codepoint : sample)
        {
            if (codepoint == ' ') continue;
            const Image glyph = font.glyphs[GetGlyphIndex(font, codepoint)].image;
            std::vector<bool> sampled(glyph.height, false);
            for (int y = 0; y < size; ++y)
            {
                const int sourceY = static_cast<int>((y + 0.5f) / scale);
                if (sourceY < glyph.height) sampled[sourceY] = true;
            }
            for (int y = 0; y < glyph.height; ++y)
            {
                if (sampled[y]) continue;
                for (int x = 0; x < glyph.width; ++x)
                {
                    if (GetImageColor(glyph, x, y).a != 0) { ++lost; break; }
                }
            }
        }
        return lost;
    }
    std::vector<VisualPrimitive> primitives(Vector2 center, int size, const std::string& value,
        float angle, bool declarative)
    {
        RuntimeObject object("text", center, Vector2{ 2, 2 }, WHITE);
        object.runtimeId = "text#1";
        object.angle = angle;
        if (declarative)
        {
            RepresentationElementDefinition element;
            element.kind = RepresentationElementKind::Text;
            element.text = value; element.fontSize = size;
            object.representation = { element };
            return RepresentationPrimitiveBuilder::build(object, WHITE);
        }
        std::vector<VisualPrimitive> result;
        DrawingContext context;
        context.begin(object, result);
        require(context.emitLocalText(object, { 0, 0 }, value, size, WHITE), "local text emission");
        context.end();
        return result;
    }
    // Independent reference for the pre-fix centering, plus the metric-centered candidate.
    void reference(Vector2 center, int size, const std::string& value, float angle, bool metricCenter)
    {
        const Font font = GetFontDefault();
        const float lineHeight = static_cast<float>(size) + 1.0f;
        std::vector<std::string> lines;
        size_t start = 0;
        while (start <= value.size())
        {
            const size_t end = value.find('\n', start);
            lines.push_back(value.substr(start, end == std::string::npos ? end : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
        const float radians = angle * DEG2RAD;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (lines[i].empty()) continue;
            const Vector2 measured = MeasureTextEx(font, lines[i].c_str(), static_cast<float>(size), 1.0f);
            const float offset = (static_cast<float>(i) - (static_cast<float>(lines.size()) - 1.0f) / 2.0f) * lineHeight;
            const Vector2 position{ center.x - std::sin(radians) * offset, center.y + std::cos(radians) * offset };
            DrawTextPro(font, lines[i].c_str(), position,
                { measured.x / 2.0f, (metricCenter ? measured.y : lineHeight) / 2.0f },
                angle, static_cast<float>(size), 1.0f, WHITE);
        }
    }
    Image raster(int width, int height, Vector2 center, int size, const std::string& value,
        float angle = 0, int mode = 0)
    {
        RenderTexture2D target = LoadRenderTexture(width, height);
        require(target.id != 0, "logical target creation");
        BeginTextureMode(target);
        ClearBackground(BLACK);
        if (mode >= 2) reference(center, size, value, angle, mode == 3);
        else DrawingRenderer::render(primitives(center, size, value, angle, mode == 1), {}, 1,
            static_cast<float>(width), static_cast<float>(height));
        EndTextureMode();
        Image result = LoadImageFromTexture(target.texture);
        UnloadRenderTexture(target);
        return result;
    }
    Image present(Image logical, int scale, bool smoothing)
    {
        Texture2D texture = LoadTextureFromImage(logical);
        SetTextureFilter(texture, smoothing ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
        RenderTexture2D target = LoadRenderTexture(logical.width * scale, logical.height * scale);
        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawTexturePro(texture, { 0, 0, static_cast<float>(logical.width), static_cast<float>(logical.height) },
            { 0, 0, static_cast<float>(target.texture.width), static_cast<float>(target.texture.height) }, { 0, 0 }, 0, WHITE);
        EndTextureMode();
        Image result = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&result);
        UnloadRenderTexture(target); UnloadTexture(texture);
        return result;
    }
}
int main()
{
    if (isHeadlessTestEnvironment())
    {
        std::cout << "[SKIP] text rasterization requires OpenGL (FLX_TEST_HEADLESS=1)\n";
        return 77;
    }
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1, 1, "FLX text rasterization tests");
    if (!IsWindowReady()) return 77;
    try
    {
        const Font font = GetFontDefault();
        require(font.baseSize == 10, "raylib 6.0 default glyph base size is 10");
        Image atlas = LoadImageFromTexture(font.texture);
        for (int y = 0; y < atlas.height; ++y)
        for (int x = 0; x < atlas.width; ++x)
        {
            const unsigned char alpha = GetImageColor(atlas, x, y).a;
            require(alpha == 0 || alpha == 255, "native bitmap has binary coverage");
        }
        UnloadImage(atlas);
        Image implicit = raster(240, 80, { 120, 40 }, 8, sample);
        SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
        Image explicitPoint = raster(240, 80, { 120, 40 }, 8, sample);
        require(equal(implicit, explicitPoint), "default font filter already equals explicit POINT");
        require(stats(implicit).gray == 0, "logical size-8 raster has sharp binary edges");
        UnloadImage(implicit); UnloadImage(explicitPoint);

        const char* directory = std::getenv("FLX_TEXT_ARTIFACT_DIR");
        Image sheet{};
        if (directory)
        {
            std::filesystem::create_directories(directory);
            sheet = GenImageColor(480, 400, BLACK);
        }
        require(unsampledInkRows(font, 6) > 0 && unsampledInkRows(font, 8) > 0,
            "native ink rows are lost below base size even with integer top-left");
        require(unsampledInkRows(font, 10) == 0, "native scale samples every source row");
        int row = 0, cases = 0;
        for (int size : { 6, 8, 10, 11, 12 })
        {
            const Vector2 metrics = MeasureTextEx(font, sample.c_str(), static_cast<float>(size), 1);
            Image old = raster(240, 80, { 120, 40 }, size, sample, 0, 2);
            Image candidate = raster(240, 80, { 120, 40 }, size, sample, 0, 3);
            std::cout << "size=" << size << " width=" << metrics.x << " height=" << metrics.y
                      << " legacy-top=" << 40.0f - (size + 1) / 2.0f
                      << " metric-top=" << 40.0f - size / 2.0f
                      << " unsampled-native-ink-rows=" << unsampledInkRows(font, size)
                      << " legacy-ink=" << stats(old).ink << " metric-ink=" << stats(candidate).ink << '\n';
            if (directory)
            {
                ImageFlipVertical(&old);
                ImageFlipVertical(&candidate);
                ImageDraw(&sheet, old, { 0, 0, 240, 80 }, { 0, static_cast<float>(row * 80), 240, 80 }, WHITE);
                ImageDraw(&sheet, candidate, { 0, 0, 240, 80 }, { 240, static_cast<float>(row * 80), 240, 80 }, WHITE);
                const std::string label = "size " + std::to_string(size);
                ImageDrawText(&sheet, (label + " / current").c_str(), 4, row * 80 + 4, 10, YELLOW);
                ImageDrawText(&sheet, (label + " / metric origin").c_str(), 244, row * 80 + 4, 10, YELLOW);
            }
            UnloadImage(old); UnloadImage(candidate); ++row;
            for (int width : { 240, 320 })
            for (float fraction : { 0.0f, 0.25f, 0.5f, 0.75f })
            for (const std::string value : { sample, std::string("MWil1\n0123456789"), std::string("M\n\nW") })
            for (float angle : { 0.0f, 25.0f, 90.0f })
            {
                const Vector2 center{ width / 2.0f + fraction, 40 + fraction };
                Image immediate = raster(width, 80, center, size, value, angle, 0);
                Image declarative = raster(width, 80, center, size, value, angle, 1);
                Image legacy = raster(width, 80, center, size, value, angle, 2);
                require(equal(immediate, declarative), "declarative and local immediate text have identical pixels");
                require(equal(immediate, legacy), "characterize existing origin independently");
                require(stats(immediate).gray == 0, "POINT is binary even at fractional positions and rotation");
                UnloadImage(declarative); UnloadImage(legacy);
                if (angle == 0 && value == sample)
                {
                    for (int scale : { 1, 2, 3 })
                    {
                        Image point = present(immediate, scale, false);
                        for (int y = 0; y < point.height; ++y)
                        for (int x = 0; x < point.width; ++x)
                            require(equal(GetImageColor(point, x, y), GetImageColor(immediate, x / scale, y / scale)),
                                "POINT presentation exactly repeats logical pixels at integer scale");
                        Image linear = present(immediate, scale, true);
                        if (scale > 1) require(stats(linear).gray > 0, "smoothing creates gray edges only during presentation");
                        UnloadImage(point); UnloadImage(linear);
                    }
                }
                UnloadImage(immediate); ++cases;
            }
        }
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        Image linearFont = raster(240, 80, { 120.25f, 40.25f }, 8, sample);
        require(stats(linearFont).gray > 0, "font BILINEAR produces logical blur, unlike its default filter");
        UnloadImage(linearFont);
        SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
        if (directory)
        {
            const std::string path = (std::filesystem::path(directory) / "centering-before-candidate.png").string();
            ImageResizeNN(&sheet, sheet.width * 3, sheet.height * 3);
            require(ExportImage(sheet, path.c_str()), "export comparative raster");
            UnloadImage(sheet);
        }
        std::cout << "[PASS] " << cases << " logical text cases, font filter and output scale characterization\n";
        CloseWindow(); return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n'; CloseWindow(); return 1;
    }
}
