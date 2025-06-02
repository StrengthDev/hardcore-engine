#pragma once

#ifdef HC_LOGGING

#include <core/log.h>

#include <sstream>

namespace hc {
    /**
    * @brief Emit a log.
    *
    * @param kind The kind of log to emit.
    * @param message The message of the log to emit.
    */
    void log(HCLogKind kind, const char* message);

    /**
    * @brief Set the global logging function.
    *
    * @param fn_ptr The pointer of the new logging function.
    */
    void set_log(HCLogFn fn_ptr);

    /**
    * @brief Set the global span start and end functions.
    *
    * @param start_fn_ptr The pointer of the new start span function.
    * @param end_fn_ptr The pointer of the new end span function.
    */
    void set_span(HCStartSpanFn start_fn_ptr, HCEndSpanFn end_fn_ptr);

    class Span {
    public:
        Span() = delete;

        Span(Span const&) = delete;

        Span& operator=(Span const&) = delete;

        Span(Span&&) = delete;

        Span& operator=(Span&&) = delete;

        Span(HCLogKind kind, const char* name);

        ~Span();

    private:
        void* inner = nullptr;
    };
}

#define CONCAT2(x, y) x ## y

#define LOG_STREAM_NAME2(line) CONCAT2(log_stream_, line)
#define LOG_STREAM_NAME LOG_STREAM_NAME2(__LINE__)

#define LOG_STR_NAME2(line) CONCAT2(log_str_, line)
#define LOG_STR_NAME LOG_STR_NAME2(__LINE__)

#define LOG_WRAPPER(kind, message)              \
{                                               \
    std::stringstream LOG_STREAM_NAME;          \
    LOG_STREAM_NAME << message;                 \
    auto LOG_STR_NAME = LOG_STREAM_NAME.str();  \
    hc::log(kind, LOG_STR_NAME.c_str());        \
}(0)

#define HC_TRACE(message) LOG_WRAPPER(HCLogKind::HCLogKind_Trace, message)
#define HC_DEBUG(message) LOG_WRAPPER(HCLogKind::HCLogKind_Debug, message)
#define HC_INFO(message) LOG_WRAPPER(HCLogKind::HCLogKind_Info, message)
#define HC_WARN(message) LOG_WRAPPER(HCLogKind::HCLogKind_Warn, message)
#define HC_ERROR(message) LOG_WRAPPER(HCLogKind::HCLogKind_Error, message)

#define HC_SPAN_NAME(line) CONCAT2(span, line)

#define HC_TRACE_SPAN(name) hc::Span HC_SPAN_NAME(__LINE__) (HCLogKind::HCLogKind_Trace, name)
#define HC_DEBUG_SPAN(name) hc::Span HC_SPAN_NAME(__LINE__) (HCLogKind::HCLogKind_Debug, name)
#define HC_INFO_SPAN(name) hc::Span HC_SPAN_NAME(__LINE__) (HCLogKind::HCLogKind_Info, name)
#define HC_WARN_SPAN(name) hc::Span HC_SPAN_NAME(__LINE__) (HCLogKind::HCLogKind_Warn, name)
#define HC_ERROR_SPAN(name) hc::Span HC_SPAN_NAME(__LINE__) (HCLogKind::HCLogKind_Error, name)

#else

#define HC_TRACE(message)
#define HC_DEBUG(message)
#define HC_INFO(message)
#define HC_WARN(message)
#define HC_ERROR(message)

#define HC_TRACE_SPAN(name)
#define HC_DEBUG_SPAN(name)
#define HC_INFO_SPAN(name)
#define HC_WARN_SPAN(name)
#define HC_ERROR_SPAN(name)

#endif // HC_LOGGING
