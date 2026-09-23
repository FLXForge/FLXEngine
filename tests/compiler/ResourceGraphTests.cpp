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

    bool diagnosticsContain(
        const Diagnostics& diagnostics,
        const std::string& text
    )
    {
        return std::any_of(
            diagnostics.all().begin(),
            diagnostics.all().end(),
            [&text](const Diagnostic& diagnostic)
            {
                return diagnostic.message.find(text) != std::string::npos;
            }
        );
    }

    nlohmann::json readSchema(const std::string& name)
    {
        const std::filesystem::path schemaPath =
            std::filesystem::path(FLX_SOURCE_DIR) /
            "docs" /
            "schemas" /
            name;

        std::ifstream schemaFile(schemaPath);
        require(schemaFile.good(), "schema should be readable: " + name);

        return nlohmann::json::parse(schemaFile);
    }

    const nlohmann::json& creationBranch(
        const nlohmann::json& schema,
        const std::string& mode
    )
    {
        for (const nlohmann::json& branch : schema["oneOf"])
        {
            if (
                branch.contains("properties") &&
                branch["properties"].contains("mode") &&
                branch["properties"]["mode"].contains("const") &&
                branch["properties"]["mode"]["const"] == mode
            )
            {
                return branch;
            }
        }

        throw std::runtime_error(
            "creation schema branch not found: " + mode
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
        const nlohmann::json schema =
            readSchema("visual.schema.json");

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

    void testObjectSchemaClosedContract()
    {
        const nlohmann::json objectSchema =
            readSchema("object.schema.json");

        require(objectSchema["additionalProperties"] == false, "object schema should reject unknown root properties");
        require(objectSchema["properties"]["attach"]["$ref"] == "attach.schema.json", "object schema should reference attach schema");

        const nlohmann::json collisions =
            objectSchema["properties"]["collisions"];

        require(collisions.contains("oneOf"), "collisions should accept multiple shapes");
        require(collisions["oneOf"][0]["type"] == "string", "collisions should accept block reference string");
        require(collisions["oneOf"][1]["type"] == "object", "collisions should accept inline collider map");

        const nlohmann::json colliderValue =
            collisions["oneOf"][1]["additionalProperties"]["oneOf"];

        require(colliderValue[0]["type"] == "string", "collider value should accept reference string");
        require(colliderValue[1]["$ref"] == "collision.schema.json", "collider value should accept inline collision schema");
    }

    void testAttachSchemaClosedContract()
    {
        const nlohmann::json attachSchema =
            readSchema("attach.schema.json");

        require(attachSchema["type"] == "object", "attach should be an object");
        require(attachSchema["additionalProperties"] == false, "attach should reject unknown properties");

        const std::vector<std::string> properties = {
            "position",
            "born",
            "x",
            "y",
            "angle"
        };

        require(attachSchema["properties"].size() == properties.size(), "attach should expose only v0.3 properties");

        for (const std::string& property : properties)
        {
            require(attachSchema["properties"].contains(property), "attach property should exist: " + property);
            require(attachSchema["properties"][property]["type"] == "boolean", "attach property should be boolean: " + property);
        }
    }

    void testCreationSchemaDiscriminatesModes()
    {
        const nlohmann::json creationSchema =
            readSchema("creation.schema.json");

        require(creationSchema.contains("oneOf"), "creation schema should discriminate modes with oneOf");
        require(creationSchema["oneOf"].size() == 3, "creation schema should expose three mode branches");

        const nlohmann::json& individual =
            creationBranch(creationSchema, "individual");

        require(individual["additionalProperties"] == false, "individual creation should reject grid and iterator properties");
        require(!individual.contains("required"), "individual creation should allow omitted mode");
        require(!individual["properties"].contains("rules"), "individual creation should not allow rules");
        require(!individual["properties"].contains("pattern"), "individual creation should not allow pattern");

        const nlohmann::json& grid =
            creationBranch(creationSchema, "grid");

        require(grid["additionalProperties"] == false, "grid creation should reject unknown properties");
        require(grid["required"] == nlohmann::json::array({ "mode", "rules", "pattern" }), "grid creation should require mode rules and pattern");
        require(grid["properties"]["rules"]["additionalProperties"] == false, "grid rules should reject iterator rules");
        require(grid["properties"]["rules"]["required"] == nlohmann::json::array({ "rows", "columns", "cellWidth", "cellHeight" }), "grid rules should require grid dimensions");
        require(!grid["properties"]["rules"]["properties"].contains("concurrent"), "grid rules should not accept iterator concurrent");
        require(!grid["properties"]["rules"]["properties"].contains("repeat"), "grid rules should not accept iterator repeat");
        require(grid["properties"]["pattern"].contains("oneOf"), "grid should keep flat and row pattern forms");

        const nlohmann::json& iterator =
            creationBranch(creationSchema, "iterator");

        require(iterator["additionalProperties"] == false, "iterator creation should reject unknown properties");
        require(iterator["required"] == nlohmann::json::array({ "mode", "pattern" }), "iterator creation should require mode and pattern");
        require(iterator["properties"]["pattern"]["minItems"] == 1, "iterator pattern should reject empty arrays");
        require(iterator["properties"]["pattern"]["items"]["type"] == "string", "iterator pattern should be a flat string array");
        require(iterator["properties"]["rules"]["additionalProperties"] == false, "iterator rules should reject grid rules");
        require(iterator["properties"]["rules"]["properties"]["concurrent"]["minimum"] == 1, "iterator concurrent should be at least one");
        require(iterator["properties"]["rules"]["properties"]["repeat"]["type"] == "boolean", "iterator repeat should be boolean");
        require(!iterator["properties"]["rules"]["properties"].contains("rows"), "iterator rules should not accept grid rows");
        require(!iterator["properties"]["rules"]["properties"].contains("columns"), "iterator rules should not accept grid columns");
        require(!iterator["properties"]["rules"]["properties"].contains("cellWidth"), "iterator rules should not accept grid cellWidth");
        require(!iterator["properties"]["rules"]["properties"].contains("cellHeight"), "iterator rules should not accept grid cellHeight");
    }

    void testClosedObjectRootValidation()
    {
        const std::filesystem::path root =
            testRoot() / "closed_object_root";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=ClosedObject\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{}\n"
        );

        CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "empty object should remain valid");

        writeFile(
            root / "game" / "root.json",
            "{ \"visible\": true }\n"
        );

        result =
            compile(root / "game.flx");

        require(result.success, "object with public root property should compile");

        writeFile(
            root / "game" / "root.json",
            "{ \"visble\": true }\n"
        );

        result =
            compile(root / "game.flx");

        require(!result.success, "object with unknown root property should fail");
        require(diagnosticsContain(result.diagnostics, "unknown root property 'visble'"), "unknown root property diagnostic should name property");

        writeFile(
            root / "game" / "root.json",
            "{ \"color\": \"white\" }\n"
        );

        result =
            compile(root / "game.flx");

        require(!result.success, "legacy root property should still fail");
        require(diagnosticsContain(result.diagnostics, "property 'color' must be declared inside 'visual'"), "legacy root property should keep specific diagnostic");
    }

    void testClosedObjectRootValidationAfterLike()
    {
        const std::filesystem::path root =
            testRoot() / "closed_object_like";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "base");

        writeFile(
            root / "game.flx",
            "name=ClosedLike\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"like\": \"base/root\" }\n"
        );

        writeFile(
            root / "game" / "base" / "root.json",
            "{ \"visble\": true }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "object resolved through like should not keep unknown root properties");
        require(diagnosticsContain(result.diagnostics, "unknown root property 'visble'"), "like diagnostic should name unknown root property");
    }

    void testCollisionReferenceGranularitiesCompile()
    {
        const std::filesystem::path root =
            testRoot() / "collision_reference_granularities";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "blocks");

        writeFile(
            root / "game.flx",
            "name=CollisionReferences\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"collisions\": \"/blocks/collisions\",\n"
            "  \"children\": {\n"
            "    \"child\": {\n"
            "      \"collisions\": {\n"
            "        \"body\": \"/blocks/collider:body\"\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "}\n"
        );

        writeFile(
            root / "game" / "blocks" / "collisions.json",
            "{\n"
            "  \"body\": {\n"
            "    \"type\": \"box\",\n"
            "    \"size\": { \"width\": 8, \"height\": 8 }\n"
            "  }\n"
            "}\n"
        );

        writeFile(
            root / "game" / "blocks" / "collider.json",
            "{\n"
            "  \"body\": {\n"
            "    \"type\": \"ellipse\",\n"
            "    \"size\": { \"width\": 4, \"height\": 4 }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "collision block and collider references should compile");

        const ObjectDefinition& rootDefinition =
            rootObject(result.project);

        require(rootDefinition.collisions.count("body") == 1, "root should load referenced collisions block");
        require(rootDefinition.collisions.at("body").type == "box", "root collision block should keep collider data");

        const ObjectDefinition* child =
            result.project.resources.findObject(rootDefinition.childResources.at("child"));

        require(child != nullptr, "child should be compiled");
        require(child->collisions.count("body") == 1, "child should load referenced collider");
        require(child->collisions.at("body").type == "ellipse", "child collider reference should keep collider data");
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
        { "object schema closed contract", testObjectSchemaClosedContract },
        { "attach schema closed contract", testAttachSchemaClosedContract },
        { "creation schema discriminates modes", testCreationSchemaDiscriminatesModes },
        { "closed object root validation", testClosedObjectRootValidation },
        { "closed object root validation after like", testClosedObjectRootValidationAfterLike },
        { "collision reference granularities compile", testCollisionReferenceGranularitiesCompile },
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

