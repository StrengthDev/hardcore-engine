#pragma once

#include "../core/result.h"
#include "../render/resource/descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

enum HCShaderStage {
    HCShaderStage_Vertex,
    HCShaderStage_Fragment,
    HCShaderStage_Compute,
    HCShaderStage_Mesh,
    HCShaderStage_TesselationControl,
    HCShaderStage_TesselationEvaluation,
    HCShaderStage_Geometry,
    HCShaderStage_Task,
    HCShaderStage_RayGeneration,
    HCShaderStage_RayIntersection,
    HCShaderStage_RayAnyHit,
    HCShaderStage_RayClosestHit,
    HCShaderStage_RayMiss,
    HCShaderStage_RayCallable,
};

/**
 * @brief
 */
struct HCShader {
    void* inner;
};

struct HCResult hc_new_shader(struct HCShader* shader, const uint32_t* bytecode, size_t size, enum HCShaderStage stage);

void hc_destroy_shader(struct HCShader* shader);

struct HCResult hc_shader_push_constants(struct HCShader const* shader, struct HCTypeDescriptor* descriptor_data, uint32_t* data_count);

#ifdef __cplusplus
}
#endif // __cplusplus
