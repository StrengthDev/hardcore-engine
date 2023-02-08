#pragma once

#include "memory_ref.hpp"
#include "data_layout.hpp"

#include <hardcore/core/core.hpp>

#include <utility>

namespace ENGINE_NAMESPACE
{
	class ENGINE_API resource
	{
	public:
		~resource();

		inline bool valid() const { return ref.valid(); }

		resource(const resource&) = delete;
		resource& operator=(const resource&) = delete;

		resource(resource&& other) noexcept : ref(std::move(other.ref)), element_layout(std::move(other.element_layout)), 
			n_elements(std::exchange(other.n_elements, 0)) 
		{ }

		inline resource& operator=(resource&& other) noexcept
		{
			this->~resource();
			ref = std::move(other.ref);
			element_layout = std::move(other.element_layout);
			n_elements = std::exchange(other.n_elements, 0);
			return *this;
		}

		inline const data_layout& layout() const noexcept { return element_layout; }
		inline std::uint32_t count() const noexcept { return n_elements; }
		inline std::size_t size() const noexcept { return element_layout.size() * n_elements; }
 
	protected:
		resource() = default;

		resource(memory_ref&& ref, const data_layout& layout, std::uint32_t count) noexcept :
			ref(std::move(ref)), element_layout(layout), n_elements(count)
		{ }

		void init(memory_ref&& ref, const data_layout& layout, std::uint32_t count) noexcept
		{
			this->ref = std::move(ref);
			this->element_layout = layout;
			this->n_elements = count;
		}

		memory_ref ref;
		data_layout element_layout;
		std::uint32_t n_elements = 0;

	private:
		friend struct std::hash<resource>;
	};

	class ENGINE_API mesh : public resource
	{
	public:
		/**
		 * @brief 
		*/
		enum class index_format : std::uint8_t
		{
			NONE = 0,
			UINT8, // requires extension
			UINT16,
			UINT32,
		};

		mesh() = default;

		mesh(const void* data, std::size_t size, std::size_t offset, bool vertex_data_first,
			index_format index_type, const data_layout& layout);
		mesh(const void* vertex_data, std::size_t vertex_data_size, const void* index_data,
			std::size_t index_data_size, index_format index_type, const data_layout& layout);
		mesh(const void* vertex_data, std::size_t vertex_data_size, const data_layout& layout);

		mesh(std::size_t vertex_data_size, std::size_t index_data_size,
			index_format index_type, const data_layout& layout);
		mesh(std::size_t vertex_data_size, const data_layout& layout);

