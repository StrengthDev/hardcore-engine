
#pragma once

#include <util/number.hpp>

#include <variant>

namespace hc::render::device::graph {
    enum class ResourceType: u8 {
        Texture,
    };

    struct ResourceRef {
        u64 id = std::numeric_limits<u64>::max();
        ResourceType type;
    };

    struct RootNode {};

    struct RenderPassNode {};

    struct DrawNode {
        u32 vertex_count;
        u32 instance_count;
        u64 pipeline_id;
        VkPipeline pipeline_instance;
    };

    typedef std::variant<RootNode, RenderPassNode, DrawNode> NodeVariant;

    struct Node {
        std::vector<ResourceRef> inputs;
        std::vector<ResourceRef> outputs;
        std::vector<u64> dependencies;
        std::vector<u64> dependents;
        NodeVariant variant;
    };
}
