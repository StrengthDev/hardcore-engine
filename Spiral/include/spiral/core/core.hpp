#pragma once

#ifdef _MSC_VER

#ifdef SPIRAL_BUILD
	#define SPIRAL_API __declspec(dllexport)
#else
	#define SPIRAL_API __declspec(dllimport)
#endif // SPIRAL_BUILD

#ifdef NDEBUG
#define DEBUG_BREAK __debugbreak();
#else
#define DEBUG_BREAK
#endif // NDEBUG

#else // _MSC_VER

#ifdef SPIRAL_BUILD
#define SPIRAL_API //TODO: export for other platforms
#else
#define SPIRAL_API //TODO: import for other platforms
#endif // SPIRAL_BUILD

#ifdef NDEBUG
#define DEBUG_BREAK //TODO: debug break for other platforms
#else
#define DEBUG_BREAK
#endif // NDEBUG

#endif 



#define BIT(x) (1 << x)

namespace Spiral
{
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
