#pragma once

#include <hardcore/core/core.hpp>

#include <sstream>

#define SET_TYPE_S(t1, t2, t3)									\
template<>														\
inline void set_type_s<t1>(u8 index) noexcept			\
{																\
	set_type(index, t2, t3);									\
}

namespace ENGINE_NAMESPACE
{
	class ENGINE_API data_layout
	{
	public:
		enum class type : u8
		{
			SCALAR = 0,
			VEC2,
			VEC3,
			VEC4,
			MAT2,
			MAT3,
			MAT4,
			MAT2x3,
			MAT2x4,
			MAT3x2,
			MAT3x4,
			MAT4x2,
			MAT4x3
		};

		enum class component_type : u8
		{
			FLOAT32 = 0,
			FLOAT64,
			FLOAT16,
			INT8,
			INT16,
			INT32,
			INT64,
			UINT8,
			UINT16,
			UINT32,
			UINT64
		};

		//empty layout constructor
		data_layout() = default;

		data_layout(u8 reserve) : n_values(reserve)
		{
			values = t_malloc<value>(reserve);
		}

		~data_layout() 
		{
			n_values = 0;
			std::free(values);
			values = nullptr;
		}

		inline void set_type(u8 index, type t, component_type ct) noexcept
		{
			INTERNAL_ASSERT(index < n_values, "Index out of bounds");
			values[index] = { t, ct };
		}

		template<typename Type>
		inline void set_type_s(u8 index) noexcept
		{
			static_assert(force_eval<Type>::value, "Type incompatible with data layout");
		}

		//scalars
		SET_TYPE_S(i8, type::SCALAR, component_type::INT8)
		SET_TYPE_S(i16, type::SCALAR, component_type::INT16)
		SET_TYPE_S(i32, type::SCALAR, component_type::INT32)
		SET_TYPE_S(i64, type::SCALAR, component_type::INT64)
		SET_TYPE_S(u8, type::SCALAR, component_type::UINT8)
		SET_TYPE_S(u16, type::SCALAR, component_type::UINT16)
		SET_TYPE_S(u32, type::SCALAR, component_type::UINT32)
		SET_TYPE_S(u64, type::SCALAR, component_type::UINT64)
		SET_TYPE_S(float, type::SCALAR, component_type::FLOAT32)
		SET_TYPE_S(double, type::SCALAR, component_type::FLOAT64)
/*
#if __has_include("glm/glm.hpp")
#include "glm/glm.hpp"

		SET_TYPE_S(glm::vec2, type::VEC2, component_type::FLOAT32)
		SET_TYPE_S(glm::vec3, type::VEC3, component_type::FLOAT32)
		SET_TYPE_S(glm::vec4, type::VEC4, component_type::FLOAT32)

#endif // using glm
*/
		template<u8 Index, typename Type, typename... Types>
		inline void set_types_s()
		{
			set_type_s<Type>(Index);
			if constexpr (sizeof...(Types))
			{
				set_types_s<Index + 1, Types...>();
			}
		}

		template<typename... Types>
		static inline data_layout create()
		{
			static_assert(CHAR_BIT * sizeof(float) == 32, "Float type must have 32 bit length.");
			static_assert(CHAR_BIT * sizeof(double) == 64, "Double type must have 64 bit length.");

			data_layout layout(sizeof...(Types));
			layout.set_types_s<0, Types...>();
			return layout;
		}

		data_layout(const data_layout& other) : n_values(other.n_values), values(t_malloc<value>(other.n_values))
		{
			std::memcpy(values, other.values, n_values * sizeof(*values));
		}

		data_layout(data_layout&& other) noexcept 
			: n_values(std::exchange(other.n_values, 0)), values(std::exchange(other.values, nullptr)) { }

