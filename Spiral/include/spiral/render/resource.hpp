#pragma once

#include <spiral/core/core.hpp>

#include "memory_reference.hpp"
#include "data_layout.hpp"

#include <utility>

namespace Spiral
{
	class SPIRAL_API resource
	{
	public:
		void free();
		inline bool valid() const { return ref.valid(); }

		resource(const resource&) = delete;
		resource& operator=(const resource&) = delete;

	protected:
		resource() = default;
		virtual ~resource() { free(); }

		//inline void bind(memory_reference&& ref) { ref = std::move(ref); }

		memory_reference ref;
	};

	class SPIRAL_API object_resource : public resource
	{
	public:
		enum class index_format : std::uint8_t
		{
			NONE = 0,
			UINT8, //requires extension
			UINT16,
			UINT32,
		};

		object_resource() = default;
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

		object_resource(const object_resource&) = delete;
		object_resource& operator=(const object_resource&) = delete;

	private:
		const data_layout layout;
		const index_format index_type;
	};

	class SPIRAL_API texture_resource : public resource
	{
	public:


		texture_resource(const texture_resource&) = delete;
		texture_resource& operator=(const texture_resource&) = delete;

	private:

	};
}
