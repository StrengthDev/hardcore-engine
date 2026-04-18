
#include <pch.hpp>

#include "render_pass_bank.hpp"

namespace hc::render {
    void RenderPassBank::destroy(VolkDeviceTable const& fn_table, VkDevice device) noexcept {
        for (auto& render_pass : this->instances | std::views::values) {
            render_pass.handle.destroy(fn_table, device);
        }

        this->bank.clear();
        this->instances.clear();
    }

    std::expected<u64, Error> RenderPassBank::create_render_pass(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::span<HCSubpass const> const& subpasses,
        Bank<texture::Texture> const& textures
    ) {
        auto render_pass_result = RenderPass::create(subpasses, textures);
        if (!render_pass_result) {
            return render_pass_result.error();
        }
        RenderPass render_pass = *std::move(render_pass_result);

        if (RenderPassInstanceKey const& key = render_pass.instance_key(); !this->instances.contains(key)) {
            auto instance_result = key.create_instance(fn_table, device);
            if (!instance_result) {
                return instance_result.error();
            }

            this->instances.emplace(key, RenderPassInstance{*std::move(instance_result), 1});
        }

        return this->bank.insert(std::move(render_pass));
    }

    void RenderPassBank::destroy_render_pass(VolkDeviceTable const& fn_table, VkDevice device, u64 id) noexcept {
        RenderPass render_pass = this->bank.erase(id);

        RenderPassInstance& instance = this->instances[render_pass.instance_key()];
        instance.ref_count--;

        if (instance.ref_count == 0) {
            instance.handle.destroy(fn_table, device);

            this->instances.erase(render_pass.instance_key());
        }
    }

    bool RenderPassBank::contains(u64 id) const {
        return this->bank.contains(id);
    }

    std::pair<RenderPass&, VkRenderPass> RenderPassBank::operator[](u64 id) noexcept {
        RenderPass& render_pass = this->bank[id];

        return {render_pass, this->instances[render_pass.instance_key()].handle};
    }

    std::pair<RenderPass const&, VkRenderPass> RenderPassBank::operator[](
        u64 id
    ) const noexcept {
        RenderPass const& render_pass = this->bank[id];

        return {render_pass, this->instances.at(render_pass.instance_key()).handle};
    }
}
