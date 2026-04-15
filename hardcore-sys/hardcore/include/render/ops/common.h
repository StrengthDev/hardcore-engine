#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief The type of dependency operation.
 */
enum HCDependencyType {
    HCDependencyType_RenderPass, //!< Render pass operation.
};

/**
 * @brief A dependency operation.
 * 
 * This type is usually associated with a resource, indicating an operation which modified said resource.
 */
struct HCDependency {
    void const* handle; //!< A pointer to the operation handle. A null value indicates there is no dependency.
    enum HCDependencyType type; //!< The type of this dependency. Ignored when the handle is null.
};

/**
 * @brief The type/signature of an operation predicate.
 *
 * This kind of function is typically used to check if an operation should be executed within a frame.
 *
 * @param frame The number of the current frame.
 * @param user_data User data to be passed to the predicate function.
 *
 * @return `true` if the operation is supposed to execute for the current frame, `false` otherwise.
 */
typedef bool (*HCOperationPredicate)(size_t frame, void* user_data);

#ifdef __cplusplus
}
#endif // __cplusplus
