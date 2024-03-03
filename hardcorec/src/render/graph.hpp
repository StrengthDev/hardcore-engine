#pragma once

#include "render_core.hpp"

#include <vector>

namespace ENGINE_NAMESPACE {
    /**
     * The type a node represents.
     */
    enum node_type : u8 {
        RESOURCE = 0,
        TARGET,
        ATTACHMENT,
        RASTER_PIPELINE,
        COMPUTE_PIPELINE,
        RAYTRACING_PIPELINE,
    };

    /**
     * A node in a graph, with connections to the nodes that lead to it.
     */
    struct node {
        //!< The indexes of all of this node's dependencies in a vector holding all nodes.
        std::vector<std::size_t> dependencies;

        //!< The index of this node in a vector holding all node values of this type.
        std::size_t idx;

        //!< The type this node represents.
        node_type type;

        //!< The pipeline used to execute this node.
        std::size_t pipeline;

        // Descriptor sets
    };

    /**
     * @brief An acyclic computational graph.
     *
     * Each computation the device performs is represented as a node in this graph.
     */
    class graph {
    public:
        graph() = default;

        graph(graph &&other);

        graph &operator=(graph &&other);

        graph(const graph &) = delete;

        graph &operator=(const graph &) = delete;

        void add_output();

        void add_pipeline();

        void record();

        /**
         * @brief Verifies if the graph can be executed without issues.
         *
         * Goes through every node/computation and resource in the graph to verify if there are any issues.
         *
         * @details This function should be called once per frame, before the graph is recorded. If there are no changes
         * in the graph's nodes since the last frame, should be a no-op.
         *
         * @return <b>\c true</b> if the graph is valid, <b>\c false</b> otherwise.
         */
        bool validate();

        /**
         * @brief Adds a new resource to the graph.
         *
         * @details The graph does not allocate any resources by itself, it will just receive and store and handle.
         */
        void add_resource(); //(resource_handle resource);

        /**
         * @brief Removes an existing resource from the graph.
         */
        void remove_resource(); //(resource_handle resource);

    private:
        std::vector<node> nodes;
        std::vector<void *> resources;
        std::vector<void *> pipelines;

        std::vector<std::size_t> passive_eps;
        std::vector<std::size_t> active_eps;

        std::vector<std::size_t> outputs;
    };
}
