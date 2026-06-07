
#include <pch.hpp>

#include "push_constants_processor.hpp"

#include <core/log.hpp>

namespace hc::render::pipeline {
    static inline u32 plain_data_size(HCBasicDescriptor const& descriptor) {
        u32 size = 0;

        if (descriptor.composition == HCComposition_Scalar
            || descriptor.composition == HCComposition_Vec2
            || descriptor.composition == HCComposition_Vec3
            || descriptor.composition == HCComposition_Vec4) {
            size = (descriptor.primitive_size + 8 - 1) / 8;
        } else {
            size = descriptor.matrix_stride;
        }

        u32 count = 0;
        switch (descriptor.composition) {
        case HCComposition_Scalar:
            count = 1;
            break;
        case HCComposition_Vec2:
        case HCComposition_Mat2x2:
        case HCComposition_Mat3x2:
        case HCComposition_Mat4x2:
            count = 2;
            break;
        case HCComposition_Vec3:
        case HCComposition_Mat2x3:
        case HCComposition_Mat3x3:
        case HCComposition_Mat4x3:
            count = 3;
            break;
        case HCComposition_Vec4:
        case HCComposition_Mat2x4:
        case HCComposition_Mat3x4:
        case HCComposition_Mat4x4:
            count = 4;
            break;
        }

        return count * size;
    }

    std::expected<PushConstantsProcessor, Error> PushConstantsProcessor::create(
        std::optional<render::PushConstant> const& push_constants
    ) {
        if (!push_constants || push_constants->descriptor.data_vec().empty()) {
            return {};
        }

        auto const& struct_descriptor = push_constants->descriptor.data_vec().front();
        if (struct_descriptor.type_category != HCDescriptorCategory_Struct) {
            HC_ERROR("Unsupported push constants layout");
            return Error(HCError_InvalidShader);
        }

        PushConstantsProcessor processor;

        // Buffer size must be a multiple of 4
        Sz real_size = push_constants->descriptor.size();
        processor.buffer_size = (real_size + 4 - 1) / 4 * 4;

        processor.constants.reserve(struct_descriptor.type_descriptor.struct_descriptor.member_count);

        for (u16 i = 0; i < struct_descriptor.type_descriptor.struct_descriptor.member_count; ++i) {
            u16 const member_idx = struct_descriptor.type_descriptor.struct_descriptor.first_member_type_idx + i;
            auto const& member = push_constants->descriptor.data_vec()[member_idx].type_descriptor.member_descriptor;

            auto const& constant_descriptor = push_constants->descriptor.data_vec()[member.type_idx];

            PushConstant constant = {};
            constant.offset = member.offset;

            switch (constant_descriptor.type_category) {
            case HCDescriptorCategory_Basic:
                constant.type = PushConstantType::PlainData;
                constant.size = plain_data_size(constant_descriptor.type_descriptor.basic_descriptor);
                break;
            case HCDescriptorCategory_Array:
                constant.type = PushConstantType::PlainData;
                constant.size = constant_descriptor.type_descriptor.array_descriptor.count
                    * constant_descriptor.type_descriptor.array_descriptor.stride;
                break;
            case HCDescriptorCategory_Pointer:
                constant.type = PushConstantType::BufferAddress;
                constant.size = 8;
                break;
            default:
                HC_ERROR("Unsupported push constants layout");
                return Error(HCError_InvalidShader);
            }

            processor.constants.emplace_back(constant);
        }

        return processor;
    }

    Sz PushConstantsProcessor::push_constants_size() const noexcept {
        return this->buffer_size;
    }

    std::expected<void, Error> PushConstantsProcessor::operator()(
        std::vector<u8>& buffer,
        std::span<void const*> const& constant_ptrs
    ) const noexcept {
        if (constant_ptrs.size() != this->constants.size()) {
            HC_ERROR("Number of provided push constants does not match the pipeline");
            return Error(HCError_InvalidParams);
        }

        for (auto const& [constant, ptr] : std::ranges::zip_view(this->constants, constant_ptrs)) {
            void const* data_ptr = ptr;

            if (constant.type == PushConstantType::BufferAddress) {
                // TODO
            }

            std::memcpy(buffer.data() + constant.offset, data_ptr, constant.size);
        }

        return {};
    }
}
