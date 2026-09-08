#pragma once

#include "../CompiledProject.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

#include <string>

namespace flx::binary
{
    struct CompiledProjectBinaryMetadata
    {
        uint32_t formatVersion = 0;
        std::string producerVersion;
    };

    struct DecodedCompiledProject
    {
        CompiledProject project;
        CompiledProjectBinaryMetadata metadata;
    };

    class CompiledProjectCodec
    {
    public:
        static constexpr uint32_t Magic =
            0x43584C46;

        static constexpr uint32_t FormatVersion =
            8;

        static void write(
            BinaryWriter& writer,
            const CompiledProject& project
        );

        static DecodedCompiledProject read(
            BinaryReader& reader
        );
    };
}
