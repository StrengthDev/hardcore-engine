
#include <pch.hpp>

#include "log.hpp"

#include <core/log.h>

#ifdef HC_LOGGING

namespace hc {
    void default_log(HCLogKind kind, const char* message) {
        if (kind == HCLogKind_Error) {
            std::cerr << message << std::endl;
        } else {
            std::cout << message << std::endl;
        }
    }

    static HCLogFn log_fn = default_log;
    static HCStartSpanFn start_span_fn = nullptr;
    static HCEndSpanFn end_span_fn = nullptr;

    void log(HCLogKind kind, const char* message) {
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

    Span::Span(HCLogKind kind, const char* name) {
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
