#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    struct ReferenceProject
    {
        std::filesystem::path root;
        std::filesystem::path manifest;
        std::filesystem::path world;
    };

    ReferenceProject createReferenceProject(const std::string& name)
    {
        ReferenceProject project;
        project.root =
            testRoot() / "reference_resolution" / name;
        project.manifest =
            project.root / "game.flx";
        project.world =
            project.root / "world";

        std::filesystem::remove_all(project.root);
        std::filesystem::create_directories(project.world);

        writeFile(
            project.manifest,
            "name=References\n"
            "path=world\n"
            "root=root\n"
        );

        return project;
    }

    const ObjectDefinition* findChild(
        const ObjectDefinition& object,
        const std::string& id
    )
    {
        const auto it =
            object.children.find(id);

        if (it == object.children.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    bool containsScript(
        const CompilationResult& result,
        const std::string& suffix
    )
    {
        for (const auto& script : result.project.resources.allScripts())
        {
            if (script.first.ends_with(suffix))
            {
                return true;
            }
        }

        return false;
    }

    std::string diagnosticsText(const Diagnostics& diagnostics)
    {
        std::string text;

        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            text += diagnostic.file;
            text += " ";
            text += diagnostic.field;
            text += " ";
            text += diagnostic.message;
            text += "\n";
        }

        return text;
    }

    void testRef002InlineObject()
    {
        const ReferenceProject project =
            createReferenceProject("ref002_inline_object");

        writeFile(
            project.world / "root.json",
            "{\n"
            "  \"children\": {\n"
            "    \"ship\": {\n"
            "      \"shape\": {\n"
            "        \"type\": \"circle\",\n"
            "        \"radius\": 4,\n"
            "        \"color\": \"white\"\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-002 inline object should compile");

        const ObjectDefinition* ship =
            findChild(result.project.rootDefinition, "ship");

        require(ship != nullptr, "REF-002 inline child should exist");
        require(ship->shapeType == "circle", "REF-002 inline child shape should be parsed");
        require(ship->radius == 4.0f, "REF-002 inline child radius should be parsed");
    }

    void testRef002RelativeObjectReference()
    {
        const ReferenceProject project =
            createReferenceProject("ref002_relative_object");

        writeFile(
            project.world / "root.json",
            "{ \"children\": { \"ship\": \"objects/ship\" } }\n"
        );
        writeFile(
            project.world / "objects" / "ship.json",
            "{ \"shape\": { \"type\": \"block\", \"size\": { \"width\": 12, \"height\": 6 } } }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-002 relative object reference should compile");

        const ObjectDefinition* ship =
            findChild(result.project.rootDefinition, "ship");

        require(ship != nullptr, "REF-002 relative child should exist");
        require(ship->size.x == 12.0f, "REF-002 relative child size should be parsed");
        require(ship->sourcePath.ends_with("world/objects/ship.json"), "REF-006 relative child should keep referenced source path");
    }

    void testRef005AbsoluteLogicalObjectReference()
    {
        const ReferenceProject project =
            createReferenceProject("ref005_absolute_object");

        writeFile(
            project.world / "root.json",
            "{ \"children\": { \"ship\": \"/objects/ship\" } }\n"
        );
        writeFile(
            project.world / "objects" / "ship.json",
            "{ \"group\": \"player\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-005 absolute logical object reference should compile");

        const ObjectDefinition* ship =
            findChild(result.project.rootDefinition, "ship");

        require(ship != nullptr, "REF-005 absolute logical child should exist");
        require(ship->group == "player", "REF-005 absolute logical child should use world path root");
    }

    void testRef009InternalReferenceValid()
    {
        const ReferenceProject project =
            createReferenceProject("ref009_internal_valid");

        writeFile(
            project.world / "root.json",
            "{ \"children\": { \"ship\": \"/catalog/objects:ship\" } }\n"
        );
        writeFile(
            project.world / "catalog" / "objects.json",
            "{\n"
            "  \"ship\": { \"group\": \"player\" },\n"
            "  \"enemy\": { \"group\": \"enemy\" }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-009 internal reference with valid key should compile");

        const ObjectDefinition* ship =
            findChild(result.project.rootDefinition, "ship");

        require(ship != nullptr, "REF-009 internal child should exist");
        require(ship->group == "player", "REF-009 internal child should load selected member only");
    }

    void testRef009InternalReferenceMissingMember()
    {
        const ReferenceProject project =
            createReferenceProject("ref009_internal_missing_current");

        writeFile(
            project.world / "root.json",
            "{ \"children\": { \"ship\": \"/catalog/objects:missing\" } }\n"
        );
        writeFile(
            project.world / "catalog" / "objects.json",
            "{ \"ship\": { \"group\": \"player\" } }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(!result.success, "REF-009 missing internal child member should fail compilation");
        require(result.diagnostics.hasErrors(), "REF-009 missing internal member should produce diagnostics");

        const std::string text =
            diagnosticsText(result.diagnostics);

        require(text.find("FLX reference key not found") != std::string::npos, "REF-009 diagnostic should explain missing member");
        require(text.find("/catalog/objects:missing") != std::string::npos, "REF-009 diagnostic should include declared reference");
        require(text.find("children.ship") != std::string::npos, "REF-009 diagnostic should include declaring field");
        require(text.find("objects.json") != std::string::npos, "REF-009 diagnostic should include effective JSON path");
        require(text.find("missing") != std::string::npos, "REF-009 diagnostic should include missing member");
    }

    void testRef006RelativeScriptDirect()
    {
        const ReferenceProject project =
            createReferenceProject("ref006_relative_script");

        writeFile(
            project.world / "root.json",
            "{ \"behavior\": { \"scripts\": [\"scripts/root\"] } }\n"
        );
        writeFile(
            project.world / "scripts" / "root.js",
            "function born(root) {}\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-006 direct relative script should compile");
        require(containsScript(result, "world/scripts/root.js"), "REF-006 direct relative script should resolve from declaring file");
    }

    void testRef005AbsoluteLogicalScript()
    {
        const ReferenceProject project =
            createReferenceProject("ref005_absolute_script_current");

        writeFile(
            project.world / "root.json",
            "{ \"behavior\": { \"scripts\": [\"/scripts/root\"] } }\n"
        );
        writeFile(
            project.world / "scripts" / "root.js",
            "function born(root) {}\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-005 absolute logical script references should resolve from path");
        require(containsScript(result, "world/scripts/root.js"), "REF-005 absolute logical script should be embedded");
    }

    void testRef007InheritedScriptUsesBaseFile()
    {
        const ReferenceProject project =
            createReferenceProject("ref007_inherited_script");

        writeFile(
            project.world / "root.json",
            "{ \"like\": \"base/base\" }\n"
        );
        writeFile(
            project.world / "base" / "base.json",
            "{ \"behavior\": { \"scripts\": [\"base\"] } }\n"
        );
        writeFile(
            project.world / "base" / "base.js",
            "function born(root) {}\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-007 inherited script should compile");
        require(containsScript(result, "world/base/base.js"), "REF-007 inherited script should resolve from base file");
    }

    void testRef008OverriddenScript()
    {
        const ReferenceProject project =
            createReferenceProject("ref008_overridden_script_current");

        writeFile(
            project.world / "consumer" / "root.json",
            "{ \"like\": \"../base/base\", \"behavior\": { \"scripts\": [\"consumer\"] } }\n"
        );
        writeFile(
            project.manifest,
            "name=References\n"
            "path=world\n"
            "root=consumer/root\n"
        );
        writeFile(
            project.world / "base" / "base.json",
            "{ \"behavior\": { \"scripts\": [\"base\"] } }\n"
        );
        writeFile(
            project.world / "consumer" / "consumer.js",
            "function born(root) {}\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-008 overridden script should resolve from consumer file");
        require(containsScript(result, "world/consumer/consumer.js"), "REF-008 overridden script should be embedded from consumer directory");
        require(!containsScript(result, "world/base/consumer.js"), "REF-008 overridden script should not resolve from base directory");
    }

    void testRef007InheritedChild()
    {
        const ReferenceProject project =
            createReferenceProject("ref007_inherited_child");

        writeFile(
            project.world / "root.json",
            "{ \"like\": \"base/base\" }\n"
        );
        writeFile(
            project.world / "base" / "base.json",
            "{ \"children\": { \"enemy\": \"enemy\" } }\n"
        );
        writeFile(
            project.world / "base" / "enemy.json",
            "{ \"group\": \"enemy\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-007 inherited child should compile");

        const ObjectDefinition* enemy =
            findChild(result.project.rootDefinition, "enemy");

        require(enemy != nullptr, "REF-007 inherited child should exist");
        require(enemy->group == "enemy", "REF-007 inherited child should resolve from base file");
    }

    void testRef008OverriddenChild()
    {
        const ReferenceProject project =
            createReferenceProject("ref008_overridden_child_current");

        writeFile(
            project.world / "consumer" / "root.json",
            "{ \"like\": \"../base/base\", \"children\": { \"item\": \"consumer-item\" } }\n"
        );
        writeFile(
            project.manifest,
            "name=References\n"
            "path=world\n"
            "root=consumer/root\n"
        );
        writeFile(
            project.world / "base" / "base.json",
            "{ \"children\": { \"item\": \"base-item\" } }\n"
        );
        writeFile(
            project.world / "base" / "base-item.json",
            "{ \"group\": \"base\" }\n"
        );
        writeFile(
            project.world / "consumer" / "consumer-item.json",
            "{ \"group\": \"consumer\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-008 overridden child should compile");

        const ObjectDefinition* item =
            findChild(result.project.rootDefinition, "item");

        require(item != nullptr, "REF-008 overridden child should exist");
        require(item->group == "consumer", "REF-008 overridden child should resolve from consumer file");
    }

    void testRef012LikeChain()
    {
        const ReferenceProject project =
            createReferenceProject("ref012_like_chain");

        writeFile(
            project.world / "root.json",
            "{ \"like\": \"level/level\", \"role\": \"root\" }\n"
        );
        writeFile(
            project.world / "level" / "level.json",
            "{ \"like\": \"../base/base\", \"group\": \"level\" }\n"
        );
        writeFile(
            project.world / "base" / "base.json",
            "{ \"visible\": false, \"group\": \"base\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-012 chain of like should compile");
        require(!result.project.rootDefinition.visible, "REF-012 chain should inherit base properties");
        require(result.project.rootDefinition.group == "level", "REF-012 chain should allow intermediate override");
        require(result.project.rootDefinition.role == "root", "REF-012 chain should allow final override");
    }

    void testRef010LikeCycle()
    {
        const ReferenceProject project =
            createReferenceProject("ref010_like_cycle");

        writeFile(
            project.world / "root.json",
            "{ \"like\": \"a\" }\n"
        );
        writeFile(
            project.world / "a.json",
            "{ \"like\": \"b\" }\n"
        );
        writeFile(
            project.world / "b.json",
            "{ \"like\": \"a\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(!result.success, "REF-010 like cycle should fail compilation");
        require(result.diagnostics.hasErrors(), "REF-010 like cycle should produce diagnostics");

        const std::string text =
            diagnosticsText(result.diagnostics);

        require(text.find("Resource reference cycle detected") != std::string::npos, "REF-010 diagnostic should explain the cycle");
        require(text.find("Chain:") != std::string::npos, "REF-010 diagnostic should include known reference chain");
        require(text.find("a.json") != std::string::npos, "REF-010 diagnostic should include cycle participant");
        require(text.find("b.json") != std::string::npos, "REF-010 diagnostic should include cycle participant");
    }

    void testRef006DifferentDirectoryReferences()
    {
        const ReferenceProject project =
            createReferenceProject("ref006_different_directories");

        writeFile(
            project.world / "screens" / "root.json",
            "{ \"children\": { \"enemy\": \"../shared/enemy\" } }\n"
        );
        writeFile(
            project.manifest,
            "name=References\n"
            "path=world\n"
            "root=screens/root\n"
        );
        writeFile(
            project.world / "shared" / "enemy.json",
            "{ \"group\": \"enemy\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-006 references from different directories should compile");
        require(findChild(result.project.rootDefinition, "enemy") != nullptr, "REF-006 cross-directory relative reference should resolve");
    }

    void testRef014ConsecutiveCompilationsDoNotReusePreviousRoot()
    {
        const ReferenceProject first =
            createReferenceProject("ref014_first_root");
        const ReferenceProject second =
            createReferenceProject("ref014_second_root");

        writeFile(
            first.world / "root.json",
            "{ \"children\": { \"marker\": \"/markers/first\" } }\n"
        );
        writeFile(
            first.world / "markers" / "first.json",
            "{ \"group\": \"first\" }\n"
        );

        writeFile(
            second.world / "root.json",
            "{ \"children\": { \"marker\": \"/markers/second\" } }\n"
        );
        writeFile(
            second.world / "markers" / "second.json",
            "{ \"group\": \"second\" }\n"
        );

        const CompilationResult firstResult =
            compile(first.manifest);
        const CompilationResult secondResult =
            compile(second.manifest);

        require(firstResult.success, "REF-014 first compilation should succeed");
        require(secondResult.success, "REF-014 second compilation should succeed");

        const ObjectDefinition* firstMarker =
            findChild(firstResult.project.rootDefinition, "marker");
        const ObjectDefinition* secondMarker =
            findChild(secondResult.project.rootDefinition, "marker");

        require(firstMarker != nullptr && firstMarker->group == "first", "REF-014 first compilation should use first project root");
        require(secondMarker != nullptr && secondMarker->group == "second", "REF-014 second compilation should use second project root");
    }

    void testRef015MissingReference()
    {
        const ReferenceProject project =
            createReferenceProject("ref015_missing_reference_current");

        writeFile(
            project.world / "root.json",
            "{ \"children\": { \"missing\": \"missing/object\" } }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(!result.success, "REF-015 missing child reference should fail compilation");
        require(result.diagnostics.hasErrors(), "REF-015 missing child reference should produce diagnostics");

        const std::string text =
            diagnosticsText(result.diagnostics);

        require(text.find("missing/object") != std::string::npos, "REF-015 diagnostic should include declared child reference");
        require(text.find("children.missing") != std::string::npos, "REF-015 diagnostic should include child field");
        require(text.find("missing/object.json") != std::string::npos, "REF-015 diagnostic should include effective searched path");
    }

    void testRef015Diagnostics()
    {
        const ReferenceProject project =
            createReferenceProject("ref015_diagnostics_current");

        writeFile(
            project.world / "root.json",
            "{ \"behavior\": { \"scripts\": [\"missing-script\"] } }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(!result.success, "REF-015 missing direct script should fail compilation");
        require(result.diagnostics.hasErrors(), "REF-015 missing direct script should be a compiler diagnostic");

        const std::string text =
            diagnosticsText(result.diagnostics);

        require(text.find("missing-script") != std::string::npos, "REF-015 diagnostic should include declared script reference");
        require(text.find("root.json") != std::string::npos, "REF-015 diagnostic should preserve declaring JSON file");
        require(text.find("behavior.scripts") != std::string::npos, "REF-015 diagnostic should preserve structured reference field");
    }

    void testRef011ResourceIdCollisionCurrentNotReproduced()
    {
        const ReferenceProject project =
            createReferenceProject("ref011_collision_current");

        writeFile(
            project.world / "root.json",
            "{\n"
            "  \"children\": {\n"
            "    \"a\": \"objects/item\",\n"
            "    \"b\": \"objects/item\"\n"
            "  }\n"
            "}\n"
        );
        writeFile(
            project.world / "objects" / "item.json",
            "{ \"group\": \"shared\" }\n"
        );

        const CompilationResult result =
            compile(project.manifest);

        require(result.success, "REF-011 current behavior: repeated use of same source under different child ids compiles");
        require(result.project.resources.objectCount() >= 2, "REF-011 current behavior: ids include logical child id, so this is reuse not a collision");
    }

    void testPendingContractSummary()
    {
        const std::vector<std::string> pending = {
            "REF-011 ResourceId collision checks are implemented, but no deterministic source-level collision was reproduced with the current id model.",
            "Diagnostics details remain encoded in the message because Diagnostics has no structured details field yet."
        };

        require(!pending.empty(), "pending contract summary should document known failures");

        std::cout << "[PENDING CONTRACT]\n";

        for (const std::string& item : pending)
        {
            std::cout << "- " << item << "\n";
        }
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "REF-002 inline object", testRef002InlineObject },
        { "REF-002 relative object reference", testRef002RelativeObjectReference },
        { "REF-005 absolute logical object reference", testRef005AbsoluteLogicalObjectReference },
        { "REF-009 internal reference valid", testRef009InternalReferenceValid },
        { "REF-009 missing internal member", testRef009InternalReferenceMissingMember },
        { "REF-006 relative script direct", testRef006RelativeScriptDirect },
        { "REF-005 absolute logical script", testRef005AbsoluteLogicalScript },
        { "REF-007 inherited script uses base file", testRef007InheritedScriptUsesBaseFile },
        { "REF-008 overridden script", testRef008OverriddenScript },
        { "REF-007 inherited child", testRef007InheritedChild },
        { "REF-008 overridden child", testRef008OverriddenChild },
        { "REF-012 like chain", testRef012LikeChain },
        { "REF-010 like cycle", testRef010LikeCycle },
        { "REF-006 different directory references", testRef006DifferentDirectoryReferences },
        { "REF-014 consecutive compilations", testRef014ConsecutiveCompilationsDoNotReusePreviousRoot },
        { "REF-015 missing reference", testRef015MissingReference },
        { "REF-015 diagnostics", testRef015Diagnostics },
        { "REF-011 current ResourceId collision characterization", testRef011ResourceIdCollisionCurrentNotReproduced },
        { "Pending reference contract summary", testPendingContractSummary }
    };

    for (const auto& test : tests)
    {
        try
        {
            test.second();
            std::cout << "[PASS] " << test.first << "\n";
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";
            return 1;
        }
    }

    return 0;
}
