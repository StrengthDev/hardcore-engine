#pragma once

#include "core.hpp"

#include <type_traits>
#include <utility>
#include <ostream>
#include <string>
#include <stdexcept>

//#define USE_SYMMETRIC_BINARY_OPERATORS
#define DURATION_COMPATIBLE_TYPES std::enable_if_t<std::is_integral<Type>::value || std::is_floating_point<Type>::value, bool> = true

namespace ENGINE_NAMESPACE
{
	typedef double time_t;

	/**
	 * @brief This class defines a duration of time. All related operator functions take arguments in seconds, regardless of type.
	*/
	class ENGINE_API duration
	{ //the class doesn't need to be exported as it's header only, but it is being exported to hide compiler warnings
	private:
		typedef unsigned long long second_t;

	public:
		duration() noexcept : seconds(0), decimal(0) {}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		duration(Type seconds) noexcept : seconds(static_cast<second_t>(seconds)), decimal(0) {}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		duration(Type seconds)
		{
			this->seconds = static_cast<second_t>(seconds);
			this->decimal = seconds - this->seconds;
		}

		duration(const duration& other) noexcept : seconds(other.seconds), decimal(other.decimal) {}
		duration(duration&& other) noexcept : seconds(std::exchange(other.seconds, 0)), decimal(std::exchange(other.decimal, 0)) {}

		duration& operator=(const duration& other) noexcept
		{
			seconds = other.seconds;
			decimal = other.decimal;
			return *this;
		}

		duration& operator=(duration&& other) noexcept
		{
			seconds = std::exchange(other.seconds, 0);
			decimal = std::exchange(other.decimal, 0);
			return *this;
		}

