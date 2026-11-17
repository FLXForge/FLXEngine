// Private JsonLoader helpers. Included inside its anonymous namespace.
using VisualPalette = std::unordered_map<std::string, Color>;

[[noreturn]] void visualError(JsonLoadSession& session, DiagnosticCode code,
    const std::filesystem::path& source, const std::string& field,
    const std::string& message)
{
    if (session.diagnostics)
        session.diagnostics->error(code, message, genericPathString(source), field);
    throw std::runtime_error(field + ": " + message);
}

void normalizeVisualPalette(JsonLoadSession& session, Json& visual,
    const std::filesystem::path& source)
{
    if (!visual.is_object() || !visual.contains("palette")) return;
    const Json declaration = visual["palette"];
    Json effective = Json::object();
    auto merge = [&](const Json& entries, const std::filesystem::path& entrySource)
    {
        if (!entries.is_object())
            visualError(session, DiagnosticCode::InvalidVisualPalette, entrySource,
                "visual.palette", "Expected a palette object with native color values");
        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            if (!it.value().is_string() ||
                !ColorParser::tryParse(it.value().get<std::string>()))
            {
                visualError(session, DiagnosticCode::InvalidVisualPalette, entrySource,
                    "visual.palette." + it.key(), "Invalid palette value " + it.value().dump() +
                    "; expected a native FLX color or #RGB/#RRGGBB. Palette values cannot reference aliases");
            }
            effective[it.key()] = it.value();
        }
    };
    if (declaration.is_object()) merge(declaration, source);
    else if (declaration.is_array())
    {
        for (const auto& entry : declaration)
        {
            if (entry.is_string())
            {
                auto reference = resolveJsonReference(session, entry.get<std::string>(),
                    source, "visual.palette", true);
                if (!reference.ok)
                    visualError(session, DiagnosticCode::InvalidVisualPalette, source,
                        "visual.palette", "External palette reference could not be resolved: " + entry.dump());
                merge(reference.data, reference.sourceFile);
            }
            else merge(entry, source);
        }
    }
    else visualError(session, DiagnosticCode::InvalidVisualPalette, source,
        "visual.palette", "Expected a palette object or ordered composition array");
    visual["palette"] = std::move(effective);
}

Color visualColor(JsonLoadSession& session, const std::string& token,
    const VisualPalette& palette, const std::filesystem::path& source,
    const std::string& field, bool compact = false)
{
    const auto found = palette.find(token);
    if (found != palette.end()) return found->second;
    if (!compact)
    {
        const auto native = ColorParser::tryParse(token);
        if (native) return *native;
    }
    visualError(session, DiagnosticCode::UnknownVisualColor, source, field,
        compact ? "Unknown compact pixel palette key '" + token + "'" :
        "Unknown visual color '" + token + "': not a palette key, native color or valid hexadecimal");
}

