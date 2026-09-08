#include "JsonLoader.h"
#include "../audio/NoteTools.h"
#include "../debug/Logger.h"
#include "../tools/ColorParser.h"
#include "../tools/TextTools.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using Json = nlohmann::ordered_json;

namespace
{
    struct ResolvedJsonReference
    {
        bool ok = false;
        Json data;
        std::filesystem::path sourceFile;
        std::filesystem::path resolvedPath;
        std::string member;
    };

    struct JsonLoadSession
    {
        std::filesystem::path projectJsonRoot;
        std::unordered_map<std::string, Json> jsonCache;
        Diagnostics* diagnostics = nullptr;
        std::vector<std::string> likeStack;
        std::vector<std::string> definitionStack;
    };

    std::string ensureExtension(
        const std::string& path,
        const std::string& extension
    )
    {
        if (path.ends_with(extension))
        {
            return path;
        }

        return path + extension;
    }

    std::string genericPathString(
        const std::filesystem::path& path
    )
    {
        std::string result =
            path.lexically_normal().generic_string();

        std::replace(
            result.begin(),
            result.end(),
            '\\',
            '/'
        );

        return result;
    }

    std::string definitionStackKey(
        const std::filesystem::path& sourceFile,
        const std::string& id
    )
    {
        return
            std::filesystem::absolute(sourceFile).lexically_normal().generic_string() +
            "#" +
            id;
    }

    bool definitionIsActive(
        const JsonLoadSession& session,
        const std::filesystem::path& sourceFile,
        const std::string& id
    )
    {
        const std::string key =
            definitionStackKey(
                sourceFile,
                id
            );

        return std::find(
            session.definitionStack.begin(),
            session.definitionStack.end(),
            key
        ) != session.definitionStack.end();
    }

    struct ScopedDefinitionStackEntry
    {
        JsonLoadSession& session;

        ScopedDefinitionStackEntry(
            JsonLoadSession& loadSession,
            const std::filesystem::path& sourceFile,
            const std::string& id
        )
            : session(loadSession)
        {
            session.definitionStack.push_back(
                definitionStackKey(
                    sourceFile,
                    id
                )
            );
        }

        ~ScopedDefinitionStackEntry()
        {
            session.definitionStack.pop_back();
        }
    };

    std::filesystem::path normalizedRelativePath(
        const std::string& path,
        const std::string& extension
    )
    {
        std::string normalized =
            ensureExtension(path, extension);

        std::replace(
            normalized.begin(),
            normalized.end(),
            '\\',
            '/'
        );

        return std::filesystem::path(normalized).lexically_normal();
    }

    std::filesystem::path resolvePath(
        const std::filesystem::path& parentFile,
        const std::string& child
    )
    {
        return (
            parentFile.parent_path() /
            normalizedRelativePath(child, ".json")
        ).lexically_normal();
    }

    std::filesystem::path resolveReferencedPath(
        const JsonLoadSession& session,
        const std::filesystem::path& sourceFile,
        const std::string& reference,
        const std::string& extension
    )
    {
        if (!reference.empty() && reference.front() == '/')
        {
            std::string normalized =
                reference;

            while (!normalized.empty() && normalized.front() == '/')
            {
                normalized.erase(normalized.begin());
            }

            std::replace(
                normalized.begin(),
                normalized.end(),
                '\\',
                '/'
            );

            return (
                std::filesystem::absolute(session.projectJsonRoot) /
                normalizedRelativePath(normalized, extension)
            ).lexically_normal();
        }

        return (
            sourceFile.parent_path() /
            normalizedRelativePath(reference, extension)
        ).lexically_normal();
    }

    bool isFlxReference(
        const std::string& value
    )
    {
        return !value.empty() && value.front() == '/';
    }

    bool loadJson(
        const std::filesystem::path& path,
        Json& data
    )
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            Logger::error(
                "json",
                "The file could not be opened " + genericPathString(path)
            );