		inline duration& operator+=(const duration& d) noexcept
		{
			this->decimal += d.decimal;
			if (this->decimal >= 1)
			{
				this->decimal -= 1;
				this->seconds += 1 + d.seconds;
			}
			else
			{
				this->seconds += d.seconds;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		inline duration& operator+=(const Type seconds) noexcept
		{
			if (seconds < 0) { return this->operator-=(-seconds); }
			this->seconds += static_cast<second_t>(seconds);
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		inline duration& operator+=(const Type seconds) noexcept
		{
			if (seconds < 0) { return this->operator-=(-seconds); }
			const second_t t = static_cast<second_t>(seconds);
			this->decimal += seconds - t;
			if (this->decimal >= 1)
			{
				this->decimal -= 1;
				this->seconds += 1 + t;
			}
			else
			{
				this->seconds += t;
			}
			return *this;
		}

		inline duration& operator-=(const duration& d) noexcept
		{
			second_t sub = d.seconds;
			this->decimal -= d.decimal;
			if (this->decimal < 0)
			{
				this->decimal += 1;
				sub++;
			}
			if (this->seconds < sub)
			{
				this->seconds = 0;
				this->decimal = 0;
			}
			else
			{
				this->seconds -= sub;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		inline duration& operator-=(const Type seconds) noexcept
		{
			if (seconds < 0) { return this->operator+=(-seconds); }
			const second_t sub = static_cast<second_t>(seconds);
			if (this->seconds < sub)
			{
				this->seconds = 0;
				this->decimal = 0;
			}
			else
			{
				this->seconds -= sub;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		inline duration& operator-=(const Type seconds) noexcept
		{
			if (seconds < 0) { return this->operator+=(-seconds); }
			second_t sub = static_cast<second_t>(seconds);
			this->decimal -= (seconds - sub);
			if (this->decimal < 0)
			{
				this->decimal += 1;
				sub++;
			}
			if (this->seconds < sub)
			{
				this->seconds = 0;
				this->decimal = 0;
			}
			else
			{
				this->seconds -= sub;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		inline duration& operator*=(const Type mult)
		{
			if (mult < 0) { throw std::invalid_argument("multiplier cannot be negative"); }
			this->seconds *= mult;
			const time_t dec = this->decimal * mult;
			if (dec >= 1)
			{
				const second_t sec = static_cast<second_t>(dec);
				this->seconds += sec;
				this->decimal = dec - sec;
			}
			else
			{
				this->decimal = dec;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		inline duration& operator*=(const Type mult)
		{
			if (mult < 0) { throw std::invalid_argument("multiplier cannot be negative"); }
			const time_t cmult = static_cast<time_t>(mult);
			const time_t sec = cmult * this->seconds;
			this->seconds = static_cast<second_t>(sec);
			const time_t dec = cmult * this->decimal + (sec - this->seconds);
			if (dec >= 1)
			{
				const second_t sub = static_cast<second_t>(dec);
				this->seconds += sub;
				this->decimal = dec - sub;
			}
			else
			{
				this->decimal = dec;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		inline duration& operator/=(const Type div)
		{
			if (div < 0) { throw std::invalid_argument("divisor cannot be negative"); }
			this->seconds /= div;
			const time_t dec = (this->decimal + (this->seconds % div)) / div;
			if (dec >= 1)
			{
				const second_t sec = static_cast<second_t>(dec);
				this->seconds += sec;
				this->decimal = dec - sec;
			}
			else
			{
				this->decimal = dec;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		inline duration& operator/=(const Type div)
		{
			if (div < 0) { throw std::invalid_argument("divisor cannot be negative"); }
			const time_t cdiv = static_cast<time_t>(div);
			const time_t sec = static_cast<time_t>(this->seconds) / cdiv;
			this->seconds = static_cast<second_t>(sec);
			const time_t dec = this->decimal / cdiv + (sec - this->seconds);
			if (dec >= 1)
			{
				const second_t sub = static_cast<second_t>(dec);
				this->seconds += sub;
				this->decimal = dec - sub;
			}
			else
			{
				this->decimal = dec;
			}
			return *this;
		}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		inline time_t operator%(const Type mod) const noexcept
		{
#ifdef DISABLE_MODULO_CORRECTION
			return static_cast<time_t>(seconds % static_cast<second_t>(mod)) + decimal;
#else
			const time_t res = static_cast<time_t>(seconds % static_cast<second_t>(mod)) + decimal;
			return res + (mod < 0 && res) ? mod : 0;
#endif // DISABLE_MODULO_CORRECTION
		}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		inline time_t operator%(const Type mod) const
		{
#ifdef DISABLE_MODULO_CORRECTION
			const time_t cmod = static_cast<time_t>(mod);
			return std::fmod(std::fmod(static_cast<time_t>(seconds), cmod) + std::fmod(decimal, cmod), cmod);
#else
			const time_t cmod = static_cast<time_t>(mod);
			const time_t res = std::fmod(std::fmod(static_cast<time_t>(seconds), cmod) + std::fmod(decimal, cmod), cmod);
			return res + (cmod < 0 && std::abs(res) >= std::numeric_limits<double>::epsilon()) ? cmod : 0;
#endif // DISABLE_MODULO_CORRECTION
		}

		inline static std::int8_t cmp(const duration& l, const duration& r) noexcept
		{
			if (l.seconds == r.seconds)
			{
				if (l.decimal == r.decimal)
				{
					return 0;
				}
				else if (l.decimal < r.decimal)
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
			else
			{
				if (l.seconds < r.seconds)
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
		}

		template<typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
		inline static std::int8_t cmp(const duration& l, const Type r) noexcept
		{
			if (l.seconds == r)
			{
				if (l.decimal == 0)
				{
					return 0;
				}
				else
				{
					return 1;
				}
			}
			else
			{
				if (l.seconds < r)
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
		}

		template<typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
		inline static std::int8_t cmp(const duration& l, const Type r) noexcept
		{
			const second_t r_seconds = static_cast<second_t>(r);
			const time_t r_decimal = static_cast<time_t>(r) - r_seconds;
			if (l.seconds == r_seconds)
			{
				if (l.decimal == r_decimal)
				{
					return 0;
				}
				else if (l.decimal < r_decimal)
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
			else
			{
				if (l.seconds < r_seconds)
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
		}

	private:
		static const int text_precision = 4;

		second_t seconds;
		time_t decimal;

		friend std::string to_string(const duration&);
		friend std::ostream& operator<<(std::ostream&, const duration&);
	};

	inline std::string to_string(const duration& d)
	{
		const std::string dec_string = std::to_string(d.decimal);
		const std::size_t i = dec_string.find('.', 0);
		return std::to_string(d.seconds).append(dec_string.substr(i, duration::text_precision + 1)); //as the function is defined in the header without an export, it should be fine to return the string, it shouldn't cross the dll boundary
	}

	inline std::ostream& operator<<(std::ostream& os, const duration& d)
	{
		os << to_string(d);
		return os;
	}

	inline duration operator+(duration d, const duration& o)
	{
		d += o;
		return d;
	}
	
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline duration operator+(duration d, const Type seconds)
	{
		d += seconds;
		return d;
	}

	inline duration operator-(duration d, const duration& o)
	{
		d -= o;
		return d;
	}

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline duration operator-(duration d, const Type seconds)
	{
		d -= seconds;
		return d;
	}

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline duration operator*(duration d, const Type mult)
	{
		d *= mult;
		return d;
	}

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline duration operator/(duration d, const Type div)
	{
		d /= div;
		return d;
	}

#ifdef USE_SYMMETRIC_BINARY_OPERATORS

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline duration operator+(const Type seconds, duration d)
	{
		d += seconds;
		return d;
	}

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline duration operator-(const Type seconds, duration d)
	{
		duration n(seconds);
		n -= d;
		return n;
	}

#endif // USE_SYMMETRIC_BINARY_OPERATORS
#undef USE_SYMMETRIC_BINARY_OPERATORS

	inline bool operator==(const duration& lhs, const duration& rhs) noexcept { return duration::cmp(lhs, rhs) == 0; }
	inline bool operator!=(const duration& lhs, const duration& rhs) noexcept { return duration::cmp(lhs, rhs) != 0; }
	inline bool operator< (const duration& lhs, const duration& rhs) noexcept { return duration::cmp(lhs, rhs) <  0; }
	inline bool operator> (const duration& lhs, const duration& rhs) noexcept { return duration::cmp(lhs, rhs) >  0; }
	inline bool operator<=(const duration& lhs, const duration& rhs) noexcept { return duration::cmp(lhs, rhs) <= 0; }
	inline bool operator>=(const duration& lhs, const duration& rhs) noexcept { return duration::cmp(lhs, rhs) >= 0; }

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator==(const duration& lhs, const Type rhs) noexcept { return duration::cmp(lhs, rhs) == 0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator!=(const duration& lhs, const Type rhs) noexcept { return duration::cmp(lhs, rhs) != 0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator< (const duration& lhs, const Type rhs) noexcept { return duration::cmp(lhs, rhs) <  0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator> (const duration& lhs, const Type rhs) noexcept { return duration::cmp(lhs, rhs) >  0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator<=(const duration& lhs, const Type rhs) noexcept { return duration::cmp(lhs, rhs) <= 0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator>=(const duration& lhs, const Type rhs) noexcept { return duration::cmp(lhs, rhs) >= 0; }

	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator==(const Type lhs, const duration& rhs) noexcept { return duration::cmp(rhs, lhs) == 0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator!=(const Type lhs, const duration& rhs) noexcept { return duration::cmp(rhs, lhs) != 0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator< (const Type lhs, const duration& rhs) noexcept { return duration::cmp(rhs, lhs) >  0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator> (const Type lhs, const duration& rhs) noexcept { return duration::cmp(rhs, lhs) <  0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator<=(const Type lhs, const duration& rhs) noexcept { return duration::cmp(rhs, lhs) >= 0; }
	template<typename Type, DURATION_COMPATIBLE_TYPES>
	inline bool operator>=(const Type lhs, const duration& rhs) noexcept { return duration::cmp(rhs, lhs) <= 0; }

}

#undef DURATION_COMPATIBLE_TYPES
