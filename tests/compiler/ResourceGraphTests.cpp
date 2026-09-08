#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectValidator.h"
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
    const ObjectDefinition& rootObject(const CompiledProject& project)
    {
        const ObjectDefinition* root =
            project.resources.findObject(project.rootId);

        require(root != nullptr, "root should exist in registry");
        return *root;
    }

    bool hasErrorCode(
        const Diagnostics& diagnostics,
        DiagnosticCode code
    )
    {
        return std::any_of(
            diagnostics.all().begin(),
            diagnostics.all().end(),
            [code](const Diagnostic& diagnostic)
            {
                return
                    diagnostic.severity == DiagnosticSeverity::Error &&
                    diagnostic.code == code;
            }
        );
    }

    void testGraphWithAutoAndManualChildren()
    {
        const std::filesystem::path root =
            testRoot() / "graph_children";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Graph\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"children\": {\n"
            "    \"ship\": { \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 8 } } },\n"
            "    \"laser\": { \"spawn\": \"manual\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 1, \"height\": 4 } } }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "graph with children should compile");
        require(result.project.resources.objectCount() == 3, "root and two children should be registered");
        require(rootObject(result.project).children.empty(), "compiled root should not keep embedded children");
        require(rootObject(result.project).childResources.count("ship") == 1, "auto child should have resource id");
        require(rootObject(result.project).childResources.count("laser") == 1, "manual child should have resource id");
    }

    void testGraphWithGridChildren()
    {
        const std::filesystem::path root =
            testRoot() / "graph_grid";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Grid\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"creation\": {\n"
            "    \"mode\": \"grid\",\n"
            "    \"rules\": { \"rows\": 1, \"columns\": 2, \"cellWidth\": 8, \"cellHeight\": 8 },\n"
            "    \"pattern\": [\"brick\"]\n"
            "  },\n"
            "  \"children\": {\n"
            "    \"brick\": { \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 8 } } }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "grid graph should compile");
        require(result.project.resources.objectCount() == 2, "grid child should be registered");
        require(rootObject(result.project).childResources.count("brick") == 1, "grid child should have resource id");
    }

    void testGraphWithLike()
    {
        const std::filesystem::path root =
            testRoot() / "graph_like";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "objects");

        writeFile(
            root / "game.flx",
            "name=Like\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"children\": { \"brick\": { \"like\": \"objects/brick\", \"visible\": false } } }\n"
        );

        writeFile(
            root / "game" / "objects" / "brick.json",
            "{ \"group\": \"brick\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 4 } } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "like graph should compile");
        const std::string childId =
            rootObject(result.project).childResources.at("brick");
        const ObjectDefinition* child =
            result.project.resources.findObject(childId);

        require(child != nullptr, "liked child should be registered");
        require(child->group == "brick", "liked child should inherit group");
        require(!child->visible, "liked child should keep override");
    }

    void testValidFlxReference()
    {
        const std::filesystem::path root =
            testRoot() / "valid_reference";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "blocks");

        writeFile(
            root / "game.flx",
            "name=Reference\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"shape\": \"/blocks/shapes:dot\" }\n"
        );

        writeFile(
            root / "game" / "blocks" / "shapes.json",
            "{ \"dot\": { \"type\": \"circle\", \"radius\": 3, \"color\": \"white\" } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "valid FLX reference should compile");
        require(rootObject(result.project).shapeType == "circle", "referenced shape should resolve");
        require(rootObject(result.project).radius == 3.0f, "referenced radius should resolve");
    }

    void testInvalidFlxReference()
    {
        const std::filesystem::path root =
            testRoot() / "invalid_reference";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=InvalidReference\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"shape\": \"/missing:block\" }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "invalid FLX reference should fail");
        require(result.diagnostics.hasErrors(), "invalid FLX reference should report errors");
    }

    void testAutomaticInstantiationCycleFailsCompiledProjectValidation()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.childResources["a"] = "a";

        ObjectDefinition a;
        a.id = "a";
        a.childResources["b"] = "b";

        ObjectDefinition b;
        b.id = "b";
        b.childResources["a"] = "a";

        require(project.resources.addObject("root", root), "root should be added");
        require(project.resources.addObject("a", a), "a should be added");
        require(project.resources.addObject("b", b), "b should be added");

        Diagnostics diagnostics;

        const bool valid =
            CompiledProjectValidator::validate(
                project,
                diagnostics,
                "automatic-cycle-test"
            );

        require(!valid, "automatic instantiation cycle should fail compiled project validation");
        require(diagnostics.hasErrors(), "automatic instantiation cycle should report diagnostics");
    }

    void testAutoChildCycleFailsCompilation()
    {
        const std::filesystem::path root =
            testRoot() / "auto_child_cycle";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=AutoCycle\n"
            "path=game\n"
            "root=a\n"
        );

        writeFile(
            root / "game" / "a.json",
            "{ \"children\": { \"b\": \"b\" } }\n"
        );

        writeFile(
            root / "game" / "b.json",
            "{ \"children\": { \"a\": \"a\" } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "auto A to B to A should fail compilation");
        require(
            hasErrorCode(result.diagnostics, DiagnosticCode::AutomaticInstantiationCycle),
            "auto cycle should report AutomaticInstantiationCycle"
        );
    }

    void testManualChildCycleCompiles()
    {
        const std::filesystem::path root =
            testRoot() / "manual_child_cycle";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=ManualCycle\n"
            "path=game\n"
            "root=a\n"
        );

        writeFile(
            root / "game" / "a.json",
            "{ \"children\": { \"b\": { \"like\": \"b\", \"spawn\": \"manual\" } } }\n"
        );

        writeFile(
            root / "game" / "b.json",
            "{ \"children\": { \"a\": { \"like\": \"a\", \"spawn\": \"manual\" } } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "manual A to B to A should compile");
        require(
            !hasErrorCode(result.diagnostics, DiagnosticCode::AutomaticInstantiationCycle),
            "manual cycle should not report AutomaticInstantiationCycle"
        );
    }

    void testMixedManualEdgeDoesNotFormAutomaticCycle()
    {
        const std::filesystem::path root =
            testRoot() / "mixed_child_cycle";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=MixedCycle\n"
            "path=game\n"
            "root=a\n"
        );

        writeFile(
            root / "game" / "a.json",
            "{ \"children\": { \"b\": \"b\" } }\n"
        );

        writeFile(
            root / "game" / "b.json",
            "{ \"children\": { \"a\": { \"like\": \"a\", \"spawn\": \"manual\" } } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "auto then manual edge should not form an automatic cycle");
        require(
            !hasErrorCode(result.diagnostics, DiagnosticCode::AutomaticInstantiationCycle),
            "mixed cycle should not report AutomaticInstantiationCycle"
        );
    }

    void testSameLogicalIdsInDifferentFilesDoNotCreateAutoCycle()
    {
        const std::filesystem::path root =
            testRoot() / "same_logical_id_no_cycle";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "a");
        std::filesystem::create_directories(root / "game" / "b");

        writeFile(
            root / "game.flx",
            "name=SameLogicalId\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"children\": { \"title\": \"a/title\" } }\n"
        );

        writeFile(
            root / "game" / "a" / "title.json",
            "{ \"children\": { \"title\": \"../b/title\" } }\n"
        );

        writeFile(
            root / "game" / "b" / "title.json",
            "{ \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 8 } } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "same logical ids in different files should compile");
        require(
            !hasErrorCode(result.diagnostics, DiagnosticCode::AutomaticInstantiationCycle),
            "same logical ids in different files should use ResourceId and avoid false cycles"
        );
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "graph with auto and manual children", testGraphWithAutoAndManualChildren },

        { "graph with grid children", testGraphWithGridChildren },

        { "graph with like", testGraphWithLike },

        { "valid FLX reference", testValidFlxReference },

        { "invalid FLX reference", testInvalidFlxReference },
        { "automatic instantiation cycle fails compiled project validation", testAutomaticInstantiationCycleFailsCompiledProjectValidation },
        { "auto child cycle fails compilation", testAutoChildCycleFailsCompilation },
        { "manual child cycle compiles", testManualChildCycleCompiles },
        { "mixed manual edge does not form automatic cycle", testMixedManualEdgeDoesNotFormAutomaticCycle },
        { "same logical ids in different files do not create auto cycle", testSameLogicalIdsInDifferentFilesDoNotCreateAutoCycle }

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

