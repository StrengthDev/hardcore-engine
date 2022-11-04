#pragma once

#include <spiral/core/core.hpp>

#include "memory_ref.hpp"
#include "data_layout.hpp"

#include <utility>

namespace ENGINE_NAMESPACE
{
	class ENGINE_API resource
	{
	public:
		resource() = default;
		virtual ~resource() { destroy(); }

		void destroy();
		inline bool valid() const { return ref.valid(); }

		resource(const resource&) = delete;
		resource& operator=(const resource&) = delete;

		resource(resource&& other) noexcept : ref(std::move(other.ref)) { }

		inline resource& operator=(resource&& other) noexcept
		{
			destroy();
			ref = std::move(other.ref);
			return *this;
		}

	protected:

		memory_ref ref;
	};

	class ENGINE_API object_resource : public resource
	{
	public:
		object_resource() = default;

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

		object_resource(const void* data, const std::size_t size, const std::size_t offset, const bool vertex_data_first,
			const index_format index_type, const data_layout& layout);
		object_resource(const void* vertex_data, const std::size_t vertex_data_size, const void* index_data,
			const std::size_t index_data_size, const index_format index_type, const data_layout& layout);
		object_resource(const void* vertex_data, const std::size_t vertex_data_size, const data_layout& layout);

		object_resource(const std::size_t vertex_data_size, const std::size_t index_data_size,
			const index_format index_type, const data_layout& layout);
		object_resource(const std::size_t vertex_data_size, const data_layout& layout);

		template<typename... Types>
		static inline object_resource create(void* data, std::size_t size, std::size_t offset, bool vertex_data_first, index_format index_type)
		{
			return object_resource(data, size, offset, vertex_data_first, index_type, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline object_resource create(const void* vertex_data, const std::size_t vertex_data_size,
			const void* index_data, const std::size_t index_data_size, const index_format index_type)
		{
			return object_resource(vertex_data, vertex_data_size, index_data, index_data_size, index_type, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline object_resource create(const void* vertex_data, const std::size_t vertex_data_size)
		{
			return object_resource(vertex_data, vertex_data_size, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline object_resource create(const std::size_t vertex_data_size, const std::size_t index_data_size,
			const index_format index_type)
		{
			return object_resource(vertex_data_size, index_data_size, index_type, data_layout::create<Types...>());
		}

		template<typename... Types>
		static inline object_resource create(const std::size_t vertex_data_size)
		{
			return object_resource(vertex_data_size, data_layout::create<Types...>());
		}

		object_resource(object_resource&& other) noexcept : resource(std::move(other)), 
			layout(std::exchange(other.layout, data_layout(0))),
			index_t(std::exchange(other.index_t, index_format::NONE)), draw_count(std::exchange(other.draw_count, 0))
		{ }

		inline object_resource& operator=(object_resource&& other) noexcept
		{
			resource::operator=(std::move(other));
			layout = std::exchange(other.layout, data_layout());
			index_ref = std::exchange(other.index_ref, memory_ref());
			index_t = std::exchange(other.index_t, index_format::NONE);
			draw_count = std::exchange(other.draw_count, 0);
			return *this;
		}

		inline const data_layout& vertex_layout() const noexcept { return layout; }
		inline index_format index_type() const noexcept { return index_t; }
		inline std::uint32_t count() const noexcept { return draw_count; }

	protected:
		memory_ref index_ref;

	private:
		data_layout layout;
		index_format index_t = index_format::NONE;
		std::uint32_t draw_count = 0;
	};

	class ENGINE_API texture_resource : public resource
	{
	public:


		texture_resource(const texture_resource&) = delete;
		texture_resource& operator=(const texture_resource&) = delete;

	private:

	};
}