		data_layout& operator=(const data_layout& other)
		{
			if (this != &other)
			{
				n_values = other.n_values;
				if (values)
				{
					values = t_realloc<value>(values, other.n_values);
				}
				else
				{
					values = t_malloc<value>(other.n_values);
				}
				std::memcpy(values, other.values, n_values * sizeof(*values));
			}
			return *this;
		}

		data_layout& operator=(data_layout&& other) noexcept
		{
			n_values = std::exchange(other.n_values, 0);
			std::free(values);
			values = std::exchange(other.values, nullptr);
			return *this;
		}

		inline bool operator==(const data_layout& rhs) const noexcept
		{
			if (this->n_values == rhs.n_values)
			{
				for (u8 i = 0; i < this->n_values; i++)
				{
					if (!(this->values[i] == rhs.values[i])) return false;
				}
				return true;
			}
			return false;
		}

		/**
		 * @brief Replaces matrix values with equivalent vector sequences
		 * @param layout Original data_layout to convert
		 * @return Converted layout
		*/
		static inline data_layout vectorize(const data_layout& layout)
		{
			if (layout.n_values == 0) return data_layout();

			data_layout converted(layout.vector_count());
			u8 c = 0;
			for (u8 i = 0; i < layout.n_values; i++)
			{
				data_layout::type converted_type;
				u32 iterations;
				switch (layout[i].t)
				{
				case data_layout::type::MAT2:	iterations = 2; converted_type = data_layout::type::VEC2; break;
				case data_layout::type::MAT2x3:	iterations = 2; converted_type = data_layout::type::VEC3; break;
				case data_layout::type::MAT2x4:	iterations = 2; converted_type = data_layout::type::VEC4; break;
				case data_layout::type::MAT3:	iterations = 3; converted_type = data_layout::type::VEC3; break;
				case data_layout::type::MAT3x2:	iterations = 3; converted_type = data_layout::type::VEC2; break;
				case data_layout::type::MAT3x4:	iterations = 3; converted_type = data_layout::type::VEC4; break;
				case data_layout::type::MAT4:	iterations = 4; converted_type = data_layout::type::VEC4; break;
				case data_layout::type::MAT4x2:	iterations = 4; converted_type = data_layout::type::VEC2; break;
				case data_layout::type::MAT4x3:	iterations = 4; converted_type = data_layout::type::VEC3; break;
				default:
					iterations = 1;
					converted_type = layout[i].t;
					break;
				}

				for (u8 k = 0; k < iterations; k++)
				{
					converted.set_type(c, converted_type, layout[i].ct);
				}
			}
			return converted;
		}

		static inline bool vectorized_equals(const data_layout& x, const data_layout& y)
		{
			if (x.vector_count() == y.vector_count())
			{
				if (x.n_values == 0) return true;

				return vectorize(x) == vectorize(y);
			}
			return false;
		}

		inline u8 count() const noexcept { return n_values; }

		inline u8 vector_count() const noexcept
		{
			u8 count = 0;
			for (u8 i = 0; i < n_values; i++)
			{
				switch (values[i].t)
				{
				case data_layout::type::MAT2:
				case data_layout::type::MAT2x3:
				case data_layout::type::MAT2x4:
					count += 2;
					break;
				case data_layout::type::MAT3:
				case data_layout::type::MAT3x2:
				case data_layout::type::MAT3x4:
					count += 3;
					break;
				case data_layout::type::MAT4:
				case data_layout::type::MAT4x2:
				case data_layout::type::MAT4x3:
					count += 4;
					break;
				default:
					count += 1;
					break;
				}
			}
			return count;
		}

		inline std::size_t size() const noexcept
		{
			std::size_t size = 0;
			for (u8 i = 0; i < n_values; i++)
			{
				size += values[i].size();
			}
			return size;
		}

	protected:
		struct value
		{
			type t;
			component_type ct;

