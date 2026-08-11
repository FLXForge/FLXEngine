#include "../support/TestSupport.h"
#include "../../engine/diagnostics/Diagnostics.h"
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    void testDiagnosticModelRequiresCodeMetadata()
    {
        Diagnostics diagnostics;
        diagnostics.error(
            DiagnosticCode::ResourceErrorUnclassified,
            "Missing resource",
            "game/root.json",
            "children.ship"
        );

        require(diagnostics.size() == 1, "diagnostic should be stored");
        require(diagnostics.hasErrors(), "error diagnostic should mark collection as failed");

        const Diagnostic& diagnostic =
            diagnostics.all().front();

        require(diagnostic.severity == DiagnosticSeverity::Error, "severity should be stored");
        require(diagnostic.code == DiagnosticCode::ResourceErrorUnclassified, "code should be stored");
        require(diagnostic.identifier == "ResourceErrorUnclassified", "identifier should come from code");
        require(diagnostic.file == "game/root.json", "file should be stored");
        require(diagnostic.field == "children.ship", "field should be stored");
        require(diagnostic.message == "Missing resource", "message should be stored");
        require(!diagnostic.range.has_value(), "range should be optional");
    }

    void testDiagnosticCodeMappingIsStableAndUnique()
    {
        const auto& codes =
            registeredDiagnosticCodes();

        require(codes.size() == 78, "registry should include generic codes and implemented project/resource/binary/runtime codes");

        std::set<std::string> textValues;
        std::set<std::string> identifiers;

        for (const DiagnosticCodeDefinition& definition : codes)
        {
            require(definition.text[0] != '\0', "diagnostic code text should not be empty");
            require(definition.identifier[0] != '\0', "diagnostic identifier should not be empty");
            require(textValues.insert(definition.text).second, "diagnostic code text should be unique");
            require(identifiers.insert(definition.identifier).second, "diagnostic identifier should be unique");
            require(
                diagnosticCodeText(definition.code) == std::string(definition.text),
                "lookup should return canonical code text"
            );
            require(
                diagnosticIdentifier(definition.code) == std::string(definition.identifier),
                "lookup should return canonical identifier"
            );
            require(
                diagnosticDomain(definition.code) == definition.domain,
                "lookup should return canonical domain"
            );
            require(
                diagnosticUsualSeverity(definition.code) == definition.usualSeverity,
                "lookup should return canonical usual severity"
            );
        }

        require(
            diagnosticCodeText(DiagnosticCode::CliErrorUnclassified) == std::string("FLX-CLI-00002"),
            "CLI generic error should use reserved 00002 code"
        );
        require(
            diagnosticIdentifier(DiagnosticCode::BuildDeprecatedFunctionality) == std::string("BuildDeprecatedFunctionality"),
            "BUILD deprecated identifier should be domain-qualified"
        );
        require(
            diagnosticCodeText(DiagnosticCode::InvalidProjectManifestSyntax) == std::string("FLX-PROJECT-00011"),
            "invalid manifest syntax should use concrete project code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::MissingReferencedResource) == std::string("FLX-RESOURCE-00010"),
            "missing referenced resource should use concrete resource code"
        );
        require(
            diagnosticIdentifier(DiagnosticCode::ReferencedScriptNotFound) == std::string("ReferencedScriptNotFound"),
            "referenced script code should expose stable identifier"
        );
        require(
            diagnosticCodeText(DiagnosticCode::CompiledProjectIdentityMismatch) == std::string("FLX-COMP-00014"),
            "compiled project identity mismatch should use concrete compiler code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::AutomaticInstantiationCycle) == std::string("FLX-COMP-00015"),
            "automatic instantiation cycle should use concrete compiler code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::CompiledProjectInvalidStateMachine) == std::string("FLX-COMP-00016"),
            "compiled project invalid state machine should use concrete compiler code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::InvalidStateMachineDeclaration) == std::string("FLX-RESOURCE-00015"),
            "invalid state machine declaration should use concrete resource code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::MissingStateMachineInitialState) == std::string("FLX-RESOURCE-00016"),
            "missing state machine initial state should use concrete resource code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::MissingStateMachineState) == std::string("FLX-RESOURCE-00017"),
            "missing state machine state should use concrete resource code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::InvalidStateTransitionTarget) == std::string("FLX-RESOURCE-00018"),
            "invalid state transition target should use concrete resource code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::DuplicateCompiledEntry) == std::string("FLX-BINARY-00019"),
            "duplicate compiled entry should use concrete binary code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::InvalidCompiledProjectValue) == std::string("FLX-BINARY-00020"),
            "invalid compiled project value should use concrete binary code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::EngineAlreadyRun) == std::string("FLX-RUNTIME-00010"),
            "engine already run should use concrete runtime code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::RuntimeWorldLoadFailed) == std::string("FLX-RUNTIME-00014"),
            "runtime world load failure should use concrete runtime code"
        );
        require(
            diagnosticCodeText(DiagnosticCode::RuntimeLoadSpawnLimitExceeded) == std::string("FLX-RUNTIME-00015"),
            "runtime load spawn limit should use concrete runtime code"
        );
    }

    void testAppendPreservesDiagnostics()
    {
        Diagnostics first;
        first.info(DiagnosticCode::CompInformationUnclassified, "Compiler ready");

        Diagnostics second;
        second.warning(DiagnosticCode::ProjectWarningUnclassified, "Using defaults");
        second.error(DiagnosticCode::BinaryErrorUnclassified, "Invalid binary");

        first.append(second);

        require(first.size() == 3, "append should add all diagnostics");
        require(first.hasErrors(), "appended errors should be visible");
        require(first.all()[0].identifier == "CompInformationUnclassified", "append should preserve original order");
        require(first.all()[1].identifier == "ProjectWarningUnclassified", "append should preserve appended order");
        require(first.all()[2].identifier == "BinaryErrorUnclassified", "append should preserve appended errors");
    }

    void testSourceRangeIsStoredOnlyWhenProvided()
    {
        Diagnostics diagnostics;

        SourceRange range;
        range.start.line = 3;
        range.start.column = 5;
        range.end.line = 3;
        range.end.column = 12;

        diagnostics.error(
            DiagnosticCode::ResourceErrorUnclassified,
            "Invalid field",
            "root.json",
            "shape.color",
            range
        );

        const Diagnostic& diagnostic =
            diagnostics.all().front();

        require(diagnostic.range.has_value(), "range should be present when provided");
        require(diagnostic.range->start.line == 3, "range start line should be stored");
        require(diagnostic.range->start.column == 5, "range start column should be stored");
        require(diagnostic.range->end.line == 3, "range end line should be stored");
        require(diagnostic.range->end.column == 12, "range end column should be stored");

        Diagnostics noRange;
        noRange.warning(DiagnosticCode::CliWarningUnclassified, "No range");
        require(!noRange.all().front().range.has_value(), "range should be absent by default");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "Diagnostics model requires code metadata", testDiagnosticModelRequiresCodeMetadata },
        { "Diagnostic code mapping is stable and unique", testDiagnosticCodeMappingIsStableAndUnique },
        { "Diagnostics append preserves diagnostics", testAppendPreservesDiagnostics },
        { "Diagnostics source range is optional", testSourceRangeIsStoredOnlyWhenProvided }
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
