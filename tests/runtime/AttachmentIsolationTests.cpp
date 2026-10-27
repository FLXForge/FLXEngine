#include "../support/TestSupport.h"
#include "../../engine/runtime/AttachmentRuntimeState.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"
#include <iostream>

using namespace flx::test;
int main()
{
    try
    {
        CompiledProject project;
        project.rootId = "root";
        ObjectDefinition root;
        root.id = "root";
        root.origin = { 10, 20 };
        root.hasOrigin = true;
        root.childResources["child"] = "child";
        ObjectDefinition child;
        child.id = "child";
        child.offset = { 3, 4 };
        child.hasOffset = true;
        child.attachOnCreate = true;
        child.attachFollowX = child.attachFollowY = true;
        require(project.resources.addObject("root", root), "root registration");
        require(project.resources.addObject("child", child), "child registration");
        RuntimeWorld first, second;
        ScriptEngine firstScripts, secondScripts;
        require(first.load(project, firstScripts).success, "first load");
        auto* instance = first.findByName("child");
        require(instance != nullptr, "attached child exists");
        const auto id = instance->runtimeId;
        AttachmentRuntimeState::setOffset(*instance, { 7, 8 });
        require(second.load(project, secondScripts).success, "second load");
        require(second.findByName("child")->runtimeId == id, "fixture reuses runtime identity");
        first.update(firstScripts, 0);
        require(instance->position.x == 17 && instance->position.y == 28, "another world cannot clear live offsets");
        second.update(secondScripts, 0);
        require(second.findByName("child")->position.x == 13, "new world starts with declared offset");
        require(first.load(project, firstScripts).success, "world reload");
        first.update(firstScripts, 0);
        instance = first.findByName("child");
        require(instance->position.x == 13 && !instance->attachmentOffset, "reload has no residual attachment state");
        require(instance->originalOffset.x == 3, "original declared offset is unchanged");
        std::cout << "[PASS] attachment ownership and reload isolation\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
