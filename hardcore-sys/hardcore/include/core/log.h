#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief The type/scope of a log.
 *
 * Applicable to both log events and spans.
 */
enum HCLogKind {
	Trace, //!< Designates very low priority, often extremely verbose, information.
	Debug, //!< Designates lower priority information.
	Info, //!< Designates useful information.
	Warn, //!< Designates hazardous situations.
	Error, //!< Designates very serious errors.
};

/**
 * @brief The type/signature of a logging function.
 *
 * This kind of function is used to emit log events.
 *
 * @param kind - The scope of the log event.
 * @param text - The log message.
 */
typedef void (*HCLogFn)(enum HCLogKind kind, const char *text);

/**
 * @brief The type/signature of a start span function.
 *
 * This kind of function is used to begin a new span.
 *
 * @param kind - The scope of the span.
 * @param name - The name of the span.
 *
 * @return A pointer to the new span object.
 */
typedef void *(*HCStartSpanFn)(enum HCLogKind kind, const char *name);

/**
 * @brief The type/signature of an end span function.
 *
 * This kind of function is used to end a span.
 *
 * @param span_ptr - A pointer to the span to end.
 */
typedef void (*HCEndSpanFn)(void *span_ptr);

#ifdef __cplusplus
}
#endif // __cplusplus
