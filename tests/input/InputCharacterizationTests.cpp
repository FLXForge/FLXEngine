#include "../support/TestSupport.h"
#include "../../engine/debug/Logger.h"
#include "../../engine/input/InputSystem.h"
#include "../../engine/machine/MachineLoader.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    std::string captureInputWarnings(
        const InputChipDefinition& chip,
        const std::string& content
    )
    {
        Logger::setConsoleEnabled(true);
        Logger::setDebugEnabled(false);

        StreamCapture capture;

        InputSystem input;
        input.configure(chip);
        input.loadMappingContent("characterization.input", content);

        return capture.output.str();
    }

    MachineDefinition loadMachine(
        const std::string& name,
        const std::string& content
    )
    {
        const std::filesystem::path root =
            testRoot() / "input_characterization" / name;

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);

        const std::filesystem::path path =
            root / "machine.yml";

        writeFile(path, content);

        Logger::setConsoleEnabled(true);
        Logger::setDebugEnabled(false);

        return MachineLoader::load(path.generic_string());
    }

    void testDefaultMachineInputIsPermissive()
    {
        const MachineDefinition machine =
            MachineLoader::defaultMachine();

        require(machine.input.players == 16, "default input should expose 16 players");
        require(machine.input.direction == "analog", "default input direction should be analog");
        require(machine.input.playerButtons == 16, "default input should expose 16 player buttons");
        require(machine.input.systemButtons == 16, "default input should expose 16 system buttons");
        require(machine.input.pointer, "default input should expose pointer capability");
        require(machine.input.text, "default input should expose text capability");
    }

    void testInputChipLoadsCurrentShape()
    {
        const MachineDefinition machine =
            loadMachine(
                "current_shape",
                "machine:\n"
                "  input:\n"
                "    players: 2\n"
                "    direction: 8way\n"
                "    buttons:\n"
                "      player: 3\n"
                "      system: 4\n"
                "    pointer: false\n"
                "    text: false\n"
            );

        require(machine.input.players == 2, "input.players should load");
        require(machine.input.direction == "8way", "input.direction should load");
        require(machine.input.playerButtons == 3, "input.buttons.player should load");
        require(machine.input.systemButtons == 4, "input.buttons.system should load");
        require(!machine.input.pointer, "input.pointer should load");
        require(!machine.input.text, "input.text should load");
    }

    void testInputChipInvalidValuesFallBack()
    {
        Logger::setConsoleEnabled(true);
        Logger::setDebugEnabled(false);

        StreamCapture capture;

        const MachineDefinition machine =
            loadMachine(
                "invalid_values",
                "machine:\n"
                "  input:\n"
                "    players: 0\n"
                "    direction: analogic\n"
                "    buttons:\n"
                "      player: -1\n"
                "      system: -1\n"
                "    pointer: maybe\n"
                "    text: maybe\n"
            );

        const std::string output =
            capture.output.str();

        require(machine.input.players == 16, "invalid players should fall back");
        require(machine.input.direction == "analog", "invalid direction should fall back");
        require(machine.input.playerButtons == 16, "invalid player buttons should fall back");
        require(machine.input.systemButtons == 16, "invalid system buttons should fall back");
        require(machine.input.pointer, "invalid pointer should fall back");
        require(machine.input.text, "invalid text should fall back");
        require(output.find("input.players") != std::string::npos, "players warning should name field");
        require(output.find("input.direction") != std::string::npos, "direction warning should name field");
        require(output.find("input.buttons.player") != std::string::npos, "player buttons warning should name field");
        require(output.find("input.buttons.system") != std::string::npos, "system buttons warning should name field");
        require(output.find("input.pointer") != std::string::npos, "pointer warning should name field");
        require(output.find("input.text") != std::string::npos, "text warning should name field");
    }

    void testMappingAcceptsCurrentKeyboardGamepadAndCombinationSyntax()
    {
        InputChipDefinition chip;
        chip.players = 1;
        chip.direction = "4way";
        chip.playerButtons = 2;
        chip.systemButtons = 2;
        chip.pointer = false;
        chip.text = false;

        const std::string output =
            captureInputWarnings(
                chip,
                "system.buttons.0=KEY_ESCAPE,KEY_LEFT_ALT+KEY_Q\n"
                "system.buttons.1=KEY_ENTER,JOY1_START\n"
                "players.1.direction.up=KEY_W,KEY_UP,JOY1_UP\n"
                "players.1.direction.down=KEY_S,JOY1_DOWN\n"
                "players.1.direction.left=KEY_A,JOY1_LEFT\n"
                "players.1.direction.right=KEY_D,JOY1_RIGHT\n"
                "players.1.buttons.0=KEY_SPACE,JOY1_A\n"
                "players.1.buttons.1=KEY_LEFT_CONTROL+KEY_C\n"
            );

        require(output.empty(), "valid current mapping syntax should not warn");
    }

    void testMappingIsConstrainedByInputChip()
    {
        InputChipDefinition chip;
        chip.players = 1;
        chip.direction = "none";
        chip.playerButtons = 1;
        chip.systemButtons = 1;
        chip.pointer = false;
        chip.text = false;

        const std::string output =
            captureInputWarnings(
                chip,
                "players.2.buttons.0=KEY_SPACE\n"
                "players.1.buttons.1=KEY_SPACE\n"
                "system.buttons.1=KEY_ESCAPE\n"
                "players.1.direction.up=KEY_W\n"
                "players.1.buttons.0=KEY_UNKNOWN\n"
            );

        require(output.find("outside Input Chip players") != std::string::npos, "player count should constrain mapping");
        require(output.find("player button outside Input Chip limit") != std::string::npos, "player buttons should constrain mapping");
        require(output.find("system button outside Input Chip limit") != std::string::npos, "system buttons should constrain mapping");
        require(output.find("direction is none") != std::string::npos, "direction none should reject direction mapping");
        require(output.find("unknown input token 'KEY_UNKNOWN'") != std::string::npos, "unknown physical token should warn");
    }

    void testMappingDoesNotExposeGameplayNames()
    {
        InputChipDefinition chip;
        chip.players = 1;
        chip.direction = "4way";
        chip.playerButtons = 1;
        chip.systemButtons = 1;
        chip.pointer = false;
        chip.text = false;

        const std::string output =
            captureInputWarnings(
                chip,
                "players.1.buttons.fire=KEY_SPACE\n"
                "players.1.fire=KEY_SPACE\n"
                "fire=KEY_SPACE\n"
            );

        require(output.find("invalid player mapping") != std::string::npos, "gameplay button names should not be accepted as player mapping");
        require(output.find("unknown mapping key 'fire'") != std::string::npos, "gameplay action names should not be top-level mapping keys");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "default machine input is permissive", testDefaultMachineInputIsPermissive },
        { "input chip loads current shape", testInputChipLoadsCurrentShape },
        { "input chip invalid values fall back", testInputChipInvalidValuesFallBack },
        { "mapping accepts current keyboard gamepad and combination syntax", testMappingAcceptsCurrentKeyboardGamepadAndCombinationSyntax },
        { "mapping is constrained by input chip", testMappingIsConstrainedByInputChip },
        { "mapping does not expose gameplay names", testMappingDoesNotExposeGameplayNames }
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
