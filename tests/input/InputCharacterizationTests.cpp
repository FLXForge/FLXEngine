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

    void testTwoWayNativeComponents()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("2way", "last"));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "twoway.input",
            "players.1.directions.0.negative=KEY_A\n"
            "players.1.directions.0.positive=KEY_D\n"
        );

        provider.setKeys({});
        input.update(0.016f);

        provider.setKeys({ KEY_A });
        input.update(0.016f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Negative), "2way negative should press from neutral");
        require(input.playerDirectionDown(1, 0, InputComponent::Negative), "2way negative should be down");
        require(!input.playerDirectionDown(1, 0, InputComponent::Positive), "2way positive should not be down");
        require(!input.componentAllowed(0, InputComponent::Up), "2way should not accept up as a native component");
        require(!input.componentAllowed(0, InputComponent::Left), "2way should not accept left as a native component");

        provider.setKeys({ KEY_D });
        input.update(0.016f);

        require(input.playerDirectionReleased(1, 0, InputComponent::Negative), "2way negative should release when positive wins");
        require(input.playerDirectionPressed(1, 0, InputComponent::Positive), "2way positive should press after negative");
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

    void testFourWayLastProjectionTransitionsAcrossPolarity()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "last"));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "projection.input",
            "players.1.directions.0.right=KEY_D\n"
            "players.1.directions.0.down=KEY_S\n"
        );

        provider.setKeys({ KEY_D });
        input.update(0.016f);

        require(input.playerDirectionDown(1, 0, InputComponent::Positive), "right should project to positive");

        provider.setKeys({ KEY_D, KEY_S });
        input.update(0.016f);

        require(input.playerDirectionReleased(1, 0, InputComponent::Right), "right should release when down wins");
        require(input.playerDirectionPressed(1, 0, InputComponent::Down), "down should press when it wins");
        require(input.playerDirectionReleased(1, 0, InputComponent::Positive), "positive projection should release on right to down");
        require(input.playerDirectionPressed(1, 0, InputComponent::Negative), "negative projection should press on right to down");
    }

    void testFourWayNeutralPolicy()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "neutral"));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "neutral.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        require(input.playerDirectionDown(1, 0, InputComponent::Up), "single active component should win under neutral policy");

        provider.setKeys({ KEY_W, KEY_D });
        input.update(0.016f);

        require(input.playerDirectionReleased(1, 0, InputComponent::Up), "neutral policy should release current when conflict appears");
        require(!input.playerDirectionDown(1, 0, InputComponent::Up), "neutral policy should not keep up during conflict");
        require(!input.playerDirectionDown(1, 0, InputComponent::Right), "neutral policy should not choose right during conflict");

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Up), "neutral policy should restore single remaining component");
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

    void testFirstBufferKeepsOnlyLatestCandidate()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "first", 0.2f));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "first_replace.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
            "players.1.directions.0.down=KEY_S\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        provider.setKeys({ KEY_W, KEY_D });
        input.update(0.016f);

        provider.setKeys({ KEY_W, KEY_D, KEY_S });
        input.update(0.016f);

        provider.setKeys({ KEY_D, KEY_S });
        input.update(0.016f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Down), "latest buffered candidate should replace previous candidate");
        require(!input.playerDirectionDown(1, 0, InputComponent::Right), "older buffered candidate should not win");
    }

    void testFirstWithZeroBufferDoesNotHoldPendingCandidate()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "first", 0.0f));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "first_zero.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);

        provider.setKeys({ KEY_W, KEY_D });
        input.update(0.016f);

        require(input.playerDirectionDown(1, 0, InputComponent::Up), "first with zero buffer should keep current component");
        require(!input.playerDirectionPressed(1, 0, InputComponent::Right), "first with zero buffer should not press blocked component");

        provider.setKeys({ KEY_D });
        input.update(0.016f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Right), "first with zero buffer should choose first remaining active component");
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

    void testBufferDoesNotChangeNeutralOrLastPolicy()
    {
        FakeInputProvider neutralProvider;
        InputSystem neutralInput;
        neutralInput.configure(inputChip("4way", "neutral", 10.0f));
        neutralInput.setPhysicalInputProvider(&neutralProvider);
        neutralInput.loadMappingContent(
            "neutral_buffer.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        neutralProvider.setKeys({ KEY_W });
        neutralInput.update(0.016f);
        neutralProvider.setKeys({ KEY_W, KEY_D });
        neutralInput.update(0.016f);

        require(!neutralInput.playerDirectionDown(1, 0, InputComponent::Up), "buffer should not override neutral conflict resolution");
        require(!neutralInput.playerDirectionDown(1, 0, InputComponent::Right), "buffer should not choose a neutral conflict candidate");

        FakeInputProvider lastProvider;
        InputSystem lastInput;
        lastInput.configure(inputChip("4way", "last", 10.0f));
        lastInput.setPhysicalInputProvider(&lastProvider);
        lastInput.loadMappingContent(
            "last_buffer.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
        );

        lastProvider.setKeys({ KEY_W });
        lastInput.update(0.016f);
        lastProvider.setKeys({ KEY_W, KEY_D });
        lastInput.update(0.016f);

        require(lastInput.playerDirectionDown(1, 0, InputComponent::Right), "buffer should not override last conflict resolution");
    }

    void testMultiplePhysicalSourcesKeepLogicalButtonAndDirectionActive()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way", "last"));
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "multi_source.input",
            "players.1.buttons.0=KEY_SPACE,KEY_ENTER\n"
            "players.1.directions.0.up=KEY_W,KEY_UP\n"
        );

        provider.setKeys({ KEY_SPACE, KEY_W });
        input.update(0.016f);

        require(input.playerButtonPressed(1, 0), "button should press from first source");
        require(input.playerDirectionPressed(1, 0, InputComponent::Up), "direction should press from first source");

        provider.setKeys({ KEY_ENTER, KEY_UP });
        input.update(0.016f);

        require(input.playerButtonDown(1, 0), "button should stay down while another source is active");
        require(!input.playerButtonReleased(1, 0), "button should not release while another source remains active");
        require(input.playerDirectionDown(1, 0, InputComponent::Up), "direction should stay down while another source is active");
        require(!input.playerDirectionReleased(1, 0, InputComponent::Up), "direction should not release while another source remains active");
    }

    void testHighIndexesPlayersSystemAndDirectionsAreIndependent()
    {
        InputChipDefinition chip;
        chip.systemButtons = 128;
        chip.players = 2;
        chip.playerButtons = 128;
        chip.directions = {
            InputDirectionDefinition{ "2way", "last", 0.0f },
            InputDirectionDefinition{ "4way", "last", 0.0f }
        };

        FakeInputProvider provider;
        InputSystem input;
        input.configure(chip);
        input.setPhysicalInputProvider(&provider);
        input.loadMappingContent(
            "high.input",
            "system.buttons.126=KEY_ESCAPE\n"
            "players.1.buttons.126=KEY_SPACE\n"
            "players.2.buttons.126=KEY_ENTER\n"
            "players.1.directions.0.positive=KEY_D\n"
            "players.1.directions.1.up=KEY_W\n"
            "players.2.directions.1.down=KEY_S\n"
        );

        provider.setKeys({ KEY_ESCAPE, KEY_SPACE, KEY_D, KEY_W });
        input.update(0.016f);

        require(input.systemButtonPressed(126), "high system button index should be representable");
        require(input.playerButtonPressed(1, 126), "high player button index should be representable");
        require(!input.playerButtonPressed(2, 126), "players should be isolated");
        require(input.playerDirectionDown(1, 0, InputComponent::Positive), "direction 0 should be independent");
        require(input.playerDirectionDown(1, 1, InputComponent::Up), "direction 1 should be independent");
        require(!input.playerDirectionDown(2, 1, InputComponent::Down), "player directions should be isolated");
    }

    void testInvalidQueriesReturnFalse()
    {
        InputSystem input;
        input.configure(inputChip("2way", "last"));

        require(!input.validPlayer(0), "player indexes should start at one");
        require(!input.validPlayer(2), "players above chip count should be invalid");
        require(!input.validPlayerButton(-1), "negative player button should be invalid");
        require(!input.validSystemButton(-1), "negative system button should be invalid");
        require(!input.validDirection(-1), "negative direction should be invalid");
        require(!input.validDirection(1), "direction outside chip should be invalid");
        require(!input.playerButtonDown(2, 0), "invalid player button query should return false");
        require(!input.playerDirectionDown(1, 1, InputComponent::Positive), "invalid direction query should return false");
        require(!input.componentAllowed(0, InputComponent::Up), "incompatible component should be rejected");
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
        { "two way native components", testTwoWayNativeComponents },
        { "four way last resolves newest logical component", testFourWayLastResolvesNewestLogicalComponent },
        { "four way last projection transitions across polarity", testFourWayLastProjectionTransitionsAcrossPolarity },
        { "four way neutral policy", testFourWayNeutralPolicy },
        { "first buffer promotes pending candidate", testFirstBufferPromotesPendingCandidate },
        { "first buffer keeps only latest candidate", testFirstBufferKeepsOnlyLatestCandidate },
        { "first with zero buffer does not hold pending candidate", testFirstWithZeroBufferDoesNotHoldPendingCandidate },
        { "first buffer expires candidate", testFirstBufferExpiresCandidate },
        { "buffer does not change neutral or last policy", testBufferDoesNotChangeNeutralOrLastPolicy },
        { "multiple physical sources keep logical button and direction active", testMultiplePhysicalSourcesKeepLogicalButtonAndDirectionActive },
        { "high indexes players system and directions are independent", testHighIndexesPlayersSystemAndDirectionsAreIndependent },
        { "invalid queries return false", testInvalidQueriesReturnFalse },
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