			inline u32 size() const noexcept
			{
				u32 size = 0;
				u32 m = 0;
				switch (t)
				{
				case type::SCALAR:
					m = 1;
					break;
				case type::VEC2:
					m = 2;
					break;
				case type::VEC3:
					m = 3;
					break;
				case type::VEC4:
				case type::MAT2:
					m = 4;
					break;
				case type::MAT3:
					m = 9;
					break;
				case type::MAT4:
					m = 16;
					break;
				default:
					INTERNAL_ASSERT(false, "Unknown type.");
					break;
				}
				switch (ct)
				{
				case component_type::INT8:
				case component_type::UINT8:
					size += m * 1;
					break;
				case component_type::FLOAT16:
				case component_type::INT16:
				case component_type::UINT16:
					size += m * 2;
					break;
				case component_type::FLOAT32:
				case component_type::INT32:
				case component_type::UINT32:
					size += m * 4;
					break;
				case component_type::FLOAT64:
				case component_type::INT64:
				case component_type::UINT64:
					size += m * 8;
					break;
				default:
					INTERNAL_ASSERT(false, "Unknown component type.");
					break;
				}
				return size;
			}

			inline bool operator==(const value& rhs) const noexcept { return this->t == rhs.t && this->ct == rhs.ct; }

			inline std::string to_string()
			{
				std::stringstream stream;
				bool not_scalar = true;
				switch (t)
				{
				case ENGINE_NAMESPACE::data_layout::type::SCALAR:	not_scalar = false;	break;
				case ENGINE_NAMESPACE::data_layout::type::VEC2:		stream << "vec2";	break;
				case ENGINE_NAMESPACE::data_layout::type::VEC3:		stream << "vec3";	break;
				case ENGINE_NAMESPACE::data_layout::type::VEC4:		stream << "vec4";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT2:		stream << "mat2";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT3:		stream << "mat3";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT4:		stream << "mat4";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT2x3:	stream << "mat2x3";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT2x4:	stream << "mat2x4";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT3x2:	stream << "mat3x2";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT3x4:	stream << "mat3x4";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT4x2:	stream << "mat4x2";	break;
				case ENGINE_NAMESPACE::data_layout::type::MAT4x3:	stream << "mat4x3";	break;
				default: INTERNAL_ASSERT(false, "Unimplemented type"); break;
				}
				if (not_scalar) stream << '<';
				switch (ct)
				{
				case ENGINE_NAMESPACE::data_layout::component_type::FLOAT32:	stream << "float";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::FLOAT64:	stream << "double";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::FLOAT16:	stream << "half_float";	break;
				case ENGINE_NAMESPACE::data_layout::component_type::INT8:		stream << "int8";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::INT16:		stream << "int16";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::INT32:		stream << "int32";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::INT64:		stream << "int64";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::UINT8:		stream << "uint8";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::UINT16:		stream << "uint16";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::UINT32:		stream << "uint32";		break;
				case ENGINE_NAMESPACE::data_layout::component_type::UINT64:		stream << "uint64";		break;
				default: INTERNAL_ASSERT(false, "Unimplemented component type"); break;
				}
				if (not_scalar) stream << '>';
				return stream.str();
			}
		};

		inline const value& operator[](u8 index) const noexcept
		{
			INTERNAL_ASSERT(index < n_values, "Index out of bounds");
			return values[index];
		}

	private:
		u8 n_values = 0;
		value* values = nullptr;

		friend std::string to_string(const data_layout&);
		friend std::ostream& operator<<(std::ostream&, const data_layout&);
	};

	inline std::string to_string(const data_layout& layout)
	{
		std::stringstream stream;
		stream << '(';
		for (u8 i = 0; i < layout.n_values; i++)
		{
			stream << layout.values[i].to_string();
			if (i < layout.n_values - 1) stream << ", ";
		}
		stream << ')';
		return stream.str();
	}

	inline std::ostream& operator<<(std::ostream& os, const data_layout& layout)
	{
		os << to_string(layout);
		return os;
	}
}

#undef SET_TYPE_S
