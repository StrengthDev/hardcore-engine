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
    if (kind == Error)
        std::cerr << message << std::endl;
    else
        std::cout << message << std::endl;
}

static HCLogFn log_fn = default_log; //!< The logging function to use for the whole library.
static HCStartSpanFn start_span_fn = nullptr; //!< The start span function to use for the whole library.
static HCEndSpanFn end_span_fn = nullptr; //!< The end span function to use for the whole library.

namespace hc {
    void log(HCLogKind kind, const char *message) {
        if (log_fn) {
            log_fn(kind, message);
        }
    }

    void set_log(HCLogFn fn_ptr) {
        log_fn = fn_ptr;
    }

    void set_span(HCStartSpanFn start_fn_ptr, HCEndSpanFn end_fn_ptr) {
        start_span_fn = start_fn_ptr;
        end_span_fn = end_fn_ptr;
    }

    Span::Span(HCLogKind kind, const char *name) {
        if (start_span_fn) {
            this->inner = start_span_fn(kind, name);
        }
    }

    Span::~Span() {
        if (this->inner) {
            if (end_span_fn) {
                end_span_fn(this->inner);
            } else {
                HC_ERROR("Span has leaked");
            }
        }
    }
}

#endif // HC_LOGGING
