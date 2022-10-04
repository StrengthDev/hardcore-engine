#pragma once

#include <spiral/core/core.hpp>

#include "data_layout.hpp"
#include "memory_reference.hpp"

namespace ENGINE_NAMESPACE
{
	class ENGINE_API instance
	{
	public:
		const data_layout layout;

		inline bool valid() const { return ref.valid(); }

		inline std::uint32_t get_count() const noexcept { return count; }

	protected:
		instance() = default;
		instance(const data_layout& layout);

		~instance();

		instance(const instance&) = delete;
		instance& operator=(const instance&) = delete;

		instance(instance&& other) noexcept : ref(std::move(other.ref)), count(std::exchange(other.count, 0)) {}

		instance& operator=(instance&& other)
		{
			this->~instance();
			ref = std::move(other.ref);
			count = std::exchange(other.count, 0);
			return *this;
		}

		memory_reference ref;
		std::uint32_t count = 0;
	};

	class ENGINE_API expanding_instance_vector : public virtual instance
	{
	protected:
		expanding_instance_vector() = default;
		expanding_instance_vector(const data_layout& layout) : instance(layout) {}

		virtual void grow(std::size_t new_size) = 0;
	};

	class ENGINE_API device_instance_array : public instance
	{
	public:
		device_instance_array(std::uint32_t count, const data_layout& layout);
	};

	class ENGINE_API device_instance_vector : public expanding_instance_vector
	{
	public:
		device_instance_vector(std::uint32_t initial_count, const data_layout& layout);

	protected:
		void grow(std::size_t new_size) override;
	};

	class instance_ref;

	class ENGINE_API host_accessible_instance : public virtual instance
	{
	protected:
		host_accessible_instance() = default;
		host_accessible_instance(const data_layout& layout) : instance(layout) {}

	public:
		instance_ref operator[](std::size_t index) noexcept;

	private:
		void update_map(void* ptr) noexcept { data = ptr; }

		void* data = nullptr;

		friend instance_ref;
	};

	class ENGINE_API instance_ref
	{
	public:
		void set(void* data) const
		{
			std::memcpy(reinterpret_cast<std::byte*>(ref.data) + ref.layout.size() * index, data, ref.layout.size());
		}

		void set_field(std::size_t field_idx, void* data)
		{

		}

	private:
		instance_ref() = delete;
		instance_ref(host_accessible_instance& ref, std::size_t index) noexcept : ref(ref), index(index) {}

		host_accessible_instance& ref;
		const std::size_t index;

		friend host_accessible_instance;
	};

	class ENGINE_API instance_array : public host_accessible_instance
	{
	public:
		instance_array(std::uint32_t count, const data_layout& layout);
	};

	class ENGINE_API instance_vector : public host_accessible_instance, public expanding_instance_vector
	{
	public:
		instance_vector() = default;
		instance_vector(std::uint32_t initial_count, const data_layout& layout);

	protected:
		void grow(std::size_t new_size) override;
	};
}
