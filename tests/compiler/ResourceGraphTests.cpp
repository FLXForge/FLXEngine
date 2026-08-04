#include "../support/TestSupport.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

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
        require(result.project.rootDefinition.childResources.count("ship") == 1, "auto child should have resource id");
        require(result.project.rootDefinition.childResources.count("laser") == 1, "manual child should have resource id");
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
        require(result.project.rootDefinition.childResources.count("brick") == 1, "grid child should have resource id");
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
            "{ \"children\": { \"brick\": { \"like\": \"objects/brick\", \"role\": \"strong\" } } }\n"
        );

        writeFile(
            root / "game" / "objects" / "brick.json",
            "{ \"group\": \"brick\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 4 } } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "like graph should compile");
        const std::string childId =
            result.project.rootDefinition.childResources.at("brick");
        const ObjectDefinition* child =
            result.project.resources.findObject(childId);

        require(child != nullptr, "liked child should be registered");
        require(child->group == "brick", "liked child should inherit group");
        require(child->role == "strong", "liked child should keep override");
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
        require(result.project.rootDefinition.shapeType == "circle", "referenced shape should resolve");
        require(result.project.rootDefinition.radius == 3.0f, "referenced radius should resolve");
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

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "graph with auto and manual children", testGraphWithAutoAndManualChildren },

        { "graph with grid children", testGraphWithGridChildren },

        { "graph with like", testGraphWithLike },

        { "valid FLX reference", testValidFlxReference },

        { "invalid FLX reference", testInvalidFlxReference }

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

