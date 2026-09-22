#pragma once

#include "../../diagnostics/Diagnostics.h"

#include <stdexcept>
#include <string>

namespace flx::binary
{
    class BinaryException : public std::runtime_error
    {
    public:
        BinaryException(
            DiagnosticCode code,
            const std::string& message,
            const std::string& field = ""
        )
            : std::runtime_error(message),
              code(code),
              field(field)
        {
        }

        DiagnosticCode code;
        std::string field;
    };
}