            return false;
        }

        try
        {
            file >> data;
        }
        catch (const Json::parse_error& error)
        {
            Logger::error(
                "json",
                "Invalid JSON in " +
                genericPathString(path) +
                ": " +
                error.what()
            );

            return false;
        }

        return true;
    }

    void addReferenceError(
        JsonLoadSession& session,
        DiagnosticCode code,
        const std::string& message,
        const std::filesystem::path& declaringFile,
        const std::string& field,
        const std::string& reference,
        const std::filesystem::path& resolvedPath,
        const std::string& member = ""
    )
    {
        std::string details =
            message +
            "\nReference: " +
            reference;

        if (!resolvedPath.empty())
        {
            details +=
                "\nResolved path: " +
                genericPathString(resolvedPath);
        }

        if (!member.empty())
        {
            details +=
                "\nMember: " +
                member;
        }

        if (session.diagnostics != nullptr)
        {
            session.diagnostics->error(
                code,
                details,
                genericPathString(declaringFile),
                field
            );
        }

        Logger::error(
            "json",
            details
        );
    }

    void addStateMachineError(
        JsonLoadSession& session,
        DiagnosticCode code,
        const std::string& message,
        const std::filesystem::path& declaringFile,
        const std::string& field
    )
    {
        if (session.diagnostics != nullptr)
        {
            session.diagnostics->error(
                code,
                message,
                genericPathString(declaringFile),
                field
            );
        }

        Logger::error(
            "json",
            message
        );
    }

    bool loadJsonCached(
        JsonLoadSession& session,
        const std::filesystem::path& path,
        Json& data
    )
    {
        const std::filesystem::path normalized =
            std::filesystem::absolute(path).lexically_normal();

        const std::string key =
            normalized.generic_string();

        const auto found =
            session.jsonCache.find(key);

        if (found != session.jsonCache.end())
        {
            data = found->second;
            return true;
        }

        if (!loadJson(normalized, data))
        {
            return false;
        }

        session.jsonCache[key] = data;

        return true;
    }

    bool pathStartsWith(
        const std::filesystem::path& child,
        const std::filesystem::path& root
    )
    {
        std::string childText =
            child.lexically_normal().generic_string();
        std::string rootText =
            root.lexically_normal().generic_string();

        std::transform(
            childText.begin(),
            childText.end(),
            childText.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
        );
        std::transform(
            rootText.begin(),
            rootText.end(),
            rootText.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
        );

        if (!rootText.ends_with("/"))
        {
            rootText += "/";
        }

        return childText.starts_with(rootText) ||
            childText == rootText.substr(0, rootText.size() - 1);
    }

    bool containsParentTraversal(
        const std::filesystem::path& path
    )
    {
        for (const auto& part : path)
        {
            if (part == "..")
            {
                return true;
            }
        }

        return false;
    }

    std::filesystem::path resolveFlxReferencePath(
        JsonLoadSession& session,
        const std::string& path
    )
    {
        if (session.projectJsonRoot.empty())
        {
            Logger::error(
                "json",
                "FLX project root is not configured"
            );

            return {};
        }

        if (path.empty() || path == "/")
        {
            Logger::error(
                "json",
                "FLX reference is empty after '/'"
            );

            return {};
        }

        std::string normalized =
            path;

        while (!normalized.empty() && normalized.front() == '/')
        {
            normalized.erase(normalized.begin());
        }

        std::replace(
            normalized.begin(),
            normalized.end(),
            '\\',
            '/'
        );

        std::filesystem::path relative =
            ensureExtension(normalized, ".json");

        if (containsParentTraversal(relative))
        {
            Logger::error(
                "json",
                "FLX reference cannot escape project root: " + path
            );

            return {};
        }

        const std::filesystem::path root =
            std::filesystem::absolute(session.projectJsonRoot).lexically_normal();
        const std::filesystem::path resolved =
            std::filesystem::absolute(root / relative).lexically_normal();

        if (!pathStartsWith(resolved, root))
        {
            Logger::error(
                "json",
                "FLX reference resolved outside project root: " + path
            );

            return {};
        }

        return resolved;
    }

    void mergeJson(
        Json& base,
        const Json& override
    )
    {
        for (auto it = override.begin(); it != override.end(); ++it)
        {
            const std::string key = it.key();

            if (
                base.contains(key) &&
                base[key].is_object() &&
                it.value().is_object()
                )
            {
                mergeJson(base[key], it.value());
            }
            else
            {
                base[key] = it.value();
            }
        }
    }

    bool resolveLike(
        JsonLoadSession& session,
        const std::filesystem::path& currentFile,
        const Json& object,
        Json& resolved,
        const std::string& field
    );

    ResolvedJsonReference resolveJsonReference(
        JsonLoadSession& session,
        const std::string& reference,
        const std::filesystem::path& declaringFile,
        const std::string& field
    );

    bool resolveBlockReference(
        JsonLoadSession& session,
        const std::filesystem::path& currentFile,
        Json& object,
        const std::string& key
    )
    {
        if (!object.contains(key) || !object[key].is_string())
        {
            return true;
        }

        const std::string reference =
            object[key].get<std::string>();

        if (isFlxReference(reference))
        {
            ResolvedJsonReference resolvedReference =
                resolveJsonReference(
                    session,
                    reference,
                    currentFile,
                    key
                );

            if (!resolvedReference.ok)
            {
                return false;
            }

            if (
                resolvedReference.data.is_object() &&
                resolvedReference.data.contains(key)
            )
            {
                object[key] =
                    resolvedReference.data[key];
            }
            else
            {
                object[key] =
                    resolvedReference.data;
            }

            return true;
        }

        const auto blockPath =
            resolvePath(
                currentFile,
                reference
            );

        Json block;

        if (!loadJson(blockPath, block))
        {
            return false;
        }

        Json resolvedBlock;

        if (!resolveLike(session, blockPath, block, resolvedBlock, key))
        {
            return false;
        }

        if (resolvedBlock.contains(key))
        {
            object[key] = resolvedBlock[key];
        }
        else
        {
            object[key] = resolvedBlock;
        }

        return true;
    }

    bool resolveBlockReferences(
        JsonLoadSession& session,
        const std::filesystem::path& currentFile,
        Json& object
    )
    {
        static const std::vector<std::string> blockKeys = {
            "shape",
            "mechanics",
            "inherit",
            "bounds",
            "collisions",
            "behavior",
            "creation",
            "states"
        };

        for (const auto& key : blockKeys)
        {
            if (!resolveBlockReference(session, currentFile, object, key))
            {
                return false;
            }
        }

        return true;
    }

    bool resolveLike(
        JsonLoadSession& session,
        const std::filesystem::path& currentFile,
        const Json& object,
        Json& resolved,
        const std::string& field
    )
    {
        if (!object.is_object())
        {
            return false;
        }

        if (!object.contains("like"))
        {
            resolved = object;
            resolved["__sourceFile"] = genericPathString(currentFile);

            if (object.contains("behavior"))
            {
                resolved["__behaviorSourceFile"] =
                    genericPathString(currentFile);
            }

            if (object.contains("children") && object["children"].is_object())
            {
                Json childSourceFiles =
                    Json::object();

                for (auto it = object["children"].begin(); it != object["children"].end(); ++it)
                {
                    childSourceFiles[it.key()] =
                        genericPathString(currentFile);
                }

                resolved["__childSourceFiles"] =
                    childSourceFiles;
            }

            return resolveBlockReferences(session, currentFile, resolved);
        }

        const std::string likeReference =
            object["like"].get<std::string>();

        if (isFlxReference(likeReference))
        {
            ResolvedJsonReference resolvedReference =
                resolveJsonReference(
                    session,
                    likeReference,
                    currentFile,
                    field.empty() ? "like" : field + ".like"
                );

            if (!resolvedReference.ok || !resolvedReference.data.is_object())
            {
                Logger::error(
                    "json",
                    "Like target could not be resolved: " + likeReference
                );

                return false;
            }

            Json override =
                object;

            override.erase("like");

            if (override.contains("behavior"))
            {
                override["__behaviorSourceFile"] =
                    genericPathString(currentFile);
            }

            if (override.contains("children") && override["children"].is_object())
            {
                Json childSourceFiles =
                    Json::object();

                if (
                    resolvedReference.data.contains("__childSourceFiles") &&
                    resolvedReference.data["__childSourceFiles"].is_object()
                )
                {
                    childSourceFiles =
                        resolvedReference.data["__childSourceFiles"];
                }

                for (auto it = override["children"].begin(); it != override["children"].end(); ++it)
                {
                    childSourceFiles[it.key()] =
                        genericPathString(currentFile);
                }

                override["__childSourceFiles"] =
                    childSourceFiles;
            }

            mergeJson(resolvedReference.data, override);

            resolved =
                resolvedReference.data;
            resolved["__sourceFile"] =
                genericPathString(resolvedReference.sourceFile);

            return resolveBlockReferences(
                session,
                resolvedReference.sourceFile,
                resolved
            );
        }

        const auto basePath =
            resolvePath(
                currentFile,
                likeReference
            );

        const std::string baseKey =
            std::filesystem::absolute(basePath).lexically_normal().generic_string();

        if (
            std::find(
                session.likeStack.begin(),
                session.likeStack.end(),
                baseKey
            ) != session.likeStack.end()
        )
        {
            std::string chain;

            for (const std::string& entry : session.likeStack)
            {
                if (!chain.empty())
                {
                    chain += " -> ";
                }

                chain += entry;
            }

            if (!chain.empty())
            {
                chain += " -> ";
            }

            chain += baseKey;

            addReferenceError(
                session,
                DiagnosticCode::ResourceReferenceCycle,
                "Resource reference cycle detected.\nChain: " + chain,
                currentFile,
                field.empty() ? "like" : field + ".like",
                likeReference,
                basePath
            );

            return false;
        }

        Json base;

        if (!loadJson(basePath, base))
        {
            addReferenceError(
                session,
                DiagnosticCode::MissingReferencedResource,
                "Like target could not be loaded.",
                currentFile,
                field.empty() ? "like" : field + ".like",
                likeReference,
                basePath
            );

            return false;
        }

        Json resolvedBase;

        session.likeStack.push_back(baseKey);

        const bool baseResolved =
            resolveLike(session, basePath, base, resolvedBase, field);

        session.likeStack.pop_back();

        if (!baseResolved)
        {
            return false;
        }

        Json override =
            object;

        override.erase("like");

        if (override.contains("behavior"))
        {
            override["__behaviorSourceFile"] =
                genericPathString(currentFile);
        }

        if (override.contains("children") && override["children"].is_object())
        {
            Json childSourceFiles =
                Json::object();

            if (
                resolvedBase.contains("__childSourceFiles") &&
                resolvedBase["__childSourceFiles"].is_object()
            )
            {
                childSourceFiles =
                    resolvedBase["__childSourceFiles"];
            }

            for (auto it = override["children"].begin(); it != override["children"].end(); ++it)
            {
                childSourceFiles[it.key()] =
                    genericPathString(currentFile);
            }

            override["__childSourceFiles"] =
                childSourceFiles;
        }

        mergeJson(resolvedBase, override);

        resolved = resolvedBase;
        resolved["__sourceFile"] = genericPathString(basePath);

        return resolveBlockReferences(session, currentFile, resolved);
    }

    bool hasShape(const Json& object)
    {
        return object.contains("shape") &&
            object["shape"].is_object();
    }

    void rejectRootProperty(
        const Json& object,
        const std::string& property,
        const std::string& owner,
        const std::string& expectedBlock
    )
    {
        if (!object.contains(property))
        {
            return;
        }

        throw std::runtime_error(
            "Invalid FLX object '" + owner + "': property '" +
            property + "' must be declared inside '" +
            expectedBlock + "'"
        );
    }

    void validateObjectRootProperties(
        const Json& object,
        const std::string& owner
    )
    {
        rejectRootProperty(object, "size", owner, "shape");
        rejectRootProperty(object, "color", owner, "shape");
        rejectRootProperty(object, "layer", owner, "shape");
        rejectRootProperty(object, "speed", owner, "mechanics");
        rejectRootProperty(object, "angle", owner, "mechanics");
        if (object.contains("role"))
        {
            throw std::runtime_error(
                "Invalid FLX object '" + owner +
                "': property 'role' was removed in v0.3"
            );
        }

        if (object.contains("collision"))
        {
            throw std::runtime_error(
                "Invalid FLX object '" + owner +
                "': property 'collision' was replaced by 'collisions'"
            );
        }

        if (object.contains("motion"))
        {
            throw std::runtime_error(
                "Invalid FLX object '" + owner +
                "': property 'motion' was replaced by 'mechanics'"
            );
        }
    }

    Vector2 parseOrigin(const Json& object)
    {
        if (!object.contains("origin") || !object["origin"].is_object())
        {
            return Vector2{ 0.0f, 0.0f };
        }

        const auto& origin = object["origin"];

        return Vector2{
            origin.value("x", 0.0f),
            origin.value("y", 0.0f)
        };
    }

    Vector2 parseSize(const Json& object)
    {
        if (hasShape(object))
        {
            const auto& shape = object["shape"];

            if (shape.contains("size") && shape["size"].is_object())
            {
                const auto& size = shape["size"];

                return Vector2{
                    size.value("width", 0.0f),
                    size.value("height", 0.0f)
                };
            }
        }

        return Vector2{ 0.0f, 0.0f };
    }

    void parseShape(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!hasShape(object))
        {
            definition.hasVisual = false;
            return;
        }

        const auto& shape = object["shape"];

        definition.hasVisual = true;
        definition.shapeType =
            TextTools::toLower(shape.value("type", "block"));
        definition.shapeMode =
            TextTools::toLower(shape.value("mode", "fill"));
        definition.textContent =
            shape.value("content", definition.textContent);

        definition.layer =
            shape.value("layer", definition.layer);

        if (shape.contains("color"))
        {
            definition.color =
                ColorParser::parse(
                    shape["color"].get<std::string>(),
                    WHITE
                );
        }

        if (shape.contains("radius"))
        {
            definition.radius =
                shape["radius"].get<float>();
        }
        else
        {
            definition.radius =
                std::max(
                    definition.size.x,
                    definition.size.y
                ) / 2.0f;
        }

        definition.points.clear();

        if (shape.contains("points") && shape["points"].is_array())
        {
            for (const auto& point : shape["points"])
            {
                definition.points.push_back(
                    Vector2{
                        point.value("x", 0.0f),
                        point.value("y", 0.0f)
                    }
                );
            }
        }
    }

    MechanicsSpeedDefinition parseMechanicsSpeed(
        const Json& owner,
        const MechanicsSpeedDefinition& fallback
    )
    {
        MechanicsSpeedDefinition speed =
            fallback;

        if (owner.is_number())
        {
            speed.start =
                owner.get<float>();
            speed.limit =
                0.0f;
            return speed;
        }

        if (!owner.is_object())
        {
            return speed;
        }

        speed.start =
            owner.value("start", speed.start);

        speed.limit =
            owner.value("limit", speed.limit);

        if (speed.limit > 0.0f && speed.limit < speed.start)
        {
            Logger::warning(
                "json",
                "mechanics speed.limit cannot be lower than speed.start; using start as limit"
            );

            speed.limit =
                speed.start;
        }

        return speed;
    }

    MechanicsAxisDefinition parseMechanicsAxis(
        const Json& axis,
        const MechanicsMotionDefinition& motion
    )
    {
        MechanicsAxisDefinition result;
        result.speed =
            motion.speed;
        result.acceleration =
            motion.acceleration;
        result.inertia =
            motion.inertia;
        result.step =
            motion.step;

        if (!axis.is_object())
        {
            return result;
        }

        if (axis.contains("speed"))
        {
            result.speed =
                parseMechanicsSpeed(axis["speed"], result.speed);
            result.hasSpeed =
                true;
        }

        if (axis.contains("acceleration"))
        {
            result.acceleration =
                axis.value("acceleration", result.acceleration);
            result.hasAcceleration =
                true;
        }

        if (axis.contains("inertia"))
        {
            result.inertia =
                std::clamp(axis.value("inertia", result.inertia), 0.0f, 1.0f);
            result.hasInertia =
                true;
        }

        if (axis.contains("step"))
        {
            result.step =
                axis.value("step", result.step);
            result.hasStep =
                true;
        }

        return result;
    }

    void parseMechanics(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("mechanics") || !object["mechanics"].is_object())
        {
            return;
        }

        const auto& mechanics =
            object["mechanics"];

        const std::string type =
            TextTools::toLower(
                mechanics.value("type", std::string("direct"))
            );

        definition.mechanics.type =
            type == "polar"
                ? MechanicsType::Polar
                : MechanicsType::Direct;

        if (mechanics.contains("motion") && mechanics["motion"].is_object())
        {
            const auto& motion =
                mechanics["motion"];

            if (motion.contains("speed"))
            {
                definition.mechanics.motion.speed =
                    parseMechanicsSpeed(
                        motion["speed"],
                        definition.mechanics.motion.speed
                    );
            }

            definition.mechanics.motion.acceleration =
                motion.value(
                    "acceleration",
                    definition.mechanics.motion.acceleration
                );

            definition.mechanics.motion.inertia =
                std::clamp(
                    motion.value("inertia", definition.mechanics.motion.inertia),
                    0.0f,
                    1.0f
                );

            definition.mechanics.motion.step =
                motion.value("step", definition.mechanics.motion.step);

            const std::string diagonal =
                TextTools::toLower(
                    motion.value("diagonal", std::string("independent"))
                );

            definition.mechanics.motion.diagonal =
                diagonal == "vector"
                    ? MechanicsDiagonalMode::Vector
                    : MechanicsDiagonalMode::Independent;

            if (motion.contains("horizontal"))
            {
                definition.mechanics.motion.horizontal =
                    parseMechanicsAxis(
                        motion["horizontal"],
                        definition.mechanics.motion
                    );
            }
            else
            {
                definition.mechanics.motion.horizontal =
                    parseMechanicsAxis(Json::object(), definition.mechanics.motion);
            }

            if (motion.contains("vertical"))
            {
                definition.mechanics.motion.vertical =
                    parseMechanicsAxis(
                        motion["vertical"],
                        definition.mechanics.motion
                    );
            }
            else
            {
                definition.mechanics.motion.vertical =
                    parseMechanicsAxis(Json::object(), definition.mechanics.motion);
            }
        }

        if (mechanics.contains("rotation") && mechanics["rotation"].is_object())
        {
            const auto& rotation =
                mechanics["rotation"];

            definition.mechanics.rotation.angle =
                rotation.value("angle", definition.mechanics.rotation.angle);

            if (rotation.contains("speed"))
            {
                definition.mechanics.rotation.speed =
                    parseMechanicsSpeed(
                        rotation["speed"],
                        definition.mechanics.rotation.speed
                    );
            }

            definition.mechanics.rotation.acceleration =
                rotation.value(
                    "acceleration",
                    definition.mechanics.rotation.acceleration
                );

            definition.mechanics.rotation.inertia =
                std::clamp(
                    rotation.value("inertia", definition.mechanics.rotation.inertia),
                    0.0f,
                    1.0f
                );

            definition.mechanics.rotation.step =
                rotation.value("step", definition.mechanics.rotation.step);
        }
    }

    void parseInherit(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("inherit") || !object["inherit"].is_object())
        {
            return;
        }

        const auto& inherit =
            object["inherit"];

        if (inherit.contains("creation") && inherit["creation"].is_object())
        {
            const auto& creation =
                inherit["creation"];

            const std::string angle =
                TextTools::toLower(creation.value("angle", std::string("none")));

            definition.inherit.creationAngle =
                angle == "copy"
                    ? InheritCreationMode::Copy
                    : InheritCreationMode::None;

            const std::string velocity =
                TextTools::toLower(creation.value("velocity", std::string("none")));

            if (velocity == "copy")
            {
                definition.inherit.creationVelocity =
                    InheritCreationMode::Copy;
            }
            else if (velocity == "compose")
            {
                definition.inherit.creationVelocity =
                    InheritCreationMode::Compose;
            }
        }

        if (inherit.contains("live") && inherit["live"].is_object())
        {
            const auto& live =
                inherit["live"];

            const std::string angle =
                TextTools::toLower(live.value("angle", std::string("none")));

            definition.inherit.liveAngle =
                angle == "copy"
                    ? InheritLiveMode::Copy
                    : InheritLiveMode::None;
        }
    }

    void parseAttach(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("attach") || !object["attach"].is_object())
        {
            return;
        }

        const auto& attach =
            object["attach"];

        const bool position =
            attach.value("position", false);

        definition.attachFollowX =
            attach.contains("x") ?
            attach.value("x", false) :
            position;

        definition.attachFollowY =
            attach.contains("y") ?
            attach.value("y", false) :
            position;

        definition.attachFollowAngle =
            attach.value("angle", false);

        definition.attachOnCreate =
            attach.value("born", false);
    }

    void parseBounds(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("bounds") || !object["bounds"].is_object())
        {
            return;
        }

        const auto& bounds = object["bounds"];

        definition.boundsMode =
            TextTools::toLower(
                bounds.value("mode", definition.boundsMode)
            );

        definition.boundsOverflow =
            bounds.value("overflow", definition.boundsOverflow);
    }

    void parseBehavior(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("behavior") || !object["behavior"].is_object())
        {
            return;
        }

        const auto& behavior = object["behavior"];
        const std::string behaviorSourceFile =
            object.value("__behaviorSourceFile", definition.sourcePath);

        if (!behavior.contains("scripts") || !behavior["scripts"].is_array())
        {
            return;
        }

        for (const auto& script : behavior["scripts"])
        {
            definition.scripts.push_back(
                script.get<std::string>()
            );
            definition.scriptSourcePaths.push_back(
                behaviorSourceFile
            );
        }
    }

    void parseCreationPattern(
        const Json& pattern,
        ObjectDefinition& definition
    )
    {
        definition.gridPattern.clear();
        definition.gridRowPattern.clear();
        definition.gridPatternIsRows = false;

        if (!pattern.is_array())
        {
            Logger::error(
                "json",
                "Invalid grid creation pattern in '" + definition.id +
                "': expected array"
            );

            return;
        }

        if (pattern.empty())
        {
            return;
        }

        if (pattern.front().is_array())
        {
            definition.gridPatternIsRows = true;

            for (const auto& row : pattern)
            {
                if (!row.is_array())
                {
                    Logger::warning(
                        "json",
                        "Ignoring invalid grid pattern row in '" +
                        definition.id + "'"
                    );

                    continue;
                }

                std::vector<std::string> rowPattern;

                for (const auto& childId : row)
                {
                    if (!childId.is_string())
                    {
                        Logger::warning(
                            "json",
                            "Ignoring invalid grid pattern value in '" +
                            definition.id + "'"
                        );

                        continue;
                    }

                    rowPattern.push_back(
                        childId.get<std::string>()
                    );
                }

                definition.gridRowPattern.push_back(rowPattern);
            }

            return;
        }

        for (const auto& childId : pattern)
        {
            if (!childId.is_string())
            {
                Logger::warning(
                    "json",
                    "Ignoring invalid grid pattern value in '" +
                    definition.id + "'"
                );

                continue;
            }

            definition.gridPattern.push_back(
                childId.get<std::string>()
            );
        }
    }

    void parseCreation(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("creation") || !object["creation"].is_object())
        {
            return;
        }

        const auto& creation =
            object["creation"];

        definition.creationMode =
            TextTools::toLower(
                creation.value("mode", definition.creationMode)
            );

        if (definition.creationMode != "individual" &&
            definition.creationMode != "grid")
        {
            Logger::warning(
                "json",
                "Unsupported creation mode '" + definition.creationMode +
                "' in '" + definition.id + "'"
            );
        }

        if (definition.creationMode != "grid")
        {
            return;
        }

        if (creation.contains("rules") && creation["rules"].is_object())
        {
            const auto& rules =
                creation["rules"];

            definition.gridRules.rows =
                rules.value("rows", definition.gridRules.rows);

            definition.gridRules.columns =
                rules.value("columns", definition.gridRules.columns);

            definition.gridRules.cellWidth =
                rules.value("cellWidth", definition.gridRules.cellWidth);

            definition.gridRules.cellHeight =
                rules.value("cellHeight", definition.gridRules.cellHeight);
        }

        if (creation.contains("pattern"))
        {
            parseCreationPattern(
                creation["pattern"],
                definition
            );
        }
    }

    void parseCollisions(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("collisions"))
        {
            return;
        }

        if (!object["collisions"].is_object())
        {
            Logger::warning(
                "json",
                "Invalid collisions declaration: expected object"
            );

            return;
        }

        definition.collisions.clear();

        const auto& collisions =
            object["collisions"];

        for (auto it = collisions.begin(); it != collisions.end(); ++it)
        {
            Json colliderData =
                it.value();

            if (colliderData.is_string())
            {
                ResolvedJsonReference reference =
                    resolveJsonReference(
                        session,
                        colliderData.get<std::string>(),
                        currentFile,
                        "collisions." + it.key()
                    );

                if (!reference.ok)
                {
                    continue;
                }

                colliderData =
                    reference.data;
            }

            if (!colliderData.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid collider '" + it.key() + "': expected object"
                );

                continue;
            }

            ColliderDefinition collider;

            collider.type =
                TextTools::toLower(
                    colliderData.value("type", collider.type)
                );

            if (colliderData.contains("size") &&
                colliderData["size"].is_object())
            {
                const auto& size =
                    colliderData["size"];

                if (size.contains("width"))
                {
                    collider.size.width =
                        size.value("width", collider.size.width);
                    collider.size.hasWidth = true;
                }

                if (size.contains("height"))
                {
                    collider.size.height =
                        size.value("height", collider.size.height);
                    collider.size.hasHeight = true;
                }
            }

            if (colliderData.contains("offset") &&
                colliderData["offset"].is_object())
            {
                const auto& offset =
                    colliderData["offset"];

                collider.offset = Vector2{
                    offset.value("x", 0.0f),
                    offset.value("y", 0.0f)
                };
            }

            collider.angle =
                colliderData.value("angle", collider.angle);

            collider.enabled =
                colliderData.value("enabled", collider.enabled);

            if (colliderData.contains("with") &&
                colliderData["with"].is_array())
            {
                for (const auto& group : colliderData["with"])
                {
                    if (group.is_string())
                    {
                        collider.with.push_back(
                            group.get<std::string>()
                        );
                    }
                }
            }

            if (colliderData.contains("states") &&
                colliderData["states"].is_array())
            {
                for (const auto& state : colliderData["states"])
                {
                    if (state.is_string())
                    {
                        collider.states.push_back(
                            state.get<std::string>()
                        );
                    }
                }
            }

            definition.collisions[it.key()] =
                collider;
        }
    }

    Json normalizeSoundValue(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& field
    )
    {
        if (value.is_string())
        {
            const std::string reference =
                value.get<std::string>();

            if (isFlxReference(reference))
            {
                ResolvedJsonReference resolvedReference =
                    resolveJsonReference(
                        session,
                        reference,
                        currentFile,
                        field
                    );

                if (!resolvedReference.ok)
                {
                    return Json{};
                }

                if (resolvedReference.data.contains("sound"))
                {
                    return resolvedReference.data["sound"];
                }

                return resolvedReference.data;
            }

            const auto soundPath =
                resolvePath(
                    currentFile,
                    reference
                );

            Json soundData;

            if (!loadJson(soundPath, soundData))
            {
                return Json{};
            }

            Json resolvedSound;

            if (!resolveLike(session, soundPath, soundData, resolvedSound, field))
            {
                return Json{};
            }

            if (resolvedSound.contains("sound"))
            {
                return resolvedSound["sound"];
            }

            return resolvedSound;
        }

        if (value.is_object() && value.contains("like"))
        {
            Json resolvedSound;

            if (!resolveLike(session, currentFile, value, resolvedSound, field))
            {
                return Json{};
            }

            if (resolvedSound.contains("sound"))
            {
                return resolvedSound["sound"];
            }

            return resolvedSound;
        }

        return value;
    }

    ResolvedJsonReference resolveJsonReference(
        JsonLoadSession& session,
        const std::string& reference,
        const std::filesystem::path& declaringFile,
        const std::string& field
    )
    {
        if (!isFlxReference(reference))
        {
            return {};
        }

        const size_t separator =
            reference.find(':');

        const std::string path =
            separator == std::string::npos
            ? reference
            : reference.substr(0, separator);
        const std::string key =
            separator == std::string::npos
            ? ""
            : reference.substr(separator + 1);

        if (path.empty() || path == "/")
        {
            Logger::error(
                "json",
                "Invalid FLX reference: " + reference
            );

            return {};
        }

        if (separator != std::string::npos && key.empty())
        {
            Logger::error(
                "json",
                "FLX reference key is empty: " + reference
            );

            return {};
        }

        const std::filesystem::path referencePath =
            resolveFlxReferencePath(session, path);

        if (referencePath.empty())
        {
            addReferenceError(
                session,
                DiagnosticCode::MissingReferencedResource,
                "FLX reference could not be resolved.",
                declaringFile,
                field,
                reference,
                referencePath,
                key
            );

            return {};
        }

        Json data;

        if (!loadJsonCached(session, referencePath, data))
        {
            addReferenceError(
                session,
                DiagnosticCode::MissingReferencedResource,
                "FLX reference file not found or invalid.",
                declaringFile,
                field,
                reference,
                referencePath,
                key
            );

            return {};
        }

        Json resolved;

        if (!resolveLike(session, referencePath, data, resolved, field))
        {
            return {};
        }

        if (separator == std::string::npos)
        {
            return ResolvedJsonReference{
                true,
                resolved,
                referencePath,
                referencePath,
                ""
            };
        }

        if (!resolved.contains(key))
        {
            addReferenceError(
                session,
                DiagnosticCode::MissingInternalResourceMember,
                "FLX reference key not found.",
                declaringFile,
                field,
                reference,
                referencePath,
                key
            );

            return {};
        }

        return ResolvedJsonReference{
            true,
            resolved[key],
            referencePath,
            referencePath,
            key
        };
    }

    Json resolveJsonValue(
        JsonLoadSession& session,
        const std::filesystem::path& currentFile,
        const Json& value,
        const std::string& field
    )
    {
        if (value.is_string())
        {
            const std::string reference =
                value.get<std::string>();

            if (isFlxReference(reference))
            {
                ResolvedJsonReference resolvedReference =
                    resolveJsonReference(
                        session,
                        reference,
                        currentFile,
                        field
                    );

                return resolvedReference.ok
                    ? resolvedReference.data
                    : Json{};
            }
        }

        if (value.is_object() && value.contains("like"))
        {
            Json resolved;

            if (!resolveLike(session, currentFile, value, resolved, field))
            {
                return Json{};
            }

            return resolved;
        }

        return value;
    }

    float clampAudioValue(
        float value,
        float minValue,
        float maxValue,
        const std::string& label
    )
    {
        if (value < minValue || value > maxValue)
        {
            Logger::warning(
                "json",
                label + " outside range; clamping"
            );
        }

        return std::clamp(value, minValue, maxValue);
    }

    bool isValidSourceType(const std::string& type)
    {
        static const std::unordered_set<std::string> valid{
            "oscillator",
            "noise",
            "impact",
            "pulse"
        };

        return valid.contains(type);
    }

    bool isValidWave(const std::string& wave)
    {
        static const std::unordered_set<std::string> valid{
            "sine",
            "square",
            "triangle",
            "saw",
            "pulse",
            "noise"
        };

        return valid.contains(wave);
    }

    AudioSourceDefinition parseAudioSource(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& label
    )
    {
        AudioSourceDefinition source;
        const Json data =
            resolveJsonValue(session, currentFile, value, label);

        if (!data.is_object())
        {
            return source;
        }

        source.type =
            TextTools::toLower(data.value("type", source.type));
        source.wave =
            TextTools::toLower(data.value("wave", source.wave));
        source.duty =
            clampAudioValue(
                data.value("duty", source.duty),
                0.05f,
                0.95f,
                label + ".duty"
            );

        if (!isValidSourceType(source.type))
        {
            Logger::warning(
                "json",
                "Invalid source.type in " + label + "; using oscillator"
            );

            source.type = "oscillator";
        }

        if (!isValidWave(source.wave))
        {
            Logger::warning(
                "json",
                "Invalid source.wave in " + label + "; using square"
            );

            source.wave = "square";
        }

        if (source.type == "noise")
        {
            source.wave = "noise";
        }

        return source;
    }

    AudioToneDefinition parseAudioTone(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& label
    )
    {
        AudioToneDefinition tone;
        const Json data =
            resolveJsonValue(session, currentFile, value, label);

        if (!data.is_object())
        {
            return tone;
        }

        if (data.contains("material") && data["material"].is_object())
        {
            const Json& material =
                data["material"];

            tone.material.brightness =
                clampAudioValue(
                    material.value("brightness", tone.material.brightness),
                    0.0f,
                    1.0f,
                    label + ".material.brightness"
                );
            tone.material.roughness =
                clampAudioValue(
                    material.value("roughness", tone.material.roughness),
                    0.0f,
                    1.0f,
                    label + ".material.roughness"
                );
            tone.material.noise =
                clampAudioValue(
                    material.value("noise", tone.material.noise),
                    0.0f,
                    1.0f,
                    label + ".material.noise"
                );
            tone.material.resonance =
                clampAudioValue(
                    material.value("resonance", tone.material.resonance),
                    0.0f,
                    1.0f,
                    label + ".material.resonance"
                );
            tone.material.metal =
                clampAudioValue(
                    material.value("metal", tone.material.metal),
                    0.0f,
                    1.0f,
                    label + ".material.metal"
                );
        }

        if (data.contains("envelope") && data["envelope"].is_object())
        {
            const Json& envelope =
                data["envelope"];

            tone.envelope.attack =
                std::max(0.0f, envelope.value("attack", tone.envelope.attack));
            tone.envelope.decay =
                std::max(0.0f, envelope.value("decay", tone.envelope.decay));
            tone.envelope.sustain =
                clampAudioValue(
                    envelope.value("sustain", tone.envelope.sustain),
                    0.0f,
                    1.0f,
                    label + ".envelope.sustain"
                );
            tone.envelope.release =
                std::max(0.0f, envelope.value("release", tone.envelope.release));
        }

        if (data.contains("space") && data["space"].is_object())
        {
            const Json& space =
                data["space"];

            tone.space.mode =
                TextTools::toLower(space.value("mode", tone.space.mode));

            if (tone.space.mode != "mono" && tone.space.mode != "stereo")
            {
                Logger::warning(
                    "json",
                    "Invalid space.mode in " + label + "; using mono"
                );

                tone.space.mode = "mono";
            }

            tone.space.width =
                clampAudioValue(
                    space.value("width", tone.space.width),
                    0.0f,
                    1.0f,
                    label + ".space.width"
                );
            tone.space.echo =
                clampAudioValue(
                    space.value("echo", tone.space.echo),
                    0.0f,
                    1.0f,
                    label + ".space.echo"
                );
        }

        return tone;
    }

    float parseAudioNote(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& label
    )
    {
        const Json data =
            resolveJsonValue(session, currentFile, value, label);

        if (data.is_number())
        {
            return std::max(1.0f, data.get<float>());
        }

        if (data.is_string())
        {
            float frequency = 0.0f;

            if (NoteTools::noteToFrequency(data.get<std::string>(), frequency))
            {
                return frequency;
            }

            Logger::warning(
                "json",
                "Invalid note in " + label + "; using A4"
            );

            return 440.0f;
        }

        if (data.is_object() && data.contains("frequency"))
        {
            return std::max(1.0f, data.value("frequency", 440.0f));
        }

        return 440.0f;
    }

    AudioMovementDefinition parseAudioMovement(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& label
    )
    {
        AudioMovementDefinition movement;
        const Json data =
            resolveJsonValue(session, currentFile, value, label);

        if (!data.is_object())
        {
            return movement;
        }

        movement.type =
            TextTools::toLower(data.value("type", movement.type));

        static const std::unordered_set<std::string> valid{
            "none",
            "rise",
            "fall",
            "pulse",
            "wobble",
            "scatter",
            "random"
        };

        if (!valid.contains(movement.type))
        {
            Logger::warning(
                "json",
                "Invalid movement.type in " + label + "; using none"
            );

            movement.type = "none";
        }

        movement.amount =
            clampAudioValue(
                data.value("amount", movement.amount),
                0.0f,
                1.0f,
                label + ".movement.amount"
            );

        return movement;
    }

    InstrumentDefinition parseInstrument(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& label
    )
    {
        InstrumentDefinition instrument;
        const Json data =
            resolveJsonValue(session, currentFile, value, label);

        if (!data.is_object())
        {
            return instrument;
        }

        if (data.contains("source"))
        {
            instrument.source =
                parseAudioSource(
                    session,
                    data["source"],
                    currentFile,
                    label + ".source"
                );
        }

        if (data.contains("tone"))
        {
            instrument.tone =
                parseAudioTone(
                    session,
                    data["tone"],
                    currentFile,
                    label + ".tone"
                );
        }

        if (data.contains("play") && data["play"].is_object())
        {
            const Json& play =
                data["play"];

            instrument.play.legato =
                play.value("legato", instrument.play.legato);
            instrument.play.glide =
                clampAudioValue(
                    play.value("glide", instrument.play.glide),
                    0.0f,
                    1.0f,
                    label + ".play.glide"
                );
            instrument.play.vibrato =
                clampAudioValue(
                    play.value("vibrato", instrument.play.vibrato),
                    0.0f,
                    1.0f,
                    label + ".play.vibrato"
                );
        }

        if (data.contains("range") && data["range"].is_object())
        {
            const Json& range =
                data["range"];

            instrument.range.min =
                range.value("min", instrument.range.min);
            instrument.range.max =
                range.value("max", instrument.range.max);
        }

        return instrument;
    }

    void parseSounds(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("sounds") || !object["sounds"].is_object())
        {
            return;
        }

        const auto& sounds =
            object["sounds"];

        for (auto it = sounds.begin(); it != sounds.end(); ++it)
        {
            const Json data =
                normalizeSoundValue(
                    session,
                    it.value(),
                    currentFile,
                    "sounds." + it.key()
                );

            if (!data.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid sound '" + it.key() + "': expected object"
                );

                continue;
            }

            SoundDefinition sound;

            if (data.contains("kind"))
            {
                const Json kindData =
                    resolveJsonValue(
                        session,
                        currentFile,
                        data["kind"],
                        "sounds." + it.key() + ".kind"
                    );

                if (kindData.is_object())
                {
                    if (kindData.contains("source"))
                    {
                        sound.kind.source =
                            parseAudioSource(
                                session,
                                kindData["source"],
                                currentFile,
                                "sound '" + it.key() + "'.kind.source"
                            );
                    }

                    if (kindData.contains("note"))
                    {
                        sound.kind.noteFrequency =
                            parseAudioNote(
                                session,
                                kindData["note"],
                                currentFile,
                                "sound '" + it.key() + "'.kind.note"
                            );
                    }

                    sound.kind.slide =
                        kindData.value("slide", sound.kind.slide);

                    if (kindData.contains("movement"))
                    {
                        sound.kind.movement =
                            parseAudioMovement(
                                session,
                                kindData["movement"],
                                currentFile,
                                "sound '" + it.key() + "'.kind"
                            );
                    }
                }
                else
                {
                    Logger::warning(
                        "json",
                        "Invalid kind in sound '" + it.key() + "': expected object"
                    );
                }
            }

            if (data.contains("tone"))
            {
                sound.tone =
                    parseAudioTone(
                        session,
                        data["tone"],
                        currentFile,
                        "sound '" + it.key() + "'.tone"
                    );
            }

            if (!data.contains("duration"))
            {
                Logger::warning(
                    "json",
                    "Sound '" + it.key() +
                    "' has no duration; using 0.1"
                );
            }

            sound.duration =
                data.value("duration", sound.duration);
            sound.volume =
                clampAudioValue(
                    data.value("volume", sound.volume),
                    0.0f,
                    1.0f,
                    "sound '" + it.key() + "'.volume"
                );

            definition.sounds[it.key()] =
                sound;
        }
    }

    Json normalizeMusicValue(
        JsonLoadSession& session,
        const Json& value,
        const std::filesystem::path& currentFile,
        const std::string& field
    )
    {
        if (value.is_string())
        {
            const std::string reference =
                value.get<std::string>();

            if (isFlxReference(reference))
            {
                ResolvedJsonReference resolvedReference =
                    resolveJsonReference(
                        session,
                        reference,
                        currentFile,
                        field
                    );

                if (!resolvedReference.ok)
                {
                    return Json{};
                }

                if (resolvedReference.data.contains("music"))
                {
                    return resolvedReference.data["music"];
                }

                return resolvedReference.data;
            }

            const auto musicPath =
                resolvePath(
                    currentFile,
                    reference
                );

            Json musicData;

            if (!loadJson(musicPath, musicData))
            {
                return Json{};
            }

            Json resolvedMusic;

            if (!resolveLike(session, musicPath, musicData, resolvedMusic, field))
            {
                return Json{};
            }

            if (resolvedMusic.contains("music"))
            {
                return resolvedMusic["music"];
            }

            return resolvedMusic;
        }

        if (value.is_object() && value.contains("like"))
        {
            Json resolvedMusic;

            if (!resolveLike(session, currentFile, value, resolvedMusic, field))
            {
                return Json{};
            }

            if (resolvedMusic.contains("music"))
            {
                return resolvedMusic["music"];
            }

            return resolvedMusic;
        }

        return value;
    }

    void parseMusic(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        const auto validMusicLength =
            [](const std::string& length)
            {
                return
                    length == "1/1" ||
                    length == "1/2" ||
                    length == "1/4" ||
                    length == "1/8" ||
                    length == "1/16";
            };

        if (!object.contains("music") || !object["music"].is_object())
        {
            return;
        }

        const auto& music =
            object["music"];

        for (auto it = music.begin(); it != music.end(); ++it)
        {
            const Json data =
                normalizeMusicValue(
                    session,
                    it.value(),
                    currentFile,
                    "music." + it.key()
                );

            if (!data.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid music '" + it.key() + "': expected object"
                );

                continue;
            }

            MusicDefinition song;
            song.tempo =
                data.value("tempo", song.tempo);
            song.loop =
                data.value("loop", song.loop);

            if (!data.contains("channels") || !data["channels"].is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid music '" + it.key() +
                    "': expected channels object"
                );

                continue;
            }

            const auto& channels =
                data["channels"];

            for (
                auto channelIt = channels.begin();
                channelIt != channels.end();
                ++channelIt
            )
            {
                if (!channelIt.value().is_object())
                {
                    Logger::warning(
                        "json",
                        "Invalid music channel '" + channelIt.key() +
                        "' in '" + it.key() + "': expected object"
                    );

                    continue;
                }

                const Json& channelData =
                    channelIt.value();

                MusicChannelDefinition channel;
                channel.id =
                    channelIt.key();

                if (channelData.contains("instrument"))
                {
                    channel.instrument =
                        parseInstrument(
                            session,
                            channelData["instrument"],
                            currentFile,
                            "music '" + it.key() +
                            "'.channel '" + channel.id + "'.instrument"
                        );
                }

                if (channelData.contains("wave"))
                {
                    channel.instrument.source.wave =
                        TextTools::toLower(
                            channelData.value(
                                "wave",
                                channel.instrument.source.wave
                            )
                        );

                    if (!isValidWave(channel.instrument.source.wave))
                    {
                        Logger::warning(
                            "json",
                            "Invalid wave in music channel '" + channel.id +
                            "'; using square"
                        );

                        channel.instrument.source.wave = "square";
                    }

                    if (channel.instrument.source.wave == "noise")
                    {
                        channel.instrument.source.type = "noise";
                    }
                }

                channel.volume =
                    clampAudioValue(
                        channelData.value("volume", channel.volume),
                        0.0f,
                        1.0f,
                        "music '" + it.key() +
                        "'.channel '" + channel.id + "'.volume"
                    );
                channel.length =
                    channelData.value("length", channel.length);

                if (!validMusicLength(channel.length))
                {
                    Logger::warning(
                        "json",
                        "Invalid length '" + channel.length +
                        "' in music channel '" + channel.id +
                        "'; using 1/4"
                    );

                    channel.length = "1/4";
                }

                if (
                    !channelData.contains("notes") ||
                    !channelData["notes"].is_array()
                )
                {
                    Logger::warning(
                        "json",
                        "Invalid music channel '" + channel.id +
                        "' in '" + it.key() + "': expected notes array"
                    );

                    continue;
                }

                for (const auto& note : channelData["notes"])
                {
                    if (!note.is_string())
                    {
                        Logger::warning(
                            "json",
                            "Invalid note in music channel '" + channel.id +
                            "': expected string"
                        );

                        continue;
                    }

                    channel.notes.push_back(
                        note.get<std::string>()
                    );
                }

                song.channels.push_back(channel);
            }

            definition.music[it.key()] =
                song;
        }
    }

    void parseStates(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& sourceFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("states"))
        {
            return;
        }

        const auto& states =
            object["states"];

        if (!states.is_object())
        {
            addStateMachineError(
                session,
                DiagnosticCode::InvalidStateMachineDeclaration,
                "Invalid states declaration in '" + definition.id +
                "': expected object",
                sourceFile,
                "states"
            );

            return;
        }

        if (!states.contains("initial"))
        {
            addStateMachineError(
                session,
                DiagnosticCode::MissingStateMachineInitialState,
                "Missing initial state in '" + definition.id + "'",
                sourceFile,
                "states.initial"
            );
        }
        else if (!states["initial"].is_string())
        {
            addStateMachineError(
                session,
                DiagnosticCode::MissingStateMachineInitialState,
                "Invalid initial state in '" + definition.id +
                "': expected non-empty string",
                sourceFile,
                "states.initial"
            );
        }
        else
        {
            definition.initialState =
                states["initial"].get<std::string>();

            if (definition.initialState.empty())
            {
                addStateMachineError(
                    session,
                    DiagnosticCode::MissingStateMachineInitialState,
                    "Invalid initial state in '" + definition.id +
                    "': expected non-empty string",
                    sourceFile,
                    "states.initial"
                );
            }
        }

        for (auto it = states.begin(); it != states.end(); ++it)
        {
            if (it.key() == "initial")
            {
                continue;
            }

            if (!it.value().is_object())
            {
                addStateMachineError(
                    session,
                    DiagnosticCode::InvalidStateMachineDeclaration,
                    "Invalid state '" + it.key() + "' in '" +
                    definition.id + "': expected object",
                    sourceFile,
                    "states." + it.key()
                );

                continue;
            }

            std::vector<std::string> nextStates;

            if (it.value().contains("next"))
            {
                if (!it.value()["next"].is_array())
                {
                    addStateMachineError(
                        session,
                        DiagnosticCode::InvalidStateMachineDeclaration,
                        "Invalid next states in '" + it.key() +
                        "': expected array",
                        sourceFile,
                        "states." + it.key() + ".next"
                    );
                }
                else
                {
                    for (const auto& nextState : it.value()["next"])
                    {
                        if (!nextState.is_string())
                        {
                            addStateMachineError(
                                session,
                                DiagnosticCode::InvalidStateTransitionTarget,
                                "Ignoring invalid next state in '" +
                                it.key() + "': expected string",
                                sourceFile,
                                "states." + it.key() + ".next"
                            );

                            continue;
                        }

                        const std::string target =
                            nextState.get<std::string>();

                        if (target.empty())
                        {
                            addStateMachineError(
                                session,
                                DiagnosticCode::InvalidStateTransitionTarget,
                                "Invalid next state in '" + it.key() +
                                "': expected non-empty string",
                                sourceFile,
                                "states." + it.key() + ".next"
                            );

                            continue;
                        }

                        if (std::find(
                            nextStates.begin(),
                            nextStates.end(),
                            target
                        ) != nextStates.end())
                        {
                            addStateMachineError(
                                session,
                                DiagnosticCode::InvalidStateTransitionTarget,
                                "Duplicate next state '" + target +
                                "' in '" + it.key() + "'",
                                sourceFile,
                                "states." + it.key() + ".next"
                            );

                            continue;
                        }

                        nextStates.push_back(target);
                    }
                }
            }

            definition.stateTransitions[it.key()] =
                nextStates;
        }

        if (
            !definition.initialState.empty() &&
            !definition.stateTransitions.contains(definition.initialState)
            )
        {
            addStateMachineError(
                session,
                DiagnosticCode::MissingStateMachineState,
                "Initial state '" + definition.initialState +
                "' is not declared in '" + definition.id + "'",
                sourceFile,
                "states.initial"
            );
        }

        for (const auto& state : definition.stateTransitions)
        {
            for (const std::string& target : state.second)
            {
                if (!definition.stateTransitions.contains(target))
                {
                    addStateMachineError(
                        session,
                        DiagnosticCode::MissingStateMachineState,
                        "State '" + state.first +
                        "' references missing next state '" + target +
                        "' in '" + definition.id + "'",
                        sourceFile,
                        "states." + state.first + ".next"
                    );
                }
            }
        }
    }

    void parseControl(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("control"))
        {
            return;
        }

        if (!object["control"].is_object())
        {
            Logger::warning(
                "json",
                "Invalid control block in '" + definition.id + "': expected object"
            );

            return;
        }

        const Json& control =
            object["control"];

        definition.controlPlayer =
            control.value("player", definition.controlPlayer);
    }

    void parseLocal(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        definition.local.clear();

        if (!object.contains("local"))
        {
            return;
        }

        if (!object["local"].is_object())
        {
            Logger::warning(
                "loading",
                "Invalid local declaration in '" + definition.id +
                "': expected object"
            );

            return;
        }

        for (auto it = object["local"].begin();
            it != object["local"].end();
            ++it)
        {
            if (it.value().is_boolean())
            {
                definition.local[it.key()] =
                    it.value().get<bool>();
                continue;
            }

            if (it.value().is_number())
            {
                definition.local[it.key()] =
                    it.value().get<double>();
                continue;
            }

            if (it.value().is_string())
            {
                definition.local[it.key()] =
                    it.value().get<std::string>();
                continue;
            }

            Logger::warning(
                "loading",
                "Invalid local value '" + it.key() + "' in '" +
                definition.id +
                "': expected boolean, number or string"
            );
        }
    }

    ObjectDefinition parseDefinition(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& currentFile,
        const std::string& id
    );

    Json normalizeChildValue(const Json& value)
    {
        if (value.is_string())
        {
            return Json{
                { "like", value.get<std::string>() }
            };
        }

        return value;
    }

    void parseChildren(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("children"))
        {
            return;
        }

        if (!object["children"].is_object())
        {
            Logger::warning(
                "json",
                "Invalid children in " + genericPathString(currentFile) +
                ": expected object"
            );

            return;
        }

        const auto& children = object["children"];
        Json childSourceFiles =
            Json::object();

        if (
            object.contains("__childSourceFiles") &&
            object["__childSourceFiles"].is_object()
        )
        {
            childSourceFiles =
                object["__childSourceFiles"];
        }

        std::vector<std::string> childNames;

        for (auto it = children.begin(); it != children.end(); ++it)
        {
            childNames.push_back(it.key());
        }

        for (const std::string& childName : childNames)
        {
            {
                Json childData =
                    normalizeChildValue(children.at(childName));

                if (!childData.is_object())
                {
                    Logger::warning(
                        "json",
                        "Invalid child '" + childName + "': expected object"
                    );

                    continue;
                }

                Json resolvedChild;
                const std::filesystem::path childSourceFile =
                    childSourceFiles.contains(childName)
                    ? std::filesystem::path(childSourceFiles[childName].get<std::string>())
                    : currentFile;

                if (!resolveLike(
                    session,
                    childSourceFile,
                    childData,
                    resolvedChild,
                    "children." + childName
                ))
                {
                    throw std::runtime_error(
                        "Child reference could not be resolved: " +
                        childName
                    );
                }

                const std::filesystem::path resolvedSourceFile =
                    resolvedChild.value(
                        "__sourceFile",
                        genericPathString(childSourceFile)
                    );

                if (
                    definitionIsActive(
                        session,
                        resolvedSourceFile,
                        childName
                        )
                    )
                {
                    ObjectDefinition childDefinition;
                    childDefinition.id =
                        childName;
                    childDefinition.sourcePath =
                        genericPathString(resolvedSourceFile);
                    childDefinition.spawnMode =
                        TextTools::toLower(
                            resolvedChild.value("spawn", childDefinition.spawnMode)
                        );

                    definition.children[childName] =
                        std::make_shared<ObjectDefinition>(
                            std::move(childDefinition)
                        );
                    definition.childSourcePaths[childName] =
                        genericPathString(resolvedSourceFile);

                    continue;
                }

                ObjectDefinition parsedChild =
                    parseDefinition(
                        session,
                        resolvedChild,
                        childSourceFile,
                        childName
                    );
                definition.children.emplace(
                    childName,
                    std::make_shared<ObjectDefinition>(
                        std::move(parsedChild)
                    )
                );
                definition.childSourcePaths[childName] =
                    genericPathString(childSourceFile);
            }
        }
    }

    ObjectDefinition parseDefinition(
        JsonLoadSession& session,
        const Json& object,
        const std::filesystem::path& currentFile,
        const std::string& id
    )
    {
        validateObjectRootProperties(object, id);

        ObjectDefinition definition;
        definition.id = id;

        const std::filesystem::path sourceFile =
            object.value(
                "__sourceFile",
                genericPathString(currentFile)
            );

        definition.sourcePath =
            genericPathString(sourceFile);

        ScopedDefinitionStackEntry stackEntry(
            session,
            sourceFile,
            id
        );

        definition.spawnMode =
            TextTools::toLower(
                object.value("spawn", definition.spawnMode)
            );

        definition.hasOffset =
            object.contains("offset") && object["offset"].is_object();

        if (definition.hasOffset)
        {
            const auto& offset = object["offset"];

            definition.offset = Vector2{
                offset.value("x", 0.0f),
                offset.value("y", 0.0f)
            };
        }

        definition.hasOrigin =
            object.contains("origin") && object["origin"].is_object();

        definition.origin =
            parseOrigin(object);

        definition.size =
            parseSize(object);

        definition.group =
            object.value("group", definition.group);

        definition.visible =
            object.value("visible", definition.visible);

        definition.component =
            object.value("component", definition.component);

        parseLocal(object, definition);
        parseControl(object, definition);
        parseShape(object, definition);
        parseMechanics(object, definition);
        parseInherit(object, definition);
        parseAttach(object, definition);
        parseBounds(object, definition);
        parseBehavior(object, definition);
        parseCreation(object, definition);
        parseCollisions(session, object, sourceFile, definition);
        parseSounds(session, object, sourceFile, definition);
        parseMusic(session, object, sourceFile, definition);
        parseStates(session, object, sourceFile, definition);
        parseChildren(session, object, sourceFile, definition);

        return definition;
    }
}

