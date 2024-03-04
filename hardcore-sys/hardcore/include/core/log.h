#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum HCLogKind {
    Trace,
    Info,
    Debug,
    Warn,
    Error,
};

typedef void (*HCLogFn)(enum HCLogKind, const char *);

#ifdef __cplusplus
}
#endif // __cplusplus
