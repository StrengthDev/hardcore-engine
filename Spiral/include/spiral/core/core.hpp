#pragma once

#ifdef _MSC_VER

#ifdef SPIRAL_BUILD
	#define SPIRAL_API __declspec(dllexport)
#else
	#define SPIRAL_API __declspec(dllimport)
#endif // SPIRAL_BUILD

#ifdef NDEBUG
#define DEBUG_BREAK
#define INTERNAL_ASSERT(exp, msg)
#else
#include <cassert>
#define DEBUG_BREAK __debugbreak();
#define INTERNAL_ASSERT(exp, msg) assert(exp && msg)
#endif // NDEBUG

#else // _MSC_VER

#ifdef SPIRAL_BUILD
#define SPIRAL_API //TODO: export for other platforms
#else
#define SPIRAL_API //TODO: import for other platforms
#endif // SPIRAL_BUILD

#ifdef NDEBUG
#define DEBUG_BREAK //TODO: debug break for other platforms
#define INTERNAL_ASSERT(exp, msg)
#else
#include <cassert>
#define DEBUG_BREAK
#define INTERNAL_ASSERT(exp, msg)
#endif // NDEBUG

#endif 



#include <memory>

#define BIT(x) (1 << x)

#define KILOBYTES(x) x * BIT(10)
#define MEGABYTES(x) KILOBYTES(x) * BIT(10)
#define GIGABYTES(x) MEGABYTES(x) * BIT(10)

namespace Spiral
{
	/**
	 * @brief Allocates uninitialized memory.
	 * @param size Number of bytes to allocate.
	 * @return Pointer to the allocated memory.
	 * @exception std::bad_alloc If any error occurs with the allocation.
	*/
	inline void* ex_malloc(std::size_t size)
	{
		void* p = std::malloc(size);
		if (!p)
		{
			throw std::bad_alloc();
		}
		return p;
	}

	/**
	 * @brief Allocates memory with all bits initialized to 0.
	 * @param count Number of objects to allocate.
	 * @param size Size of each object in bytes.
	 * @return Pointer to the allocated memory.
	 * @exception std::bad_alloc If any error occurs with the allocation.
	*/
	inline void* ex_calloc(std::size_t count, std::size_t size)
	{
		void* p = std::calloc(count, size);
		if (!p)
		{
			throw std::bad_alloc();
		}
		return p;
	}

	/**
	 * @brief Reallocates the memory of a pointer.
	 * @param ptr Pointer to already allocated memory.
	 * @param size Number of bytes in the new allocation.
	 * @return Pointer to the allocated memory.
	 * @exception std::bad_alloc If any error occurs with the allocation.
	*/
	inline void* ex_realloc(void* ptr, std::size_t size)
	{
		void* np = std::realloc(ptr, size);
		if (!np)
		{
			throw std::bad_alloc();
		}
		return np;
	}

	/**
	 * @brief Allocates uninitialized memory.
	 * @tparam Type Type of objects contained in the allocated memory.
	 * @param count Number of objects in the allocated memory.
	 * @return Pointer to the allocated memory.
	 * @exception std::bad_alloc If any error occurs with the allocation.
	*/
	template<typename Type>
	inline Type* t_malloc(std::size_t count)
	{
		return reinterpret_cast<Type*>(ex_malloc(count * sizeof(Type)));
	}

	/**
	 * @brief Allocates memory with all bits initialized to 0.
	 * @tparam Type Type of objects contained in the allocated memory.
	 * @param count Number of objects in the allocated memory.
	 * @return Pointer to the allocated memory.
	 * @exception std::bad_alloc If any error occurs with the allocation.
	*/
	template<typename Type>
	inline Type* t_calloc(std::size_t count)
	{
		return reinterpret_cast<Type*>(ex_calloc(count, sizeof(Type)));
	}

	/**
	 * @brief Reallocates the memory of a pointer.
	 * @tparam Type Type of objects contained in the allocated memory.
	 * @param count Number of objects in the allocated memory.
	 * @return Pointer to the allocated memory.
	 * @exception std::bad_alloc If any error occurs with the allocation.
	*/
	template<typename Type>
	inline Type* t_realloc(void* ptr, std::size_t count)
	{
		return reinterpret_cast<Type*>(ex_realloc(ptr, count * sizeof(Type)));
	}

	static const char* ENGINE_NAME = "Spiral Engine";
	static const unsigned int MAJOR_VERSION = 1;
	static const unsigned int MINOR_VERSION = 0;
	static const unsigned int PATCH_VERSION = 0;

	struct program_id
	{
		const char* name;
		unsigned int major;
		unsigned int minor;
		unsigned int patch;
	};
}
