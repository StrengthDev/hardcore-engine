#include <pch.hpp>

#include <spiral/render/resource.hpp>
#include <spiral/render/renderer_internal.hpp>

namespace Spiral
{
	void resource::destroy()
	{

	}

	object_resource::object_resource(const void* data, const std::size_t size, const std::size_t offset, const bool vertex_data_first,
		const index_format index_type, const data_layout& layout)
		: layout(layout), index_type(index_type)
	{

	}

	object_resource::object_resource(const void* vertex_data, const std::size_t vertex_data_size, const void* index_data,
		const size_t index_data_size, const index_format index_type, const data_layout& layout)
		: layout(layout), index_type(index_type)
	{

	}

	object_resource::object_resource(const void* vertex_data, const std::size_t vertex_data_size, const data_layout& layout)
		: layout(layout), index_type(index_format::NONE), count(static_cast<std::uint32_t>(vertex_data_size / layout.size()))
	{
		INTERNAL_ASSERT((vertex_data_size % layout.size()) == 0, "Allocated vertex memory does not align with vertex layout");
		ref = renderer::get_device().get_memory().ialloc_object(vertex_data, vertex_data_size);
	}

	object_resource::object_resource(const std::size_t vertex_data_size, const std::size_t index_data_size,
		const index_format index_type, const data_layout& layout)
		: layout(layout), index_type(index_type)
	{

	}

	object_resource::object_resource(const std::size_t vertex_data_size, const data_layout& layout)
		: layout(layout), index_type(index_format::NONE), count(static_cast<std::uint32_t>(vertex_data_size / layout.size()))
	{
		INTERNAL_ASSERT(vertex_data_size % layout.size() == 0, "Allocated vertex memory does not align with vertex layout");
		ref = renderer::get_device().get_memory().alloc_object(vertex_data_size);
	}
}
