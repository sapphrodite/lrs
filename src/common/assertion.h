#ifndef ASSERTION_H
#define ASSERTION_H

#include <source_location>
#include <string>
#include <stdexcept> // std::logic_error

// An exception-based assertion, disabled in release builds. Useful for unit/integration testing
static void assertion(bool expression, const char* error_message, const std::source_location location = std::source_location::current()) {
    #ifndef NDEBUG
    if (!expression) {
        // Still waiting for GCC to support std::format...
        std::string message = "Assertion failed at " + std::string(location.file_name())
                + ", line " + std::to_string(location.line())
                + ", with message: \"" + std::string(error_message) +"\"";
        throw std::logic_error(message);
    }
    #endif //NDEBUG
}

#endif //ASSERTION_H