cmake_minimum_required(VERSION 3.20)

project(FLXEngine VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(json)

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
    GIT_TAG v0.10.1
)

FetchContent_MakeAvailable(quickjs)

add_executable(FlxEngine
    FLXEngine.cpp
    FLXEngine.h

    engine/core/Engine.h
    engine/core/Engine.cpp

    engine/runtime/RuntimeObject.h
    engine/runtime/RuntimeObject.cpp 
    engine/runtime/ObjectDefinition.h
    engine/runtime/RuntimeObjectBuilder.h
    engine/runtime/RuntimeObjectBuilder.cpp
    engine/runtime/RuntimeWorld.h
    engine/runtime/RuntimeWorld.cpp
    engine/runtime/RayCastResult.h
    engine/runtime/RuntimeHelpers.h
    engine/runtime/RuntimeHelpers.cpp
    engine/runtime/RuntimeConstants.h 
    engine/audio/SoundDefinition.h
    engine/audio/AudioSystem.h
    engine/audio/AudioSystem.cpp
    engine/collision/CollisionSystem.h
    engine/collision/CollisionSystem.cpp
    engine/tools/ColorParser.h
    engine/tools/ColorParser.cpp
    engine/tools/TextTools.h
    engine/tools/TextTools.cpp
    engine/graphics/FadeSystem.h
    engine/graphics/FadeSystem.cpp
    engine/loading/JsonLoader.h
    engine/loading/JsonLoader.cpp
    engine/project/FlxContext.h
    engine/project/FlxContextBuilder.h
    engine/project/FlxContextBuilder.cpp
    engine/scripting/ScriptEngine.h
    engine/scripting/ScriptEngine.cpp 
    engine/scripting/ScriptBindings.h
    engine/scripting/ScriptBindings.cpp 
    engine/scripting/bindings/BindingHelpers.h
    engine/scripting/bindings/AudioBindings.h
    engine/scripting/bindings/AudioBindings.cpp
    engine/scripting/bindings/CoreBindings.h
    engine/scripting/bindings/CoreBindings.cpp
    engine/scripting/bindings/InputBindings.h
    engine/scripting/bindings/InputBindings.cpp
    engine/scripting/bindings/DrawBindings.h
    engine/scripting/bindings/DrawBindings.cpp
    engine/scripting/bindings/MotionBindings.h
    engine/scripting/bindings/MotionBindings.cpp
    engine/scripting/bindings/SpawnBindings.h
    engine/scripting/bindings/SpawnBindings.cpp
    engine/scripting/bindings/StateBindings.h
    engine/scripting/bindings/StateBindings.cpp
    engine/scripting/bindings/TimerBindings.h
    engine/scripting/bindings/TimerBindings.cpp
    engine/debug/Logger.cpp
    engine/debug/Logger.h
    engine/scripting/ScriptModule.h)

target_include_directories(
    FlxEngine
    PRIVATE
    ${quickjs_SOURCE_DIR}
)

target_link_libraries(
    FlxEngine
    PRIVATE
    raylib
    nlohmann_json::nlohmann_json
    qjs
)
