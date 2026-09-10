#include "../support/TestSupport.h"
#include "../../engine/debug/Logger.h"
#include "../../engine/input/InputMappingLoader.h"
#include "../../engine/input/InputSystem.h"
#include "../../engine/machine/MachineLoader.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <raylib.h>

#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
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

    InputMappingLoadResult validateMapping(
        const InputChipDefinition& chip,
        const std::string& content
    )
    {
        return InputMappingLoader::loadContent(
            "characterization.input",
            content,
            chip
        );
    }

    bool hasCode(
        const Diagnostics& diagnostics,
        DiagnosticCode code
    )
    {
        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            if (diagnostic.code == code)
            {
                return true;
            }
        }

        return false;
    }

    bool hasSeverity(
        const Diagnostics& diagnostics,
        DiagnosticCode code,
        DiagnosticSeverity severity
    )
    {
        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            if (diagnostic.code == code &&
                diagnostic.severity == severity)
            {
                return true;
            }
        }

        return false;
    }

    double localNumber(
        const RuntimeObject& object,
        const std::string& key
    )
    {
        const auto it =
            object.local.find(key);

        if (it == object.local.end())
        {
            return 0.0;
        }

        if (const auto* number = std::get_if<double>(&it->second))
        {
            return *number;
        }

        if (const auto* boolean = std::get_if<bool>(&it->second))
        {
            return *boolean ? 1.0 : 0.0;
        }

        return 0.0;
    }

    bool loadMappingInto(
        InputSystem& input,
        const InputChipDefinition& chip,
        const std::string& sourceName,
        const std::string& content
    )
    {
        const InputMappingLoadResult result =
            InputMappingLoader::loadContent(sourceName, content, chip);

        if (result.success)
        {
            input.setMapping(result.mapping);
        }

        return result.success;
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

    InputChipDefinition capacityChip(
        int playerButtons,
        int systemButtons = 2,
        int players = 1,
        int directions = 1
    )
    {
        InputChipDefinition chip;
        chip.systemButtons = systemButtons;
        chip.players = players;
        chip.playerButtons = playerButtons;
        chip.directions.clear();

        for (int index = 0; index < directions; ++index)
        {
            chip.directions.push_back(
                InputDirectionDefinition{ "4way", "last", 0.0f }
            );
        }

        return chip;
    }

    std::string fourButtonMapping()
    {
        return
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.down=KEY_S\n"
            "players.1.directions.0.left=KEY_A\n"
            "players.1.directions.0.right=KEY_D\n"
            "players.1.buttons.0=KEY_SPACE\n"
            "players.1.buttons.1=KEY_LEFT_CONTROL\n"
            "players.1.buttons.2=KEY_LEFT_SHIFT\n"
            "players.1.buttons.3=KEY_Z\n"
            "system.buttons.0=KEY_ENTER\n"
            "system.buttons.1=KEY_ESCAPE\n";
    }

    void testDefaultMachineInputMatchesDefaultMapping()
    {
        const MachineDefinition machine =
            MachineLoader::defaultMachine();

        require(machine.input.systemButtons == 2, "default input should expose 2 system buttons");
        require(machine.input.players == 1, "default input should expose 1 player");
        require(machine.input.playerButtons == 4, "default input should expose 4 player buttons");
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
        const InputMappingLoadResult result =
            validateMapping(
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

        require(result.success, "valid consolidated mapping syntax should validate");
    }

    void testMappingCapacityDifferencesAreWarnings()
    {
        const InputMappingLoadResult result =
            validateMapping(
                inputChip("2way"),
                "players.2.buttons.0=KEY_SPACE\n"
                "players.1.buttons.2=KEY_SPACE\n"
                "system.buttons.2=KEY_ESCAPE\n"
                "players.1.directions.1.down=KEY_A\n"
                "players.1.directions.0.positive=KEY_W\n"
                "players.1.buttons.0=KEY_UNKNOWN\n"
            );

        require(!result.success, "invalid mapping should fail validation");
        require(hasSeverity(result.diagnostics, DiagnosticCode::InputMappingPlayerOutOfRange, DiagnosticSeverity::Warning), "player count differences should be warnings");
        require(hasSeverity(result.diagnostics, DiagnosticCode::InputMappingButtonOutOfRange, DiagnosticSeverity::Warning), "button count differences should be warnings");
        require(hasSeverity(result.diagnostics, DiagnosticCode::InputMappingDirectionOutOfRange, DiagnosticSeverity::Warning), "direction count differences should be warnings");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingUnknownDirectionComponent), "positive/negative should not be public mapping components");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingUnknownPhysicalToken), "unknown physical token should fail");
    }

    void testMappingWithMorePlayerButtonsThanMachineSucceedsWithWarning()
    {
        const InputMappingLoadResult zeroButtons =
            validateMapping(capacityChip(0), fourButtonMapping());

        require(zeroButtons.success, "machine with zero player buttons should accept broader mapping");
        require(hasSeverity(zeroButtons.diagnostics, DiagnosticCode::InputMappingButtonOutOfRange, DiagnosticSeverity::Warning), "broader player button mapping should warn");

        const InputMappingLoadResult twoButtons =
            validateMapping(capacityChip(2), fourButtonMapping());

        require(twoButtons.success, "machine with two player buttons should accept four-button mapping");
        require(hasSeverity(twoButtons.diagnostics, DiagnosticCode::InputMappingButtonOutOfRange, DiagnosticSeverity::Warning), "four buttons over two-button machine should warn");
    }

    void testMachineWithMorePlayerButtonsThanMappingSucceedsWithWarning()
    {
        const InputMappingLoadResult result =
            validateMapping(capacityChip(10), fourButtonMapping());

        require(result.success, "machine with more player buttons should accept smaller mapping");
        require(hasSeverity(result.diagnostics, DiagnosticCode::InputMappingMachineControlUnmapped, DiagnosticSeverity::Warning), "unmapped machine player buttons should warn");
    }

    void testMatchingCoverageSucceedsWithoutCoverageWarnings()
    {
        const InputMappingLoadResult result =
            validateMapping(capacityChip(4), fourButtonMapping());

        require(result.success, "matching mapping and machine coverage should succeed");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingPlayerOutOfRange), "matching player coverage should not warn");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingButtonOutOfRange), "matching button coverage should not warn");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingDirectionOutOfRange), "matching direction coverage should not warn");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingMachineControlUnmapped), "matching coverage should not report unmapped machine controls");
    }

    void testDefaultMappingIsCompleteAndNotTrimmedByMachine()
    {
        const InputMappingLoadResult zeroButtons =
            InputMappingLoader::loadDefault(capacityChip(0));

        require(zeroButtons.success, "default mapping should load over zero-button machine");
        require(zeroButtons.content.find("players.1.buttons.3=KEY_Z,JOY1_Y") != std::string::npos, "default mapping should expose player button 3");
        require(zeroButtons.mapping.players.at(1).buttons.contains(3), "default mapping should preserve button 3");

        const InputMappingLoadResult tenButtons =
            InputMappingLoader::loadDefault(capacityChip(10));

        require(tenButtons.success, "default mapping should load over larger machine");
        require(hasSeverity(tenButtons.diagnostics, DiagnosticCode::InputMappingMachineControlUnmapped, DiagnosticSeverity::Warning), "larger machine should warn about unmapped default buttons");
    }

    void testDefaultMappingLoadsForDefaultMachine()
    {
        const MachineDefinition machine =
            MachineLoader::defaultMachine();

        const InputMappingLoadResult result =
            InputMappingLoader::loadDefault(machine.input);

        require(result.success, "default mapping should validate against default machine");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingMachineControlUnmapped), "default mapping should fully cover default machine controls");
        require(result.sourceName == "<default input mapping>", "default mapping should have a stable source name");
        require(result.content.find("players.1.directions.0.up=KEY_W") != std::string::npos, "default mapping should declare up");
        require(result.content.find("system.buttons.0=KEY_ENTER") != std::string::npos, "default mapping should declare system confirm");

        FakeInputProvider provider;
        InputSystem input;
        input.configure(machine.input);
        input.setPhysicalInputProvider(&provider);
        input.setMapping(result.mapping);

        provider.setKeys({ KEY_W, KEY_SPACE, KEY_ENTER });
        input.update(0.016f);

        require(input.playerDirectionPressed(1, 0, InputComponent::Up), "default mapping should expose player up");
        require(input.playerButtonPressed(1, 0), "default mapping should expose player button 0");
        require(input.systemButtonPressed(0), "default mapping should expose system button 0");
    }

    void testMappingValidationRejectsMalformedAndUnknownLines()
    {
        const InputMappingLoadResult result =
            validateMapping(
                inputChip("4way"),
                "players.1.buttons.0 KEY_SPACE\n"
                "unknown.root=KEY_SPACE\n"
                "players.1.fire=KEY_SPACE\n"
            );

        require(!result.success, "malformed and unknown mapping should fail");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingMalformedLine), "malformed line should be diagnostic");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingUnknownKey), "unknown root should be diagnostic");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingInvalidKey), "invalid player key should be diagnostic");
    }

    void testMappingValidationRejectsEmptyDuplicateAndInvalidMembers()
    {
        const InputMappingLoadResult result =
            validateMapping(
                inputChip("4way"),
                "players.1.buttons.0=\n"
                "players.1.buttons.0=KEY_SPACE\n"
                "players.1.buttons.1=KEY_SPACE,\n"
                "players.1.directions.0.up=KEY_W+KEY_UNKNOWN\n"
            );

        require(!result.success, "empty duplicate and invalid members should fail");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingEmptyBinding), "empty rhs or member should be diagnostic");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingDuplicateBinding), "duplicate key should be diagnostic");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingUnknownPhysicalToken), "invalid combination member should be diagnostic");
    }

    void testEmptyMappingFailsValidation()
    {
        const InputMappingLoadResult result =
            validateMapping(
                inputChip("4way"),
                "# only comments\n"
                "\n"
            );

        require(!result.success, "empty explicit mapping should fail");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingEmpty), "empty mapping should be diagnostic");
    }

    void testAlternativesAndCombinationsAreValidatedAsWholeActions()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way"));
        input.setPhysicalInputProvider(&provider);

        require(
            loadMappingInto(input, inputChip("4way"),
                "actions.input",
                "players.1.buttons.0=KEY_A,KEY_B\n"
                "players.1.buttons.1=KEY_LEFT_CONTROL+KEY_C\n"
            ),
            "valid alternatives and combinations should load"
        );

        provider.setKeys({ KEY_A });
        input.update(0.016f);
        require(input.playerButtonPressed(1, 0), "first alternative should activate");

        provider.setKeys({});
        input.update(0.016f);
        provider.setKeys({ KEY_LEFT_CONTROL });
        input.update(0.016f);
        require(!input.playerButtonDown(1, 1), "partial combination should not activate");

        provider.setKeys({ KEY_LEFT_CONTROL, KEY_C });
        input.update(0.016f);
        require(input.playerButtonPressed(1, 1), "complete combination should activate");
    }

    void testTwoWayAcceptsPublicFourDirectionVocabulary()
    {
        const InputChipDefinition chip =
            inputChip("2way", "last");

        FakeInputProvider provider;
        InputSystem input;
        input.configure(chip);
        input.setPhysicalInputProvider(&provider);
        loadMappingInto(input, chip,
            "twoway_public.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.directions.0.right=KEY_D\n"
            "players.1.directions.0.down=KEY_S\n"
            "players.1.directions.0.left=KEY_A\n"
        );

        provider.setKeys({ KEY_W });
        input.update(0.016f);
        require(input.playerDirectionDown(1, 0, InputComponent::Positive), "2way up should normalize to positive");

        provider.setKeys({ KEY_D });
        input.update(0.016f);
        require(input.playerDirectionDown(1, 0, InputComponent::Positive), "2way right should normalize to positive");

        provider.setKeys({ KEY_S });
        input.update(0.016f);
        require(input.playerDirectionDown(1, 0, InputComponent::Negative), "2way down should normalize to negative");

        provider.setKeys({ KEY_A });
        input.update(0.016f);
        require(input.playerDirectionDown(1, 0, InputComponent::Negative), "2way left should normalize to negative");
    }

    void testExplicitMissingMappingFailsCompilation()
    {
        const std::filesystem::path root =
            testRoot() / "input_characterization" / "missing_mapping";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=MissingInput\n"
            "path=game\n"
            "root=root\n"
            "input.mapping=missing.input\n"
        );

        writeFile(root / "game" / "root.json", "{}\n");

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "explicit missing input mapping should fail compilation");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingCouldNotBeOpened), "missing mapping should use input diagnostic");
    }

    void testExplicitMappingCoverageWarningsDoNotFailCompilation()
    {
        const std::filesystem::path root =
            testRoot() / "input_characterization" / "coverage_compile";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");
        std::filesystem::create_directories(root / "machines");

        writeFile(
            root / "game.flx",
            "name=CoverageInput\n"
            "path=game\n"
            "root=root\n"
            "machine=machines/input.machine.yml\n"
            "input.mapping=controls.input\n"
        );

        writeFile(root / "game" / "root.json", "{}\n");
        writeFile(root / "controls.input", fourButtonMapping());

        writeFile(
            root / "machines" / "input.machine.yml",
            "machine:\n"
            "  input:\n"
            "    system:\n"
            "      buttons: 4\n"
            "    players:\n"
            "      count: 1\n"
            "      controls:\n"
            "        buttons: 2\n"
            "        directions:\n"
            "          - type: 4way\n"
        );

        CompilationResult broader =
            compile(root / "game.flx");

        require(broader.success, "explicit broader mapping should compile with warnings");
        require(hasSeverity(broader.diagnostics, DiagnosticCode::InputMappingButtonOutOfRange, DiagnosticSeverity::Warning), "broader explicit mapping should warn");

        writeFile(
            root / "machines" / "input.machine.yml",
            "machine:\n"
            "  input:\n"
            "    system:\n"
            "      buttons: 4\n"
            "    players:\n"
            "      count: 1\n"
            "      controls:\n"
            "        buttons: 10\n"
            "        directions:\n"
            "          - type: 4way\n"
        );

        CompilationResult narrower =
            compile(root / "game.flx");

        require(narrower.success, "explicit narrower mapping should compile with warnings");
        require(hasSeverity(narrower.diagnostics, DiagnosticCode::InputMappingMachineControlUnmapped, DiagnosticSeverity::Warning), "narrower explicit mapping should warn");
    }

    void testButtonPressedDownReleasedAreLogicalAndIdempotent()
    {
        FakeInputProvider provider;
        InputSystem input;
        input.configure(inputChip("4way"));
        input.setPhysicalInputProvider(&provider);
        loadMappingInto(input, inputChip("4way"),
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
        const InputChipDefinition chip =
            inputChip("2way", "last");

        FakeInputProvider provider;
        InputSystem input;
        input.configure(chip);
        input.setPhysicalInputProvider(&provider);
        loadMappingInto(input, chip,
            "twoway.input",
            "players.1.directions.0.down=KEY_A\n"
            "players.1.directions.0.up=KEY_D\n"
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(neutralInput, inputChip("4way", "neutral", 10.0f),
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
        loadMappingInto(lastInput, inputChip("4way", "last", 10.0f),
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
        loadMappingInto(input, inputChip("4way"),
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
        loadMappingInto(input, chip,
            "high.input",
            "system.buttons.126=KEY_ESCAPE\n"
            "players.1.buttons.126=KEY_SPACE\n"
            "players.2.buttons.126=KEY_ENTER\n"
            "players.1.directions.0.right=KEY_D\n"
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
        const InputMappingLoadResult result =
            validateMapping(
                inputChip("4way"),
                "players.1.buttons.fire=KEY_SPACE\n"
                "players.1.fire=KEY_SPACE\n"
                "fire=KEY_SPACE\n"
            );

        require(!result.success, "gameplay names should not be accepted as input mapping");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingInvalidKey), "gameplay button names should not be accepted as player mapping");
        require(hasCode(result.diagnostics, DiagnosticCode::InputMappingUnknownKey), "gameplay action names should not be top-level mapping keys");
    }

    void testProjectWithoutMachineUsesDefaultDirectionFromPlayerSubject()
    {
        const std::filesystem::path root =
            testRoot() / "input_characterization" / "no_machine_player_subject";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "scripts");

        writeFile(
            root / "game.flx",
            "name=NoMachineInput\n"
            "path=game\n"
            "root=root\n"
            "input.mapping=controls.input\n"
        );

        writeFile(
            root / "controls.input",
            "players.1.directions.0.up=KEY_W\n"
            "players.1.buttons.0=KEY_SPACE\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"behavior\": { \"scripts\": [\"scripts/input-probe\"] } }\n"
        );

        writeFile(
            root / "game" / "scripts" / "input-probe.js",
            "const MOVE = direction(0);\n"
            "const FIRE = button(0);\n"
            "function action(root) {\n"
            "  if (input_pressed(player(1), MOVE, UP)) write_local(root, 'upPressed', 1);\n"
            "  if (input_down(player(1), MOVE, UP)) write_local(root, 'upDown', 1);\n"
            "  if (input_released(player(1), MOVE, UP)) write_local(root, 'upReleased', 1);\n"
            "  if (input_pressed(player(1), FIRE)) write_local(root, 'firePressed', 1);\n"
            "  if (input_pressed(root, MOVE, UP)) write_local(root, 'objectSubjectPressed', 1);\n"
            "}\n"
        );

        CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "project without machine should compile");
        require(result.project.context.machine.input.players >= 1, "default machine should provide player 1");
        require(!result.project.context.machine.input.directions.empty(), "default machine should provide direction 0");
        require(result.project.context.machine.input.directions[0].type == "4way", "default direction should be 4way");
        require(result.project.context.machine.input.directions[0].simultaneous == "last", "default direction policy should be last");
        require(result.project.context.machine.input.directions[0].buffer == 0.0f, "default direction buffer should be zero");

        FakeInputProvider provider;
        InputSystem input;
        input.configure(result.project.context.machine.input);
        input.setPhysicalInputProvider(&provider);
        input.setMapping(result.project.context.inputMapping);

        ScriptEngine scripts;
        scripts.setInputSystem(&input);
        scripts.setScreenScale(1);

        RuntimeWorld world;
        RuntimeLoadResult loadResult =
            world.load(result.project, scripts);

        require(loadResult.success, "runtime should load project without machine");

        RuntimeObject* runtimeRoot =
            world.findByName("root");

        require(runtimeRoot != nullptr, "runtime root should exist");

        provider.setKeys({ KEY_W, KEY_SPACE });
        input.update(0.016f);
        scripts.setFrameDelta(0.016f);
        world.update(scripts, 640.0f, 480.0f, 0.016f);

        require(localNumber(*runtimeRoot, "upPressed") == 1.0, "player(1) direction should report pressed without control.player");
        require(localNumber(*runtimeRoot, "upDown") == 1.0, "player(1) direction should report down without control.player");
        require(localNumber(*runtimeRoot, "firePressed") == 1.0, "player(1) button should still work under default machine");
        require(localNumber(*runtimeRoot, "objectSubjectPressed") == 0.0, "RuntimeObject subject should still require control.player");

        provider.setKeys({});
        input.update(0.016f);
        scripts.setFrameDelta(0.016f);
        world.update(scripts, 640.0f, 480.0f, 0.016f);

        require(localNumber(*runtimeRoot, "upReleased") == 1.0, "player(1) direction should report released without control.player");
    }

    void testProjectWithoutMachineAndInputHasNoInputCoverageWarnings()
    {
        const std::filesystem::path root =
            testRoot() / "input_characterization" / "zero_config_no_machine_no_input";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=ZeroConfig\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "zero-config project should compile");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingMachineControlUnmapped), "zero-config project should not report unmapped machine controls");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingPlayerOutOfRange), "zero-config project should not report player coverage mismatch");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingButtonOutOfRange), "zero-config project should not report button coverage mismatch");
        require(!hasCode(result.diagnostics, DiagnosticCode::InputMappingDirectionOutOfRange), "zero-config project should not report direction coverage mismatch");

        require(result.project.context.machine.input.systemButtons == 2, "zero-config machine should expose 2 system buttons");
        require(result.project.context.machine.input.players == 1, "zero-config machine should expose 1 player");
        require(result.project.context.machine.input.playerButtons == 4, "zero-config machine should expose 4 player buttons");
        require(result.project.context.machine.input.directions.size() == 1, "zero-config machine should expose one direction");
        require(result.project.context.machine.input.directions[0].type == "4way", "zero-config direction should be 4way");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "default machine input matches default mapping", testDefaultMachineInputMatchesDefaultMapping },
        { "input chip loads consolidated shape", testInputChipLoadsConsolidatedShape },
        { "unsupported capabilities produce diagnostics", testUnsupportedCapabilitiesProduceDiagnostics },
        { "mapping accepts keyboard gamepad combination and directions", testMappingAcceptsKeyboardGamepadCombinationAndDirections },
        { "mapping capacity differences are warnings", testMappingCapacityDifferencesAreWarnings },
        { "mapping with more player buttons than machine succeeds with warning", testMappingWithMorePlayerButtonsThanMachineSucceedsWithWarning },
        { "machine with more player buttons than mapping succeeds with warning", testMachineWithMorePlayerButtonsThanMappingSucceedsWithWarning },
        { "matching coverage succeeds without coverage warnings", testMatchingCoverageSucceedsWithoutCoverageWarnings },
        { "default mapping is complete and not trimmed by machine", testDefaultMappingIsCompleteAndNotTrimmedByMachine },
        { "default mapping loads for default machine", testDefaultMappingLoadsForDefaultMachine },
        { "mapping validation rejects malformed and unknown lines", testMappingValidationRejectsMalformedAndUnknownLines },
        { "mapping validation rejects empty duplicate and invalid members", testMappingValidationRejectsEmptyDuplicateAndInvalidMembers },
        { "empty mapping fails validation", testEmptyMappingFailsValidation },
        { "alternatives and combinations are validated as whole actions", testAlternativesAndCombinationsAreValidatedAsWholeActions },
        { "two way accepts public four direction vocabulary", testTwoWayAcceptsPublicFourDirectionVocabulary },
        { "explicit missing mapping fails compilation", testExplicitMissingMappingFailsCompilation },
        { "explicit mapping coverage warnings do not fail compilation", testExplicitMappingCoverageWarningsDoNotFailCompilation },
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
        { "mapping does not expose gameplay names", testMappingDoesNotExposeGameplayNames },
        { "project without machine uses default direction from player subject", testProjectWithoutMachineUsesDefaultDirectionFromPlayerSubject },
        { "project without machine and input has no input coverage warnings", testProjectWithoutMachineAndInputHasNoInputCoverageWarnings }
    };

    for (const auto& test : tests)
    {
        try
        {
            test.second();
            std::cout << "[PASS] " << test.first << std::endl;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";
            return 1;
        }
    }

    return 0;
}
