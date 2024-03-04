#include <pch.hpp>

#include <core/log.h>
#include "log.hpp"

#ifdef HC_LOGGING

/**
 * @brief The default logging function.
 *
 * @param kind The type of the log.
 * @param message The message of the log.
 */
void default_log(HCLogKind kind, const char *message) {
    switch (kind) {
        case Trace:
        case Info:
        case Debug:
        case Warn:
            std::cout << message << std::endl;
            break;
        case Error:
            std::cerr << message << std::endl;
            break;
    }
}

HCLogFn log_fn = default_log; //!< The logging function to use for the whole library.

namespace hc {
    void log(HCLogKind kind, const char *message) {
        if (log_fn) {
            log_fn(kind, message);
        }
    }

    void set_log(HCLogFn fn_ptr) {
        log_fn = fn_ptr;
    }
}

#endif // HC_LOGGING
