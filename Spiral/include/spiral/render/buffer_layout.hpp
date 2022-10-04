#pragma once

#include <spiral/core/core.hpp>

#include <cstdint>

namespace ENGINE_NAMESPACE
{
	enum AttributeType
	{
		Float, Vec2, Vec3, Vec4
	};

	struct AttributeProperties
	{
		uint32_t location;
		AttributeType type;
		uint32_t offset;
	};

	struct VertexLayout
	{
		uint32_t size; //stride
		const AttributeProperties* attributes;
	};
}