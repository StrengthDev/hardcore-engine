#pragma once

#ifdef SPIRAL_BUILD
	#define SPIRAL_API __declspec(dllexport)
#else
	#define SPIRAL_API __declspec(dllimport)
#endif // SPIRAL_BUILD

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak();
#else
#define DEBUG_BREAK
#endif // _MSC_VER



#define BIT(x) (1 << x)

namespace Spiral
{
	struct ECProperties //Engine/Client properties
	{
		const char* name;
		unsigned int major;
		unsigned int minor;
		unsigned int patch;
	};
}
