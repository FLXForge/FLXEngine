#include "../support/TestSupport.h"
#include "../../engine/cli/DiagnosticPrinter.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

    void testDiagnosticPrinterTextDiagnostics()
    {
        Diagnostics diagnostics;
        diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "Invalid child",
            "game/root.json",
            "children.ship"
        );

        std::ostringstream output;
        std::ostringstream error;

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            CliOutputFormat::Text,
            "validate",
            CliExitCode::CompilationError,
            false,
            output,
            error
        );

        require(output.str().empty(), "text diagnostics should not write to stdout");
        require(
            error.str() == "error FLX-CLI-00002 CliErrorUnclassified: game/root.json [children.ship]: Invalid child\n",
            "text diagnostics should include severity, file and field on stderr"
        );

        Diagnostics info;
        info.info(DiagnosticCode::CliInformationUnclassified, "Plain diagnostic");

        output.str("");
        output.clear();
        error.str("");
        error.clear();

        DiagnosticPrinter::printDiagnostics(
            info,
            CliOutputFormat::Text,
            "validate",
            CliExitCode::Success,
            true,
            output,
            error
        );

        require(output.str().empty(), "text info diagnostics should not write to stdout");
        require(
            error.str() == "info FLX-CLI-00000 CliInformationUnclassified: Plain diagnostic\n",
            "text diagnostics without file should still be written to stderr"
        );
    }

    void testDiagnosticPrinterTextResultWithWarning()
    {
        Diagnostics diagnostics;
        diagnostics.warning(
            DiagnosticCode::CliWarningUnclassified,
            "Machine uses defaults",
            "project.flx",
            "machine"
        );

        std::ostringstream output;
        std::ostringstream error;

        DiagnosticPrinter::printResult(
            "compile",
            "Compiled project written: game.flxc",
            CliOutputFormat::Text,
            output,
            error,
            diagnostics
        );

        require(
            output.str() == "Compiled project written: game.flxc\n",
            "text result message should be written to stdout"
        );
        require(
            error.str() == "warning FLX-CLI-00001 CliWarningUnclassified: project.flx [machine]: Machine uses defaults\n",
            "text result diagnostics should be written to stderr"
        );
    }

    void testDiagnosticPrinterJsonSuccessAndFailure()
    {
        Diagnostics successDiagnostics;
        successDiagnostics.warning(DiagnosticCode::CliWarningUnclassified, "Careful");

        std::ostringstream output;
        std::ostringstream error;

        DiagnosticPrinter::printResult(
            "validate",
            "Project is valid: game.flx",
            CliOutputFormat::Json,
            output,
            error,
            successDiagnostics
        );

        require(error.str().empty(), "json result should not write to stderr");

        nlohmann::json payload =
            nlohmann::json::parse(output.str());

        require(payload["success"] == true, "json result should report success");
        require(payload["command"] == "validate", "json result should include command");
        require(payload["exitCode"] == 0, "json result should include exit code");
        require(payload["message"] == "Project is valid: game.flx", "json result should include message");
        require(payload["diagnostics"].size() == 1, "json result should include diagnostics");
        require(payload["diagnostics"][0]["severity"] == "warning", "json result should include diagnostic severity");
        require(payload["diagnostics"][0]["code"] == "FLX-CLI-00001", "json result should include diagnostic code");
        require(payload["diagnostics"][0]["identifier"] == "CliWarningUnclassified", "json result should include diagnostic identifier");

        Diagnostics failureDiagnostics;
        failureDiagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "Missing root",
            "game.flx",
            "root"
        );

        output.str("");
        output.clear();
        error.str("");
        error.clear();

        DiagnosticPrinter::printDiagnostics(
            failureDiagnostics,
            CliOutputFormat::Json,
            "compile",
            CliExitCode::CompilationError,
            false,
            output,
            error
        );

        require(error.str().empty(), "json diagnostics should not write to stderr");

        payload =
            nlohmann::json::parse(output.str());

        require(payload["success"] == false, "json diagnostics should report failure");
        require(payload["command"] == "compile", "json diagnostics should include command");
        require(
            payload["exitCode"] == static_cast<int>(CliExitCode::CompilationError),
            "json diagnostics should include exit code"
        );
        require(payload.find("message") == payload.end(), "json diagnostics should not include result message");
        require(payload["diagnostics"].size() == 1, "json diagnostics should include errors");
        require(payload["diagnostics"][0]["file"] == "game.flx", "json diagnostics should include file");
        require(payload["diagnostics"][0]["field"] == "root", "json diagnostics should include field");
    }

    void testDiagnosticPrinterJsonEscapingAndMultipleDiagnostics()
    {
        const std::string escapedText =
            std::string("quotes \" slash / backslash \\ newline \n carriage \r tab \t backspace ") +
            std::string(1, '\b') +
            " formfeed " +
            std::string(1, '\f') +
            " control " +
            std::string(1, static_cast<char>(1));

        const std::string escapedFile =
            std::string("file\"\\/") +
            "\n\r\t" +
            std::string(1, '\b') +
            std::string(1, '\f') +
            std::string(1, static_cast<char>(1)) +
            ".json";

        Diagnostics diagnostics;
        diagnostics.info(
            DiagnosticCode::CliInformationUnclassified,
            escapedText,
            escapedFile,
            "field"
        );
        const std::string utf8Text =
            "Texto UTF-8: español, acento á, universo 宇宙";

        diagnostics.warning(DiagnosticCode::CliWarningUnclassified, utf8Text);

        std::ostringstream output;
        std::ostringstream error;

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            CliOutputFormat::Json,
            "validate",
            CliExitCode::Success,
            true,
            output,
            error
        );

        require(error.str().empty(), "json escaping should not write to stderr");

        nlohmann::json payload =
            nlohmann::json::parse(output.str());

        require(payload["diagnostics"].size() == 2, "json should include multiple diagnostics");
        require(
            payload["diagnostics"][0]["message"].get<std::string>() == escapedText,
            "json should preserve escaped control characters"
        );
        require(
            payload["diagnostics"][0]["file"].get<std::string>() == escapedFile,
            "json should preserve escaped file characters"
        );
        require(
            payload["diagnostics"][1]["message"].get<std::string>() == utf8Text,
            "json should preserve UTF-8 text"
        );
    }

    void testDiagnosticPrinterJsonRange()
    {
        SourceRange range;
        range.start.line = 2;
        range.start.column = 4;
        range.end.line = 2;
        range.end.column = 11;

        Diagnostics diagnostics;
        diagnostics.error(
            DiagnosticCode::ResourceErrorUnclassified,
            "Invalid value",
            "root.json",
            "shape.color",
            range
        );

        std::ostringstream output;
        std::ostringstream error;

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            CliOutputFormat::Json,
            "validate",
            CliExitCode::CompilationError,
            false,
            output,
            error
        );

        require(error.str().empty(), "json range diagnostics should not write to stderr");

        const nlohmann::json payload =
            nlohmann::json::parse(output.str());

        const nlohmann::json diagnostic =
            payload["diagnostics"][0];

        require(diagnostic.contains("code"), "json range diagnostic should include code field");
        require(diagnostic["code"].get<std::string>() == diagnosticCodeText(DiagnosticCode::ResourceErrorUnclassified), "json range diagnostic should include stable code value");
        require(
            diagnostic["identifier"].get<std::string>() == "ResourceErrorUnclassified",
            "json range diagnostic should include identifier; actual=" +
            diagnostic["identifier"].get<std::string>()
        );
        require(diagnostic.contains("range"), "json diagnostic should include range when present");
        require(diagnostic["range"]["startLine"] == 2, "json range should include start line");
        require(diagnostic["range"]["startColumn"] == 4, "json range should include start column");
        require(diagnostic["range"]["endLine"] == 2, "json range should include end line");
        require(diagnostic["range"]["endColumn"] == 11, "json range should include end column");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "DiagnosticPrinter text diagnostics", testDiagnosticPrinterTextDiagnostics },

        { "DiagnosticPrinter text result with warning", testDiagnosticPrinterTextResultWithWarning },

        { "DiagnosticPrinter JSON success and failure", testDiagnosticPrinterJsonSuccessAndFailure },

        { "DiagnosticPrinter JSON escaping and multiple diagnostics", testDiagnosticPrinterJsonEscapingAndMultipleDiagnostics },

        { "DiagnosticPrinter JSON range", testDiagnosticPrinterJsonRange }

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