std::shared_ptr<const BitmapDefinition> parseBitmap(JsonLoadSession& session,
    const Json& node, const VisualPalette& palette,
    const std::filesystem::path& source, const std::string& field)
{
    auto fail = [&](const std::string& message) -> void
    {
        visualError(session, DiagnosticCode::InvalidBitmapDeclaration, source, field, message);
    };
    auto only = [&](const Json& object, const std::vector<std::string>& allowed)
    {
        for (auto it = object.begin(); it != object.end(); ++it)
            if (std::find(allowed.begin(), allowed.end(), it.key()) == allowed.end())
                fail("Unexpected bitmap property '" + it.key() + "'");
    };
    auto repetition = [&](const Json& object)
    {
        if (!object.contains("repeat")) return 1;
        const auto& value = object["repeat"];
        // Check unsigned values before converting, including UINT64_MAX.
        if (!value.is_number_integer() ||
            (value.is_number_unsigned() && value.get<uint64_t>() > 255) ||
            value.get<int64_t>() < 1 || value.get<int64_t>() > 255)
            fail("repeat must be a positive integer; expansion cannot exceed 255 pixels per dimension");
        return value.get<int>();
    };
    struct PixelToken { std::string value; bool compact; };
    using Row = std::vector<PixelToken>;
    auto pattern = [&](const Json& value)
    {
        Row row;
        if (value.is_string())
        {
            // JSON parsing validates UTF-8. Compact cells are Unicode scalar values,
            // not UTF-8 bytes; no grapheme normalization or recursive aliases.
            const std::string text = value.get<std::string>();
            for (size_t i = 0; i < text.size();)
            {
                const auto c = static_cast<unsigned char>(text[i]);
                const size_t length = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
                row.push_back({text.substr(i, length), true});
                if (row.size() > 255) fail("Bitmap width exceeds 255 pixels");
                i += length;
            }
        }
        else if (value.is_array())
        {
            if (value.size() > 255) fail("Bitmap width exceeds 255 pixels");
            for (const auto& token : value)
            {
                if (!token.is_string()) fail("Explicit pixels must be strings; nested patterns are not allowed");
                row.push_back({token.get<std::string>(), false});
            }
        }
        else fail("Expected a compact string or explicit string array");
        if (row.empty()) fail("Bitmap rows and patterns must not be empty");
        return row;
    };
    if (!node.is_object() || !node.contains("pixels") || !node["pixels"].is_array())
        fail("bitmap requires a pixels array");
    only(node, {"pixels"});
    std::vector<Row> rows;
    for (const auto& declaration : node["pixels"])
    {
        const Json* rowNode = &declaration;
        int vertical = 1;
        if (declaration.is_object())
        {
            only(declaration, {"row", "repeat"});
            if (!declaration.contains("row")) fail("Repeated row requires row");
            vertical = repetition(declaration);
            rowNode = &declaration["row"];
        }
        Row row;
        if (rowNode->is_array())
        {
            for (const auto& entry : *rowNode)
            {
                if (entry.is_object())
                {
                    only(entry, {"pattern", "repeat"});
                    if (!entry.contains("pattern")) fail("Horizontal repetition requires pattern");
                    const Row segment = pattern(entry["pattern"]);
                    const int count = repetition(entry);
                    if (segment.size() * static_cast<size_t>(count) > 255 - row.size())
                        fail("Bitmap width exceeds 255 pixels after expansion");
                    for (int i = 0; i < count; ++i) row.insert(row.end(), segment.begin(), segment.end());
                }
                else
                {
                    if (!entry.is_string()) fail("Explicit pixels must be strings or horizontal pattern objects");
                    if (row.size() == 255) fail("Bitmap width exceeds 255 pixels");
                    row.push_back({entry.get<std::string>(), false});
                }
            }
        }
        else row = pattern(*rowNode);
        if (row.empty()) fail("Bitmap rows must not be empty");
        if (static_cast<size_t>(vertical) > 255 - rows.size())
            fail("Bitmap height exceeds 255 pixels after expansion");
        for (int i = 0; i < vertical; ++i) rows.push_back(row);
    }
    if (rows.empty()) fail("Bitmap height must be at least 1 pixel");
    const size_t width = rows.front().size();
    for (const auto& row : rows)
        if (row.size() != width) fail("Bitmap matrix must be rectangular after expansion");
    auto bitmap = std::make_shared<BitmapDefinition>();
    bitmap->width = static_cast<int>(width);
    bitmap->height = static_cast<int>(rows.size());
    bitmap->pixels.reserve(width * rows.size());
    for (size_t y = 0; y < rows.size(); ++y)
        for (size_t x = 0; x < width; ++x)
            bitmap->pixels.push_back(visualColor(session, rows[y][x].value, palette, source,
                field + ".pixels[" + std::to_string(y) + "][" + std::to_string(x) + "]", rows[y][x].compact));
    return bitmap;
}
