execute_process(
    COMMAND "${FLX_CLI}" bulid
    RESULT_VARIABLE FLX_RESULT
    OUTPUT_VARIABLE FLX_OUTPUT
    ERROR_VARIABLE FLX_ERROR
)

if(NOT FLX_RESULT EQUAL 2)
    message(FATAL_ERROR "Expected exit code 2, got ${FLX_RESULT}")
endif()

if(NOT FLX_ERROR MATCHES "Unknown command: bulid")
    message(FATAL_ERROR "Expected unknown command diagnostic. stderr: ${FLX_ERROR}")
endif()

if(NOT FLX_ERROR MATCHES "flx --help")
    message(FATAL_ERROR "Expected help suggestion. stderr: ${FLX_ERROR}")
endif()
