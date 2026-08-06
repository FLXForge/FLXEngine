#include "CompiledProjectBinary.h"
#include "CompiledProjectValidator.h"
#include "binary/BinaryException.h"
#include "binary/BinaryLimits.h"
#include "binary/BinaryReader.h"
#include "binary/BinaryWriter.h"
#include "binary/CompiledProjectCodec.h"

#include <filesystem>
#include <fstream>

namespace
{
    void removeFileIfExists(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path temporaryPathFor(const std::filesystem::path& outputPath)
    {
        return outputPath.string() + ".tmp";
    }

    std::filesystem::path backupPathFor(const std::filesystem::path& outputPath)
    {
        return outputPath.string() + ".bak";
    }
}

bool CompiledProjectWriter::write(
    const std::string& path,
    const CompiledProject& project,
    Diagnostics& diagnostics
)
{
    const std::filesystem::path outputPath(path);

    if (!CompiledProjectValidator::validate(
        project,
        diagnostics,
        path
    ))
    {
        return false;
    }

    const std::filesystem::path parentPath =
        outputPath.parent_path();

    if (!parentPath.empty())
    {
        std::error_code directoryError;
        std::filesystem::create_directories(
            parentPath,
            directoryError
        );

        if (directoryError)
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectCouldNotBeCreated,
                "Compiled project directory could not be created: " +
                directoryError.message(),
                parentPath.generic_string()
            );

            return false;
        }
    }

    const std::filesystem::path temporaryPath =
        temporaryPathFor(outputPath);
    const std::filesystem::path backupPath =
        backupPathFor(outputPath);

    removeFileIfExists(temporaryPath);
    removeFileIfExists(backupPath);

    try
    {
        std::ofstream file(
            temporaryPath,
            std::ios::binary | std::ios::trunc
        );

        if (!file.is_open())
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectCouldNotBeCreated,
                "Compiled project file could not be created",
                temporaryPath.generic_string()
            );

            return false;
        }

        flx::binary::BinaryWriter writer(file);
        flx::binary::CompiledProjectCodec::write(
            writer,
            project
        );

        file.flush();

        if (!file)
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectWriteFailed,
                "Compiled project file could not be written",
                temporaryPath.generic_string()
            );

            removeFileIfExists(temporaryPath);
            return false;
        }

        file.close();

        if (!file)
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectWriteFailed,
                "Compiled project file could not be closed cleanly",
                temporaryPath.generic_string()
            );

            removeFileIfExists(temporaryPath);
            return false;
        }
    }
    catch (const flx::binary::BinaryException& exception)
    {
        diagnostics.error(
            exception.code,
            exception.what(),
            path,
            exception.field
        );

        removeFileIfExists(temporaryPath);
        return false;
    }

    std::error_code temporarySizeError;
    const uintmax_t temporarySize =
        std::filesystem::file_size(
            temporaryPath,
            temporarySizeError
        );

    if (temporarySizeError)
    {
        diagnostics.error(
            DiagnosticCode::CompiledProjectWriteFailed,
            "Temporary compiled project file size could not be inspected: " +
            temporarySizeError.message(),
            temporaryPath.generic_string(),
            "file"
        );

        removeFileIfExists(temporaryPath);
        return false;
    }

    if (temporarySize > flx::binary::MaxBinaryFileSize)
    {
        diagnostics.error(
            DiagnosticCode::CompiledProjectLimitExceeded,
            "Compiled project file exceeds the maximum binary size",
            temporaryPath.generic_string(),
            "file"
        );

        removeFileIfExists(temporaryPath);
        return false;
    }

    bool hadExistingOutput =
        false;

    std::error_code existsError;
    hadExistingOutput =
        std::filesystem::exists(
            outputPath,
            existsError
        );

    if (existsError)
    {
        diagnostics.error(
            DiagnosticCode::CompiledProjectWriteFailed,
            "Existing compiled project file could not be inspected: " +
            existsError.message(),
            outputPath.generic_string()
        );

        removeFileIfExists(temporaryPath);
        return false;
    }

    if (hadExistingOutput)
    {
        std::error_code backupError;
        std::filesystem::rename(
            outputPath,
            backupPath,
            backupError
        );

        if (backupError)
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectWriteFailed,
                "Existing compiled project file could not be prepared for replacement: " +
                backupError.message(),
                outputPath.generic_string()
            );

            removeFileIfExists(temporaryPath);
            return false;
        }
    }

    std::error_code renameError;
    std::filesystem::rename(
        temporaryPath,
        outputPath,
        renameError
    );

    if (renameError)
    {
        diagnostics.error(
            DiagnosticCode::CompiledProjectWriteFailed,
            "Compiled project file could not be finalized: " +
            renameError.message(),
            outputPath.generic_string()
        );

        removeFileIfExists(temporaryPath);

        if (hadExistingOutput)
        {
            std::error_code restoreError;
            std::filesystem::rename(
                backupPath,
                outputPath,
                restoreError
            );

            if (restoreError)
            {
                diagnostics.error(
                    DiagnosticCode::CompiledProjectWriteFailed,
                    "Previous compiled project file could not be restored: " +
                    restoreError.message(),
                    outputPath.generic_string()
                );
            }
        }

        return false;
    }

    removeFileIfExists(backupPath);

    return true;
}

CompiledProjectBinaryResult CompiledProjectReader::read(
    const std::string& path
)
{
    CompiledProjectBinaryResult result;

    const std::filesystem::path inputPath(path);

    std::error_code sizeError;
    const uintmax_t fileSize =
        std::filesystem::file_size(
            inputPath,
            sizeError
        );

    if (!sizeError && fileSize > flx::binary::MaxBinaryFileSize)
    {
        result.diagnostics.error(
            DiagnosticCode::CompiledProjectLimitExceeded,
            "Compiled project file exceeds the maximum binary size",
            path,
            "file"
        );

        return result;
    }

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        result.diagnostics.error(
            DiagnosticCode::CompiledProjectCouldNotBeOpened,
            "Compiled project file could not be opened",
            path
        );

        return result;
    }

    try
    {
        flx::binary::BinaryReader reader(file);
        flx::binary::DecodedCompiledProject decoded =
            flx::binary::CompiledProjectCodec::read(reader);

        result.project =
            decoded.project;
        result.metadata.formatVersion =
            decoded.metadata.formatVersion;
        result.metadata.producerVersion =
            decoded.metadata.producerVersion;
    }
    catch (const flx::binary::BinaryException& exception)
    {
        result.diagnostics.error(
            exception.code,
            exception.what(),
            path,
            exception.field
        );

        return result;
    }

    if (!CompiledProjectValidator::validate(
        result.project,
        result.diagnostics,
        path
    ))
    {
        return result;
    }

    result.success =
        !result.diagnostics.hasErrors();

    return result;
}
