#pragma once

#include <sstream>

#include <core/log.h>

#ifdef HC_LOGGING

namespace hc {
    /**
     * @brief Emit a log.
     *
     * @param kind The kind of log to emit.
     * @param message The message of the log to emit.
     */
    void log(HCLogKind kind, const char *message);

    /**
     * @brief Set the global logging function.
     *
     * @param fn_ptr The pointer of the new logging function.
     */
    void set_log(HCLogFn fn_ptr);
}

#define LOG_WRAPPER(kind, message) \
{                                  \
    std::stringstream stream;      \
    stream << message;             \
    auto str = stream.str();       \
    hc::log(kind, str.c_str());    \
}(0)

#define HC_TRACE(message) LOG_WRAPPER(HCLogKind::Trace, message)
#define HC_INFO(message) LOG_WRAPPER(HCLogKind::Info, message)
#define HC_DEBUG(message) LOG_WRAPPER(HCLogKind::Debug, message)
#define HC_WARN(message) LOG_WRAPPER(HCLogKind::Warn, message)
#define HC_ERROR(message) LOG_WRAPPER(HCLogKind::Error, message)

#else

#define HC_TRACE(message)
#define HC_INFO(message)
#define HC_DEBUG(message)
#define HC_WARN(message)
#define HC_ERROR(message)

#endif // HC_LOGGING
