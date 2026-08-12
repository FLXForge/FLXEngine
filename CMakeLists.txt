cmake_minimum_required(VERSION 3.20)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" FLX_VERSION)
string(STRIP "${FLX_VERSION}" FLX_VERSION)

if(NOT FLX_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain only a semantic version in X.Y.Z format")
endif()

string(REPLACE "." ";" FLX_VERSION_PARTS "${FLX_VERSION}")
list(GET FLX_VERSION_PARTS 0 FLX_VERSION_MAJOR)
list(GET FLX_VERSION_PARTS 1 FLX_VERSION_MINOR)
list(GET FLX_VERSION_PARTS 2 FLX_VERSION_PATCH)

project(FLX VERSION ${FLX_VERSION} LANGUAGES C CXX)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FlxVersion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/FlxVersion.h"
    @ONLY
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/EHsc>)
endif()

option(
    FLX_WINDOWS_SUBSYSTEM
    "Build flx as a Windows subsystem executable without a console window"
    OFF
)

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
    yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(yaml_cpp)

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
    GIT_TAG v0.10.1
    SOURCE_SUBDIR flx-no-upstream-cmake
)

FetchContent_MakeAvailable(quickjs)

# FLX embeds QuickJS as a scripting library. QuickJS-NG v0.10.1 declares
# qjs.exe, qjsc.exe and test helpers unconditionally from its CMakeLists.txt,
# so FLX only uses FetchContent to populate the source tree and defines the
# embedded qjs library target directly.
add_library(qjs
    ${quickjs_SOURCE_DIR}/cutils.c
    ${quickjs_SOURCE_DIR}/libregexp.c
    ${quickjs_SOURCE_DIR}/libunicode.c
    ${quickjs_SOURCE_DIR}/quickjs.c
    ${quickjs_SOURCE_DIR}/xsum.c
)

set_target_properties(
    qjs
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS ON
)

target_compile_definitions(qjs PRIVATE _GNU_SOURCE)

if(WIN32)
    target_compile_definitions(
        qjs
        PRIVATE
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0601
    )
endif()

if(MSVC)
    target_compile_options(qjs PRIVATE /experimental:c11atomics)
endif()

target_include_directories(
    qjs
    PUBLIC
    ${quickjs_SOURCE_DIR}
)

target_link_libraries(qjs PUBLIC ${CMAKE_DL_LIBS})

find_package(Threads)

if(Threads_FOUND)
    target_link_libraries(qjs PUBLIC Threads::Threads)
endif()

find_library(M_LIBRARIES m)

if(M_LIBRARIES)
    target_link_libraries(qjs PUBLIC ${M_LIBRARIES})
endif()

