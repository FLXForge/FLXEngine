#include "../support/TestSupport.h"
#include "../../engine/debug/Logger.h"
#include "../../engine/input/InputSystem.h"
#include "../../engine/machine/MachineLoader.h"

#include <raylib.h>

#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    class FakeInputProvider : public PhysicalInputProvider
    {
    public:
        void setKeys(const std::set<int>& nextKeys)
        {
            previousKeys =
                currentKeys;

            currentKeys =
                nextKeys;
        }

        bool keyDown(int key) const override
        {
            return currentKeys.contains(key);
        }

        bool keyPressed(int key) const override
        {
            return currentKeys.contains(key) &&
                !previousKeys.contains(key);
        }

        bool gamepadButtonDown(int, int) const override
        {
            return false;
        }

        bool gamepadButtonPressed(int, int) const override
        {
            return false;
        }

    private:
        std::set<int> previousKeys;
        std::set<int> currentKeys;
    };

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
        const std::string& content,
        Diagnostics& diagnostics
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

        return MachineLoader::load(path.generic_string(), diagnostics);
    }

    InputChipDefinition inputChip(
        const std::string& type,
        const std::string& simultaneous = "last",
        float buffer = 0.0f
    )
    {
        InputChipDefinition chip;
        chip.systemButtons = 2;
        chip.players = 1;
        chip.playerButtons = 2;
        chip.directions = {
            InputDirectionDefinition{
                type,
                simultaneous,
                buffer
            }
        };
        return chip;
    }

    void testDefaultMachineInputIsDigitalAndPermissive()
    {
        const MachineDefinition machine =
            MachineLoader::defaultMachine();

        require(machine.input.systemButtons == 16, "default input should expose 16 system buttons");
        require(machine.input.players == 16, "default input should expose 16 players");
        require(machine.input.playerButtons == 16, "default input should expose 16 player buttons");
        require(machine.input.directions.size() == 1, "default input should expose one direction");
        require(machine.input.directions[0].type == "4way", "default direction should be 4way");
        require(machine.input.directions[0].simultaneous == "last", "default simultaneous should be last");
        require(machine.input.directions[0].buffer == 0.0f, "default direction buffer should be zero");
    }

    void testInputChipLoadsConsolidatedShape()
    {
        Diagnostics diagnostics;
        const MachineDefinition machine =
            loadMachine(
                "current_shape",
                "machine:\n"
                "  input:\n"
                "    system:\n"
                "      buttons: 4\n"
                "    players:\n"
                "      count: 2\n"
                "      controls:\n"
                "        directions:\n"
                "          - type: 2way\n"
                "            simultaneous: first\n"
                "            buffer: 0.25\n"
                "          - type: 4way\n"
                "            simultaneous: neutral\n"
                "            buffer: 0\n"
                "        buttons: 3\n",
                diagnostics
            );

        require(!diagnostics.hasErrors(), "valid consolidated input chip should not produce errors");
        require(machine.input.systemButtons == 4, "system buttons should load");
        require(machine.input.players == 2, "players count should load");
        require(machine.input.playerButtons == 3, "player buttons should load");
        require(machine.input.directions.size() == 2, "multiple directions should load");
        require(machine.input.directions[0].type == "2way", "first direction type should load");
        require(machine.input.directions[0].simultaneous == "first", "first simultaneous should load");
        require(machine.input.directions[0].buffer == 0.25f, "buffer should load");
        require(machine.input.directions[1].type == "4way", "second direction type should load");
        require(machine.input.directions[1].simultaneous == "neutral", "second simultaneous should load");
    }

    void testUnsupportedCapabilitiesProduceDiagnostics()
    {
        Diagnostics diagnostics;
        loadMachine(
            "unsupported",
            "machine:\n"
            "  input:\n"
            "    pointer: true\n"
            "    text: true\n"
            "    players:\n"
            "      count: 1\n"
            "      controls:\n"
            "        directions:\n"
            "          - type: analog\n"
            "          - type: 8way\n",
            diagnostics
        );

        require(diagnostics.hasErrors(), "unsupported input capabilities should fail validation");
        require(diagnostics.size() >= 4, "pointer text analog and 8way should be reported");
    }

    void testMappingAcceptsKeyboardGamepadCombinationAndDirections()
    {
        const std::string output =
            captureInputWarnings(
                inputChip("4way"),
                "system.buttons.0=KEY_ESCAPE,KEY_LEFT_ALT+KEY_Q\n"
                "system.buttons.1=KEY_ENTER,JOY1_START\n"
                "players.1.directions.0.up=KEY_W,KEY_UP,JOY1_UP\n"
                "players.1.directions.0.down=KEY_S,JOY1_DOWN\n"
                "players.1.directions.0.left=KEY_A,JOY1_LEFT\n"
                "players.1.directions.0.right=KEY_D,JOY1_RIGHT\n"
                "players.1.buttons.0=KEY_SPACE,JOY1_A\n"
                "players.1.buttons.1=KEY_LEFT_CONTROL+KEY_C\n"
            );

        require(output.empty(), "valid consolidated mapping syntax should not warn");
    }

    void testMappingIsConstrainedByInputChip()
    {
        const std::string output =
            captureInputWarnings(
                inputChip("2way"),
                "players.2.buttons.0=KEY_SPACE\n"
                "players.1.buttons.2=KEY_SPACE\n"
                "system.buttons.2=KEY_ESCAPE\n"
                "players.1.directions.1.negative=KEY_A\n"
                "players.1.directions.0.up=KEY_W\n"
                "players.1.buttons.0=KEY_UNKNOWN\n"
            );

        require(output.find("outside Input Chip players") != std::string::npos, "player count should constrain mapping");
        require(output.find("player button outside Input Chip limit") != std::string::npos, "player buttons should constrain mapping");
        require(output.find("system button outside Input Chip limit") != std::string::npos, "system buttons should constrain mapping");
        require(output.find("direction outside Input Chip limit") != std::string::npos, "direction count should constrain mapping");
        require(output.find("incompatible direction component") != std::string::npos, "direction type should constrain components");
        require(output.find("unknown input token 'KEY_UNKNOWN'") != std::string::npos, "unknown physical token should warn");
    }

    void testButtonPressedDownReleasedAreLogicalAndIdempotent()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way"));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "buttons.input",
            "players.1.buttons.0=KEY_SPACE\n"
            "system.buttons.0=KEY_ESCAPE\n"
        );

        provider.setKeys({});
        input.update(0.016f);

        provider.setKeys({ KEY_SPACE, KEY_ESCAPE });
        input.update(0.016f);

        require(input.playerButtonPressed(1, 0), "player button should be pressed on transition");
        require(input.playerButtonPressed(1, 0), "pressed should be idempotent in same frame");
        require(input.playerButtonDown(1, 0), "player button should be down");
        require(input.systemButtonPressed(0), "system button should be pressed on transition");
        require(input.systemButtonDown(0), "system button should be down");

        provider.setKeys({ KEY_SPACE, KEY_ESCAPE });
        input.update(0.016f);

        require(!input.playerButtonPressed(1, 0), "held button should not remain pressed");
        require(input.playerButtonDown(1, 0), "held button should remain down");

        provider.setKeys({});
        input.update(0.016f);

        require(input.playerButtonReleased(1, 0), "player button should be released on transition");
        require(input.systemButtonReleased(0), "system button should be released on transition");
    }

    void testFourWayLastResolvesNewestLogicalComponent()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "last"));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "direction.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Up), "up should be pressed first");
        require(input.playerDirectionDown(1, 0, InputComponent::Positive), "up projects to positive");

        provider.setKeys({ KEY_W, KEY_D });
        input.update(0.016f);

        require(input.playerDirectionReleased(1, 0, InputComponent::Up), "last policy should release up when right wins");
        require(input.playerDirectionPressed(1, 0, InputComponent::Right), "right should be pressed when it wins");
        require(!input.playerDirectionPressed(1, 0, InputComponent::Positive), "positive projection should not retrigger between up and right");
        require(input.playerDirectionDown(1, 0, InputComponent::Positive), "right also projects to positive");
    }

    void testFirstBufferPromotesPendingCandidate()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "first", 0.2f));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "first.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        provider.setKeys({ KEY_W, KEY_D });
        input.update(0.016f);

        require(input.playerDirectionDown(1, 0, InputComponent::Up), "first policy should keep dominant component");
        require(!input.playerDirectionPressed(1, 0, InputComponent::Right), "blocked candidate should not be logical yet");

        provider.setKeys({ KEY_D });
        input.update(0.05f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Right), "pending candidate should become logical before buffer expires");
    }

    void testFirstBufferExpiresCandidate()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "first", 0.01f));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "first_expire.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        provider.setKeys({ KEY_W, KEY_D });
        input.update(0.016f);

        provider.setKeys({ KEY_D });
        input.update(0.05f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Right), "active candidate should still win as first remaining input after buffer expiry");
    }

    void testMappingDoesNotExposeGameplayNames()
    {
        const std::string output =
            captureInputWarnings(
                inputChip("4way"),
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
        { "default machine input is digital and permissive", testDefaultMachineInputIsDigitalAndPermissive },
        { "input chip loads consolidated shape", testInputChipLoadsConsolidatedShape },
        { "unsupported capabilities produce diagnostics", testUnsupportedCapabilitiesProduceDiagnostics },
        { "mapping accepts keyboard gamepad combination and directions", testMappingAcceptsKeyboardGamepadCombinationAndDirections },
        { "mapping is constrained by input chip", testMappingIsConstrainedByInputChip },
        { "button pressed down released are logical and idempotent", testButtonPressedDownReleasedAreLogicalAndIdempotent },
        { "four way last resolves newest logical component", testFourWayLastResolvesNewestLogicalComponent },
        { "first buffer promotes pending candidate", testFirstBufferPromotesPendingCandidate },
        { "first buffer expires candidate", testFirstBufferExpiresCandidate },
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