		template<typename... Types>
		static inline mesh create(const void* data, std::size_t size, std::size_t offset, bool vertex_data_first, 
			index_format index_type)
		{
			return mesh(data, size, offset, vertex_data_first, index_type, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline mesh create(const void* vertex_data, std::size_t vertex_data_size,
			const void* index_data, std::size_t index_data_size, index_format index_type)
		{
			return mesh(vertex_data, vertex_data_size, index_data, index_data_size, index_type, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline mesh create(const void* vertex_data, std::size_t vertex_data_size)
		{
			return mesh(vertex_data, vertex_data_size, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline mesh create(std::size_t vertex_data_size, std::size_t index_data_size, index_format index_type)
		{
			return mesh(vertex_data_size, index_data_size, index_type, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline mesh create(std::size_t vertex_data_size)
		{
			return mesh(vertex_data_size, data_layout::create<Types...>());
		}

		mesh(mesh&& other) noexcept : resource(std::move(other)), index_ref(std::move(other.index_ref)),
			index_t(std::exchange(other.index_t, index_format::NONE)), n_indexes(std::exchange(other.n_indexes, 0))
		{ }

		inline mesh& operator=(mesh&& other) noexcept
		{
			resource::operator=(std::move(other));
			index_ref = std::move(other.index_ref);
			index_t = std::exchange(other.index_t, index_format::NONE);
			n_indexes = std::exchange(other.n_indexes, 0);
			return *this;
		}

		inline index_format index_type() const noexcept { return index_t; }
		inline std::uint32_t index_count() const noexcept { return n_indexes; }
		inline std::uint32_t draw_count() const noexcept { return index_t == index_format::NONE ? n_elements : n_indexes; }
		inline std::size_t index_size() const noexcept { return static_cast<std::size_t>(n_indexes) * index_size(index_t); }

	protected:
		memory_ref index_ref;

	private:
		index_format index_t = index_format::NONE;
		std::uint32_t n_indexes = 0;

		static inline std::uint32_t index_size(index_format format) noexcept
		{
			switch (format)
			{
			case index_format::NONE:	return 0;
			case index_format::UINT8:	return 1;
			case index_format::UINT16:	return 2;
			case index_format::UINT32:	return 4;
			default:
				INTERNAL_ASSERT(false, "Invalid vertex type");
				break;
			}
			return std::numeric_limits<std::uint32_t>::max();
		}
	};

	class ENGINE_API unmapped_resource : public virtual resource
	{
	protected:
		unmapped_resource() = default;

	public:
		void update(void* data, std::size_t size, std::size_t offset);
	};

	class ENGINE_API resource_ref;

	class ENGINE_API mapped_resource : public virtual resource
	{
	protected:
		mapped_resource() = default;

		inline mapped_resource& operator=(mapped_resource&& other) noexcept
		{
			resource::operator=(std::move(other));
			data_ptr = std::exchange(other.data_ptr, nullptr);
			offset = std::exchange(other.offset, 0);
			return *this;
		}

		inline mapped_resource& move_no_base(mapped_resource&& other) noexcept
		{
			data_ptr = std::exchange(other.data_ptr, nullptr);
			offset = std::exchange(other.offset, 0);
			return *this;
		}

	public:
		inline resource_ref operator[](std::size_t index) noexcept;

	protected:
		virtual void update_map() = 0;

		void** data_ptr = nullptr;
		std::size_t offset = 0;

		friend class ::ENGINE_NAMESPACE::resource_ref;
	};

	class ENGINE_API resource_ref
	{
	public:
		inline void set(void* data) noexcept
		{
			std::memcpy(reinterpret_cast<std::byte*>(*ref->data_ptr) + ref->offset + ref->layout().size() * index, data, 
				ref->layout().size());
		}

		inline void set_field(std::size_t field_idx, void* data)
		{

		}

	private:
		resource_ref() = delete;
		resource_ref(mapped_resource& ref, std::size_t index) noexcept : ref(&ref), index(index)
		{ }

		mapped_resource* ref;
		std::size_t index;

		friend class ::ENGINE_NAMESPACE::mapped_resource;
	};

	inline resource_ref mapped_resource::operator[](std::size_t index) noexcept
	{
		return resource_ref(*this, index);
	}

	class ENGINE_API resizable_resource : public virtual resource
	{
	protected:
		resizable_resource() = default;
		resizable_resource(memory_ref&& ref, const data_layout& layout, std::uint32_t count);

		inline resizable_resource& operator=(resizable_resource&& other) noexcept
		{
			resource::operator=(std::move(other));
			return *this;
		}

		virtual void resize(std::uint32_t new_size) = 0;
	};

	class ENGINE_API uniform final : public mapped_resource
	{
	public:
		uniform() = default;
		uniform(const data_layout& layout, std::uint32_t count = 1);

		//inline uniform& operator=(uniform&& other) noexcept
		//{
		//	mapped_resource::operator=(std::move(other));
		//	return *this;
		//}

	protected:
		void update_map() override;
	};

	class ENGINE_API unmapped_uniform final : public unmapped_resource
	{
	public:
		unmapped_uniform() = default;
		unmapped_uniform(const data_layout& layout, std::uint32_t count = 1);
	};

	class ENGINE_API storage_array final : public unmapped_resource
	{
	public:
		storage_array() = default;
		storage_array(const data_layout& layout, std::uint32_t count);
	};

	class ENGINE_API dynamic_storage_array final : public mapped_resource
	{
	public:
		dynamic_storage_array() = default;
		dynamic_storage_array(const data_layout& layout, std::uint32_t count);

	protected:
		void update_map() override;
	};

	class ENGINE_API storage_vector final : public unmapped_resource, public resizable_resource
	{
	public:
		storage_vector() = default;
		storage_vector(const data_layout& layout, std::uint32_t count);

		inline storage_vector& operator=(storage_vector&& other) noexcept
		{
			resource::operator=(std::move(other));
			return *this;
		}

		void resize(std::uint32_t new_count) override;
	};

	class ENGINE_API dynamic_storage_vector final : public mapped_resource, public resizable_resource
	{
	public:
		dynamic_storage_vector() = default;
		dynamic_storage_vector(const data_layout& layout, std::uint32_t count);

		inline dynamic_storage_vector& operator=(dynamic_storage_vector&& other) noexcept
		{
			resource::operator=(std::move(other));
			move_no_base(std::move(other));
			return *this;
		}

		void resize(std::uint32_t new_count) override;

	protected:
		void update_map() override;
	};

	class ENGINE_API texture_resource : public resource
	{
	public:


		texture_resource(const texture_resource&) = delete;
		texture_resource& operator=(const texture_resource&) = delete;

	private:

	};
}

template<>
struct std::hash<ENGINE_NAMESPACE::resource>
{
	inline std::size_t operator()(const ENGINE_NAMESPACE::resource& key) const noexcept
	{
		return key.ref.hash();
	}
};
