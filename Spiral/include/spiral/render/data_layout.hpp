#pragma once

#define SET_TYPE_S(t1, t2, t3)									\
template<>														\
inline void set_type_s<t1>(std::uint8_t index) noexcept			\
{																\
	set_type(index, t2, t3);									\
}


namespace Spiral
{
	class data_layout
	{
	public:
		enum class type : std::uint8_t
		{
			SCALAR = 0,
			VEC2,
			VEC3,
			VEC4,
			MAT2,
			MAT3,
			MAT4
		};

		enum class component_type : std::uint8_t
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

		data_layout() = delete;

		data_layout(std::uint8_t reserve) : n_values(reserve)
		{
			values = t_malloc<value>(reserve);
		}

		inline void set_type(std::uint8_t index, type t, component_type ct) noexcept
		{
			values[index] = { t, ct };
		}

		template<typename Type>
		inline void set_type_s(std::uint8_t index) noexcept
		{
			static_assert(false, "Type incompatible with data layout");
		}

		//scalars
		SET_TYPE_S(std::int8_t, type::SCALAR, component_type::INT8)
		SET_TYPE_S(std::int16_t, type::SCALAR, component_type::INT16)
		SET_TYPE_S(std::int32_t, type::SCALAR, component_type::INT32)
		SET_TYPE_S(std::int64_t, type::SCALAR, component_type::INT64)
		SET_TYPE_S(std::uint8_t, type::SCALAR, component_type::UINT8)
		SET_TYPE_S(std::uint16_t, type::SCALAR, component_type::UINT16)
		SET_TYPE_S(std::uint32_t, type::SCALAR, component_type::UINT32)
		SET_TYPE_S(std::uint64_t, type::SCALAR, component_type::UINT64)
		SET_TYPE_S(float, type::SCALAR, component_type::FLOAT32)
		SET_TYPE_S(double, type::SCALAR, component_type::FLOAT64)

		template<typename Type, typename... Types, std::uint8_t Index = 0>
		inline void set_types_s()
		{
			set_type_s<Type>(Index);
			if constexpr (!sizeof...(Types))
			{
				set_types_s<Types..., Index + 1>();
			}
		}

		template<typename... Types>
		static inline data_layout create()
		{
			static_assert(CHAR_BIT * sizeof(float) == 32, "Float type must have 32 bit length.");
			static_assert(CHAR_BIT * sizeof(double) == 64, "Double type must have 64 bit length.");

			data_layout layout(sizeof...(Types));
			layout.set_types_s<Types...>();
			return layout;
		}

		data_layout(const data_layout& other) : n_values(other.n_values), values(t_malloc<value>(other.n_values))
		{
			std::memcpy(values, &other.n_values, n_values * sizeof(value));
		}

		data_layout(data_layout&& other) noexcept 
			: n_values(std::exchange(other.n_values, 0)), values(std::exchange(other.values, nullptr)) { }

		data_layout& operator=(const data_layout& other)
		{
			if (this != &other)
			{
				n_values = other.n_values;
				values = t_malloc<value>(other.n_values);
				std::memcpy(values, &other.n_values, n_values * sizeof(value));
			}
			return *this;
		}

		data_layout& operator=(data_layout&& other) noexcept
		{
			n_values = std::exchange(other.n_values, 0);
			values = std::exchange(other.values, nullptr);
			return *this;
		}

		~data_layout() 
		{
			n_values = 0;
			std::free(values);
		}

		inline bool operator==(const data_layout& rhs) const noexcept
		{
			if (this->n_values == rhs.n_values && this->n_values != 0)
			{
				for (std::uint8_t i = 0; i < this->n_values; i++)
				{
					if (!(this->values[i] == rhs.values[i])) return false;
				}
				return true;
			}
			return false;
		}

		inline std::uint8_t count() const noexcept { return n_values; }

		inline std::size_t size() const noexcept
		{
			std::size_t size = 0;
			for (std::uint8_t i = 0; i < n_values; i++)
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

			inline std::uint32_t size() const noexcept
			{
				std::uint32_t size = 0;
				std::uint32_t m = 0;
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
					assertm(false, "Unknown type.");
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
					assertm(false, "Unknown component type.");
					break;
				}
				return size;
			}

			inline bool operator==(const value& rhs) const noexcept { return this->t == rhs.t && this->ct == rhs.ct; }
		};

		inline const value& operator[](std::uint8_t index) const noexcept { return values[index]; }

	private:
		std::uint8_t n_values = 0;
		value* values = nullptr;
	};
}

#undef SET_TYPE_S
