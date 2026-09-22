#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectValidator.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
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
            "    \"ship\": { \"size\": { \"width\": 8, \"height\": 8 }, \"visual\": { \"representation\": [{ \"primitive\": \"rectangle\" }] } },\n"
            "    \"laser\": { \"spawn\": \"manual\", \"size\": { \"width\": 1, \"height\": 4 }, \"visual\": { \"representation\": [{ \"primitive\": \"rectangle\" }] } }\n"
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
            "    \"brick\": { \"size\": { \"width\": 8, \"height\": 8 }, \"visual\": { \"representation\": [{ \"primitive\": \"rectangle\" }] } }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "grid graph should compile");
        require(result.project.resources.objectCount() == 2, "grid child should be registered");
        require(rootObject(result.project).childResources.count("brick") == 1, "grid child should have resource id");
    }

    void testGraphWithIteratorChildren()
    {
        const std::filesystem::path root =
            testRoot() / "graph_iterator";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Iterator\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"creation\": {\n"
            "    \"mode\": \"iterator\",\n"
            "    \"pattern\": [\"enemy\"]\n"
            "  },\n"
            "  \"children\": {\n"
            "    \"enemy\": { \"size\": { \"width\": 8, \"height\": 8 }, \"visual\": { \"representation\": [{ \"primitive\": \"rectangle\" }] } }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "iterator graph should compile");
        require(rootObject(result.project).creationMode == "iterator", "iterator mode should compile");
        require(rootObject(result.project).iteratorRules.concurrent == 1, "iterator default concurrent should be 1");
        require(!rootObject(result.project).iteratorRules.repeat, "iterator default repeat should be false");
        require(rootObject(result.project).iteratorPattern.size() == 1, "iterator pattern should compile");
        require(rootObject(result.project).iteratorPattern.front() == "enemy", "iterator pattern child id should compile");
    }

    void testIteratorCreationRejectsInvalidContracts()
    {
        const auto compileIterator =
            [](const std::string& name, const std::string& rootJson)
            {
                const std::filesystem::path root =
                    testRoot() / name;

                std::filesystem::remove_all(root);
                std::filesystem::create_directories(root / "game");

                writeFile(
                    root / "game.flx",
                    "name=IteratorInvalid\n"
                    "path=game\n"
                    "root=root\n"
                );

                writeFile(
                    root / "game" / "root.json",
                    rootJson
                );

                return compile(root / "game.flx");
            };

        {
            const CompilationResult result =
                compileIterator(
                    "iterator_empty_pattern",
                    "{\n"
                    "  \"creation\": { \"mode\": \"iterator\", \"pattern\": [] },\n"
                    "  \"children\": { \"enemy\": {} }\n"
                    "}\n"
                );

            require(!result.success, "iterator empty pattern should fail");
            require(result.diagnostics.hasErrors(), "iterator empty pattern should produce diagnostics");
        }

        {
            const CompilationResult result =
                compileIterator(
                    "iterator_missing_child",
                    "{\n"
                    "  \"creation\": { \"mode\": \"iterator\", \"pattern\": [\"missing\"] },\n"
                    "  \"children\": { \"enemy\": {} }\n"
                    "}\n"
                );

            require(!result.success, "iterator missing child should fail");
            require(result.diagnostics.hasErrors(), "iterator missing child should produce diagnostics");
        }

        {
            const CompilationResult result =
                compileIterator(
                    "iterator_manual_child",
                    "{\n"
                    "  \"creation\": { \"mode\": \"iterator\", \"pattern\": [\"enemy\"] },\n"
                    "  \"children\": { \"enemy\": { \"spawn\": \"manual\" } }\n"
                    "}\n"
                );

            require(!result.success, "iterator manual child should fail in v1");
            require(result.diagnostics.hasErrors(), "iterator manual child should produce diagnostics");
        }

        {
            const CompilationResult result =
                compileIterator(
                    "iterator_invalid_concurrent",
                    "{\n"
                    "  \"creation\": { \"mode\": \"iterator\", \"rules\": { \"concurrent\": 0 }, \"pattern\": [\"enemy\"] },\n"
                    "  \"children\": { \"enemy\": {} }\n"
                    "}\n"
                );

            require(!result.success, "iterator concurrent below one should fail");
            require(result.diagnostics.hasErrors(), "iterator concurrent below one should produce diagnostics");
        }
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
            "{ \"group\": \"brick\", \"size\": { \"width\": 8, \"height\": 4 }, \"visual\": { \"representation\": [{ \"primitive\": \"rectangle\" }] } }\n"
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
            "{ \"visual\": \"/blocks/shapes:dot\" }\n"
        );

        writeFile(
            root / "game" / "blocks" / "shapes.json",
            "{ \"dot\": { \"color\": \"white\", \"representation\": [{ \"primitive\": \"ellipse\", \"size\": { \"width\": 6, \"height\": 6 } }] } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "valid FLX reference should compile");
        require(rootObject(result.project).visual.representation.size() == 1, "referenced visual should resolve");
        require(rootObject(result.project).visual.representation[0].primitive == "ellipse", "referenced primitive should resolve");
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
            "{ \"visual\": \"/missing:block\" }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "invalid FLX reference should fail");
        require(result.diagnostics.hasErrors(), "invalid FLX reference should report errors");
    }

    void testVisualRepresentationCompiles()
    {
        const std::filesystem::path root =
            testRoot() / "visual_representation";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Visual\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"size\": { \"width\": 20, \"height\": 10 },\n"
            "  \"visual\": {\n"
            "    \"color\": \"white\",\n"
            "    \"depth\": 5,\n"
            "    \"representation\": [\n"
            "      { \"primitive\": \"rectangle\", \"size\": { \"width\": \"50%\", \"height\": 4 } },\n"
            "      { \"geometry\": [{ \"x\": -2, \"y\": 0 }, { \"x\": 0, \"y\": -2 }, { \"x\": 2, \"y\": 0 }], \"mode\": \"fill\" },\n"
            "      { \"text\": \"OK\", \"fontSize\": 8 }\n"
            "    ]\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "visual representation should compile");

        const ObjectDefinition& object =
            rootObject(result.project);

        require(object.hasSize, "object size should be present");
        require(object.hasVisual, "visual block should be present");
        require(object.visual.hasColor, "visual base color should be present");
        require(object.visual.depth == 5, "visual depth should compile");
        require(object.visual.representation.size() == 3, "representation order should compile");
        require(object.visual.representation[0].kind == RepresentationElementKind::Primitive, "primitive element should compile");
        require(object.visual.representation[0].size.width.percentage, "primitive percentage size should compile");
        require(object.visual.representation[1].kind == RepresentationElementKind::Geometry, "geometry element should compile");
        require(object.visual.representation[1].geometryMode == "fill", "geometry mode should compile");
        require(object.visual.representation[2].kind == RepresentationElementKind::Text, "text element should compile");
        require(object.visual.representation[2].fontSize == 8, "text font size should compile");
    }

    void testOnePointGeometryIsAcceptedBySchemaAndCompiler()
    {
        std::filesystem::path schemaPath =
            "docs/schemas/visual.schema.json";

        if (!std::filesystem::exists(schemaPath))
        {
            schemaPath =
                "../../../docs/schemas/visual.schema.json";
        }

        std::ifstream schemaFile(schemaPath);
        require(schemaFile.good(), "visual schema should be readable");

        const nlohmann::json schema =
            nlohmann::json::parse(schemaFile);

        require(
            schema["$defs"]["geometryElement"]["allOf"][1]["properties"]["geometry"]["minItems"].get<int>() == 1,
            "visual schema should accept one-point geometry"
        );

        const std::filesystem::path root =
            testRoot() / "one_point_geometry";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=OnePointGeometry\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"visual\": {\n"
            "    \"representation\": [\n"
            "      { \"geometry\": [{ \"x\": 10, \"y\": 20 }], \"mode\": \"open\" },\n"
            "      { \"geometry\": [{ \"x\": 11, \"y\": 20 }], \"mode\": \"close\" },\n"
            "      { \"geometry\": [{ \"x\": 12, \"y\": 20 }], \"mode\": \"fill\" }\n"
            "    ]\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "one-point geometry should compile");

        const ObjectDefinition& object =
            rootObject(result.project);

        require(object.visual.representation.size() == 3, "all one-point geometry modes should be preserved");

        for (const RepresentationElementDefinition& element : object.visual.representation)
        {
            require(element.kind == RepresentationElementKind::Geometry, "one-point element should remain geometry");
            require(element.geometry.size() == 1, "one-point geometry should keep exactly one point");
        }

        writeFile(
            root / "game" / "root.json",
            "{ \"visual\": { \"representation\": [{ \"geometry\": [] }] } }\n"
        );

        const CompilationResult invalid =
            compile(root / "game.flx");

        require(!invalid.success, "empty geometry should fail compilation");
        require(invalid.diagnostics.hasErrors(), "empty geometry should report an error");
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
            "{ \"size\": { \"width\": 8, \"height\": 8 }, \"visual\": { \"representation\": [{ \"primitive\": \"rectangle\" }] } }\n"
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
        { "graph with iterator children", testGraphWithIteratorChildren },
        { "iterator creation rejects invalid contracts", testIteratorCreationRejectsInvalidContracts },

        { "graph with like", testGraphWithLike },

        { "valid FLX reference", testValidFlxReference },

        { "invalid FLX reference", testInvalidFlxReference },
        { "visual representation compiles", testVisualRepresentationCompiles },
        { "one-point geometry is accepted by schema and compiler", testOnePointGeometryIsAcceptedBySchemaAndCompiler },
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

