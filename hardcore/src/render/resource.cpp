#include <pch.hpp>

#include <render/resource.hpp>
#include <render/renderer_internal.hpp>

namespace ENGINE_NAMESPACE
{
	resource::~resource()
	{

	}

	mesh::mesh(const void* data, std::size_t size, std::size_t offset, bool vertex_data_first,
		index_format index_type, const data_layout& layout)
		: index_t(index_type)
	{
		std::size_t vertex_data_size, index_data_size;
		const void *vertex_data, *index_data;
		if (vertex_data_first)
		{
			vertex_data_size = offset;
			index_data_size = size - offset;
			vertex_data = data;
			index_data = static_cast<const std::byte*>(data) + offset;
		}
		else
		{
			vertex_data_size = size - offset;
			index_data_size = offset;
			vertex_data = static_cast<const std::byte*>(data) + offset;
			index_data = data;
		}

		INTERNAL_ASSERT(vertex_data_size% layout.size() == 0, "Allocated vertex memory does not align with vertex layout");
		INTERNAL_ASSERT(index_data_size% index_size(index_type) == 0, "Allocated index memory does not align with index type");
		init(renderer::get_device().get_memory().alloc_vertices(vertex_data, vertex_data_size), layout,
			static_cast<std::uint32_t>(vertex_data_size / layout.size()));
		index_ref = renderer::get_device().get_memory().alloc_indexes(index_data, index_data_size);
		n_indexes = static_cast<std::uint32_t>(index_data_size / index_size(index_type));
	}

	mesh::mesh(const void* vertex_data, std::size_t vertex_data_size, const void* index_data,
		std::size_t index_data_size, index_format index_type, const data_layout& layout)
		: index_t(index_type)
	{
		INTERNAL_ASSERT(vertex_data_size % layout.size() == 0, "Allocated vertex memory does not align with vertex layout");
		INTERNAL_ASSERT(index_data_size % index_size(index_type) == 0, "Allocated index memory does not align with index type");
		init(renderer::get_device().get_memory().alloc_vertices(vertex_data, vertex_data_size), layout,
			static_cast<std::uint32_t>(vertex_data_size / layout.size()));
		index_ref = renderer::get_device().get_memory().alloc_indexes(index_data, index_data_size);
		n_indexes = static_cast<std::uint32_t>(index_data_size / index_size(index_type));
	}

	mesh::mesh(const void* vertex_data, std::size_t vertex_data_size, const data_layout& layout)
		: index_t(index_format::NONE), n_indexes(0)
	{
		INTERNAL_ASSERT((vertex_data_size % layout.size()) == 0, "Allocated vertex memory does not align with vertex layout");
		init(renderer::get_device().get_memory().alloc_vertices(vertex_data, vertex_data_size), layout, 
			static_cast<std::uint32_t>(vertex_data_size / layout.size()));
	}

	mesh::mesh(std::size_t vertex_data_size, std::size_t index_data_size,
		index_format index_type, const data_layout& layout)
		: index_t(index_type)
	{
		INTERNAL_ASSERT(vertex_data_size % layout.size() == 0, "Allocated vertex memory does not align with vertex layout");
		INTERNAL_ASSERT(index_data_size % index_size(index_type) == 0, "Allocated index memory does not align with index type");
		init(renderer::get_device().get_memory().alloc_vertices(vertex_data_size), layout,
			static_cast<std::uint32_t>(vertex_data_size / layout.size()));
		index_ref = renderer::get_device().get_memory().alloc_indexes(index_data_size);
		n_indexes = static_cast<std::uint32_t>(index_data_size / index_size(index_type));
	}

	mesh::mesh(std::size_t vertex_data_size, const data_layout& layout)
		: index_t(index_format::NONE), n_indexes(0)
	{
		INTERNAL_ASSERT(vertex_data_size % layout.size() == 0, "Allocated vertex memory does not align with vertex layout");
		init(renderer::get_device().get_memory().alloc_vertices(vertex_data_size), layout, 
			static_cast<std::uint32_t>(vertex_data_size / layout.size()));
	}

	void unmapped_resource::update(void* data, std::size_t size, std::size_t offset)
	{
		renderer::get_device().get_memory().submit_upload(ref, data, size, offset);
	}

	resizable_resource::resizable_resource(memory_ref&& ref, const data_layout& layout, std::uint32_t count)
	{

	}

	uniform::uniform(const data_layout& layout, std::uint32_t count) :
		resource(renderer::get_device().get_memory().alloc_dynamic_uniform(layout.size() * count), layout, count)
	{
		update_map();
	}

	void uniform::update_map()
	{
		renderer::get_device().get_memory().uniform_map(ref, &data_ptr, &offset);
	}

	unmapped_uniform::unmapped_uniform(const data_layout& layout, std::uint32_t count) :
		resource(renderer::get_device().get_memory().alloc_uniform(layout.size() * count), layout, count)
	{

	}

	storage_array::storage_array(const data_layout& layout, std::uint32_t count) :
		resource(renderer::get_device().get_memory().alloc_storage(layout.size() * count), layout, count)
	{

	}

	dynamic_storage_array::dynamic_storage_array(const data_layout& layout, std::uint32_t count) :
		resource(renderer::get_device().get_memory().alloc_dynamic_storage(layout.size() * count), layout, count)
	{
		update_map();
	}

	void dynamic_storage_array::update_map()
	{
		renderer::get_device().get_memory().storage_map(ref, &data_ptr, &offset);
	}

	storage_vector::storage_vector(const data_layout& layout, std::uint32_t count) :
		resource(renderer::get_device().get_memory().alloc_storage(layout.size() * count), layout, count)
	{

	}

	void storage_vector::resize(std::uint32_t new_count)
	{

	}

	dynamic_storage_vector::dynamic_storage_vector(const data_layout& layout, std::uint32_t count) :
		resource(renderer::get_device().get_memory().alloc_dynamic_storage(layout.size() * count), layout, count)
	{
		update_map();
	}

	void dynamic_storage_vector::resize(std::uint32_t new_count)
	{

	}

	void dynamic_storage_vector::update_map()
	{
		renderer::get_device().get_memory().storage_map(ref, &data_ptr, &offset);
	}
}
