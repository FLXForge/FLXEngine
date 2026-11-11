#include "../session/SessionTestProject.h"
#include "TemporaryDirectory.h"
#include <compiler/CompiledProjectBinary.h>
#include <compiler/binary/CompiledProjectCodec.h>
#include <core/FlxSession.h>
#include <persistence/PersistenceSystem.h>
#include <iostream>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    CompiledProject fixtureProject()
    {
        auto project = sessionTestProject(
            "let ticks=0;"
            "function born(o){save('cross','text','caf\\u00e9');save('cross','flag',true);}"
            "function action(o){save('cross','number',++ticks+0.5);if(ticks===3)exit();}");
        project.context.name = "Format12CrossPlatform";
        project.context.notes = "Deterministic production-core compatibility fixture";
        auto root = *project.resources.findObject("root");
        root.local["seed"] = 12.5;
        root.childResources["marker"] = "marker";
        ObjectDefinition marker;
        marker.id = "marker";
        marker.spawnMode = "manual";
        ResourceRegistry resources;
        require(resources.addObject("root", root), "fixture root");
        require(resources.addObject("marker", marker), "fixture manual child");
        require(resources.addScript("entry", *project.resources.findScript("entry")), "fixture script");
        project.resources = std::move(resources);
        return project;
    }

    void verify(const std::filesystem::path& file)
    {
        auto decoded = CompiledProjectReader::read(file.string());
        for (const auto& diagnostic : decoded.diagnostics.all())
            std::cerr << diagnosticCodeText(diagnostic.code) << " " << diagnostic.message << '\n';
        require(decoded.success, "read fixture without source files");
        require(decoded.metadata.formatVersion == 12 &&
            flx::binary::CompiledProjectCodec::FormatVersion == 12, "format 12 contract");
        require(!decoded.metadata.producerVersion.empty(), "producer metadata");
        require(decoded.project.context.name == "Format12CrossPlatform" &&
            decoded.project.rootId == "root", "project identity");
        require(decoded.project.resources.objectCount() == 2 &&
            decoded.project.resources.scriptCount() == 1, "resources survived exchange");
        const auto* root = decoded.project.resources.findObject("root");
        require(root && root->childResources.at("marker") == "marker" &&
            std::get<double>(root->local.at("seed")) == 12.5, "relationships and numbers survived exchange");
        require(decoded.project.resources.findObject("marker")->spawnMode == "manual", "manual child contract");
        flx::portability::TemporaryDirectory temporary;
        FlxSessionOptions options;
        options.enableVideo = options.enableAudio = false;
        options.savePath = temporary.path() / "save path with spaces";
        FlxSession session;
        require(session.load(decoded.project, options), "load exchanged project into production core");
        decoded.project = {};
        for (int frame = 0; frame < 4 && !session.finished(); ++frame)
            require(session.update(0.125f), "execute exchanged JavaScript");
        require(session.finished() && session.result().success &&
            session.result().exitReason == FlxSessionExitReason::ScriptRequestedExit &&
            session.result().framesExecuted == 2, "deterministic script completion");
        session.unload();
        PersistenceSystem saves(options.savePath);
        const auto number = saves.load("cross", "number");
        const auto text = saves.load("cross", "text");
        const auto flag = saves.load("cross", "flag");
        require(number && number->numberValue == 3.5, "JavaScript numeric result");
        require(text && text->stringValue == "caf\xc3\xa9", "JavaScript UTF-8 string result");
        require(flag && flag->booleanValue, "JavaScript boolean result");
        std::cout << "[PASS] format=12 producer=" << decoded.metadata.producerVersion
            << " objects=2 scripts=1 frames=2 number=3.5 text=UTF-8 flag=true\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 3 && std::string(argv[1]) == "--write-fixture")
        {
            Diagnostics diagnostics;
            require(CompiledProjectWriter::write(argv[2], fixtureProject(), diagnostics), "write fixture");
            verify(argv[2]);
        }
        else if (argc == 3 && std::string(argv[1]) == "--read-fixture")
        {
            verify(argv[2]);
        }
        else if (argc == 1)
        {
            verify(FLX_WINDOWS_FIXTURE);
            flx::portability::TemporaryDirectory temporary;
            const auto roundtrip = temporary.path() / "native fixture.flxc";
            Diagnostics diagnostics;
            require(CompiledProjectWriter::write(roundtrip.string(), fixtureProject(), diagnostics), "native writer");
            verify(roundtrip);
        }
        else
        {
            throw std::runtime_error("Usage: flx-format12-portability-tests [--write-fixture|--read-fixture file]");
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
