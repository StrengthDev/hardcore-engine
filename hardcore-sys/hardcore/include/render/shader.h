#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

enum HCShaderStage {
    VertexStage,
    FragmentStage,
    ComputeStage,
    MeshStage,
    TesselationControlStage,
    TesselationEvaluationStage,
    GeometryStage,
    TaskStage,
    RayGenerationStage,
    RayIntersectionStage,
    RayAnyHitStage,
    RayClosestHitStage,
    RayMissStage,
    RayCallableStage,
};

/**
 * @brief
 */
struct HCShader {
    void* inner;
};

struct HCShader hc_create_shader(const uint32_t* bytecode, size_t size, enum HCShaderStage stage);

void hc_destroy_shader(struct HCShader* shader);

#ifdef __cplusplus
}
#endif // __cplusplus
