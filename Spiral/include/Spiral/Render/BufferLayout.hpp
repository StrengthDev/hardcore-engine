#pragma once

#include <cstdint>

namespace Spiral
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