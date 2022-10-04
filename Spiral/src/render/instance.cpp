#include <pch.hpp>

#include <spiral/render/instance.hpp>
#include <spiral/render/renderer_internal.hpp>

namespace Spiral
{
	instance::instance(const data_layout& layout) : layout(layout)
	{
		INTERNAL_ASSERT(layout.count() != 0, "Invalid layout");
		INTERNAL_ASSERT(layout.size() != 0, "Invalid layout");
	}

	instance::~instance()
	{

	}

	device_instance_array::device_instance_array(std::uint32_t count, const data_layout& layout) : instance(layout)
	{
		this->count = count;
		ref = renderer::get_device().get_memory().alloc_object(count * layout.size());
	}

	device_instance_vector::device_instance_vector(std::uint32_t count, const data_layout& layout)
		: instance(layout), expanding_instance_vector(layout)
	{
		this->count = count;
		ref = renderer::get_device().get_memory().alloc_object(count * layout.size());
	}

	void device_instance_vector::grow(std::size_t new_size)
	{

	}

	instance_ref host_accessible_instance::operator[](std::size_t index) noexcept
	{
		return instance_ref(*this, index);
	}

	instance_array::instance_array(std::uint32_t count, const data_layout& layout)
		: instance(layout), host_accessible_instance(layout)
	{
		this->count = count;
		//ref = renderer::get_device().get_memory().alloc_dynamic_object(count * layout.size());
	}

	instance_vector::instance_vector(std::uint32_t count, const data_layout& layout)
		: instance(layout), host_accessible_instance(layout), expanding_instance_vector(layout)
	{
		this->count = count;
		//ref = renderer::get_device().get_memory().alloc_dynamic_object(count * layout.size());
	}

	void instance_vector::grow(std::size_t new_size)
	{

	}
}