add_executable(flx
    FLXEngine.cpp

    engine/cli/CliApplication.h
    engine/cli/CliApplication.cpp
    engine/cli/CliArguments.h
    engine/cli/CliExitCode.h
    engine/cli/CliParser.h
    engine/cli/CliParser.cpp
    engine/cli/DiagnosticPrinter.h
    engine/cli/DiagnosticPrinter.cpp
    engine/cli/ProjectResolver.h
    engine/cli/ProjectResolver.cpp
    engine/cli/commands/CompileCommand.h
    engine/cli/commands/CompileCommand.cpp
    engine/cli/commands/HelpCommand.h
    engine/cli/commands/HelpCommand.cpp
    engine/cli/commands/RunCommand.h
    engine/cli/commands/RunCommand.cpp
    engine/cli/commands/RunCompiledCommand.h
    engine/cli/commands/RunCompiledCommand.cpp
    engine/cli/commands/ValidateCommand.h
    engine/cli/commands/ValidateCommand.cpp
    engine/cli/commands/VersionCommand.h
    engine/cli/commands/VersionCommand.cpp

    engine/core/Engine.h
    engine/core/Engine.cpp
    engine/core/EngineResult.h
    engine/core/RunOptions.h
    engine/compiler/CompiledProject.h
    engine/compiler/CompiledProjectBinary.h
    engine/compiler/CompiledProjectBinary.cpp
    engine/compiler/CompiledProjectValidator.h
    engine/compiler/CompiledProjectValidator.cpp
    engine/compiler/binary/BinaryException.h
    engine/compiler/binary/BinaryLimits.h
    engine/compiler/binary/BinaryReader.h
    engine/compiler/binary/BinaryReader.cpp
    engine/compiler/binary/BinaryWriter.h
    engine/compiler/binary/BinaryWriter.cpp
    engine/compiler/binary/CompiledProjectCodec.h
    engine/compiler/binary/CompiledProjectCodec.cpp
    engine/diagnostics/Diagnostics.h
    engine/diagnostics/Diagnostics.cpp
    engine/compiler/ResourceRegistry.h
    engine/compiler/ResourceRegistry.cpp
    engine/compiler/ProjectCompiler.h
    engine/compiler/ProjectCompiler.cpp

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
    engine/audio/MusicDefinition.h
    engine/audio/NoteTools.h
    engine/audio/NoteTools.cpp
    engine/audio/AudioSystem.h
    engine/audio/AudioSystem.cpp
    engine/collision/CollisionSystem.h
    engine/collision/CollisionSystem.cpp
    engine/input/InputSystem.h
    engine/input/InputSystem.cpp
    engine/tools/ColorParser.h
    engine/tools/ColorParser.cpp
    engine/tools/TextTools.h
    engine/tools/TextTools.cpp
    engine/graphics/FadeSystem.h
    engine/graphics/FadeSystem.cpp
    engine/machine/MachineDefinition.h
    engine/machine/MachineLoader.h
    engine/machine/MachineLoader.cpp
    engine/machine/VideoColorProcessor.h
    engine/machine/VideoColorProcessor.cpp
    engine/persistence/PersistenceSystem.h
    engine/persistence/PersistenceSystem.cpp
    engine/loading/JsonLoader.h
    engine/loading/JsonLoader.cpp
    engine/project/FlxContext.h
    engine/project/ProjectManifest.h
    engine/project/ProjectManifestLoader.h
    engine/project/ProjectManifestLoader.cpp
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

if(WIN32 AND FLX_WINDOWS_SUBSYSTEM)
    set_target_properties(
        flx
        PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

target_include_directories(
    flx
    PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${quickjs_SOURCE_DIR}
)

target_link_libraries(
    flx
    PRIVATE
    raylib
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    qjs
)

enable_testing()

get_target_property(FLX_TEST_SOURCES flx SOURCES)
list(REMOVE_ITEM FLX_TEST_SOURCES FLXEngine.cpp)

add_library(
    flx-test-engine
    STATIC
    ${FLX_TEST_SOURCES}
)

target_include_directories(
    flx-test-engine
    PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${quickjs_SOURCE_DIR}
)

target_link_libraries(
    flx-test-engine
    PUBLIC
    raylib
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    qjs
)

add_library(
    flx-test-support
    STATIC
    tests/support/TestSupport.h
    tests/support/TestSupport.cpp
)

target_compile_definitions(
    flx-test-support
    PUBLIC
    FLX_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    FLX_VERSION_TEXT="${FLX_VERSION}"
)

target_link_libraries(
    flx-test-support
    PUBLIC
    flx-test-engine
)

function(add_flx_test_suite target_name)
    add_executable(
        ${target_name}
        ${ARGN}
    )

    target_link_libraries(
        ${target_name}
        PRIVATE
        flx-test-support
    )

    add_test(
        NAME ${target_name}
        COMMAND ${target_name}
    )
endfunction()

add_flx_test_suite(flx-cli-parser-tests tests/cli/CliParserTests.cpp)
add_flx_test_suite(flx-cli-command-tests tests/cli/CliCommandTests.cpp)
add_flx_test_suite(flx-cli-project-resolver-tests tests/cli/ProjectResolverTests.cpp)
add_flx_test_suite(flx-cli-diagnostic-printer-tests tests/cli/DiagnosticPrinterTests.cpp)
add_flx_test_suite(flx-project-manifest-tests tests/project/ProjectManifestTests.cpp)
add_flx_test_suite(flx-diagnostics-tests tests/diagnostics/DiagnosticsTests.cpp)
add_flx_test_suite(flx-loading-reference-tests tests/loading/ReferenceResolutionTests.cpp)
add_flx_test_suite(flx-compiler-project-tests tests/compiler/ProjectCompilerTests.cpp)
add_flx_test_suite(flx-compiler-resource-graph-tests tests/compiler/ResourceGraphTests.cpp)
add_flx_test_suite(flx-compiled-binary-tests tests/compiled/CompiledProjectBinaryTests.cpp)
add_flx_test_suite(flx-runtime-compiled-tests tests/runtime/CompiledRuntimeTests.cpp)
add_flx_test_suite(flx-runtime-lifecycle-tests tests/runtime/RuntimeLifecycleTests.cpp)
add_flx_test_suite(flx-runtime-timer-tests tests/runtime/RuntimeTimerTests.cpp)
add_flx_test_suite(flx-engine-host-tests tests/runtime/EngineHostTests.cpp)
add_flx_test_suite(flx-example-smoke-tests tests/examples/ExampleSmokeTests.cpp)

add_test(
    NAME cli-version
    COMMAND $<TARGET_FILE:flx> --version
)

set_tests_properties(
    cli-version
    PROPERTIES
    PASS_REGULAR_EXPRESSION "^${FLX_VERSION}"
)

add_test(
    NAME cli-help
    COMMAND $<TARGET_FILE:flx> --help
)

set_tests_properties(
    cli-help
    PROPERTIES
    PASS_REGULAR_EXPRESSION "Usage:"
)

add_test(
    NAME cli-validate-pong
    COMMAND $<TARGET_FILE:flx> validate "${CMAKE_CURRENT_SOURCE_DIR}/examples/pong.flx"
)

add_test(
    NAME cli-invalid-command
    COMMAND
        ${CMAKE_COMMAND}
        -DFLX_CLI=$<TARGET_FILE:flx>
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunCliInvalidCommandTest.cmake"
)