std::string JsonLoader::resolveProjectPath(
    const std::string& projectPath,
    const std::string& path,
    const std::string& extension
)
{
    const std::filesystem::path normalizedRoot =
        std::filesystem::absolute(projectPath).lexically_normal();

    const std::filesystem::path resolved =
        normalizedRoot /
        normalizedRelativePath(path, extension);

    return genericPathString(resolved);
}

std::string JsonLoader::resolveReferencedPath(
    const std::string& sourceFile,
    const std::string& path,
    const std::string& extension
)
{
    const std::filesystem::path resolved =
        std::filesystem::path(sourceFile).parent_path() /
        normalizedRelativePath(path, extension);

    return genericPathString(resolved);
}

ObjectDefinition JsonLoader::loadObjectDefinition(
    const std::string& path
)
{
    Diagnostics diagnostics;
    return loadObjectDefinition(
        path,
        std::filesystem::path(path).parent_path().generic_string(),
        diagnostics
    );
}

ObjectDefinition JsonLoader::loadObjectDefinition(
    const std::string& path,
    const std::string& projectRoot,
    Diagnostics& diagnostics
)
{
    const std::filesystem::path objectPath(path);

    JsonLoadSession session;
    session.projectJsonRoot =
        std::filesystem::absolute(projectRoot).lexically_normal();
    session.diagnostics =
        &diagnostics;

    Json data;

    if (!loadJson(objectPath, data))
    {
        throw std::runtime_error(
            "JSON object could not be loaded: " +
            genericPathString(objectPath)
        );
    }

    Json resolved;

    const std::string rootKey =
        std::filesystem::absolute(objectPath).lexically_normal().generic_string();

    session.likeStack.push_back(rootKey);

    const bool resolvedOk =
        resolveLike(
            session,
            objectPath,
            data,
            resolved,
            ""
        );

    session.likeStack.pop_back();

    if (!resolvedOk)
    {
        throw std::runtime_error(
            "JSON object could not be resolved: " +
            genericPathString(objectPath)
        );
    }

    return parseDefinition(
        session,
        resolved,
        objectPath,
        objectPath.stem().generic_string()
    );
}
