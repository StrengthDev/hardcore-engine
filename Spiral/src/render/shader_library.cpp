#include <pch.hpp>

#include <spiral/render/shader_library.hpp>

std::unordered_map<std::string, Spiral::shader> shaders;

namespace Spiral
{
	/*
	void shader_library::init()
	{ }

	void shader_library::terminate()
	{
		shaders.clear();
	}
	*/

	const shader& shader_library::add(shader&& s)
	{
		const char* name = s.get_name();
		shaders[name] = std::move(s);
		return shaders[name];
	}

	const shader& shader_library::get(const char* name)
	{
		return shaders.at(name);
	}

	bool shader_library::has(const char* name)
	{
		//C++20 has map::contains, could consider upgrading (TODO)
		return shaders.find(name) == shaders.end();
	}
}
