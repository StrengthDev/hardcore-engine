#pragma once

#pragma warning(push, 0)
#include <spirv_reflect.h>
#pragma warning(pop)

namespace hc::render {
    /**
    * @brief Returns a C string representation of the provided `SpvReflectResult`.
    *
    * @param result The `SpvReflectResult` for which the string is returned.
    * @return A C string representation of the `SpvReflectResult`.
    */
    const char* to_str(SpvReflectResult result);
}
