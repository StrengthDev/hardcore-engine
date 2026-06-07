
#pragma once

#include <core/error.hpp>
#include <core/log.hpp>

#define HC_VALIDATE(condition, err_msg) \
if (!(condition)) {                     \
    HC_WARN((err_msg));                 \
    return;                             \
}(0)

#define HC_VALIDATE_PTR(ptr, name) HC_VALIDATE((ptr), "Null " name " pointer")

#define HC_VALIDATE_PTR_RANGE(ptr, count, name)         \
HC_VALIDATE_PTR(ptr, name);                             \
for(decltype(count) i = 0; i < (count); i++) {          \
    HC_VALIDATE_PTR(*((ptr) + i), name " element");     \
}(0)

#define HC_VALIDATE_RE(condition, err_msg)      \
if (!(condition)) {                             \
    HC_ERROR((err_msg));                        \
    return hc::Error(HCError_InvalidParams);    \
}(0)

#define HC_VALIDATE_PTR_RE(ptr, name) HC_VALIDATE_RE((ptr), "Null " name " pointer")

#define HC_VALIDATE_PTR_RANGE_RE(ptr, count, name)      \
HC_VALIDATE_PTR_RE(ptr, name);                          \
for(decltype(count) i = 0; i < (count); i++) {          \
    HC_VALIDATE_PTR_RE(*((ptr) + i), name " element");  \
}(0)
