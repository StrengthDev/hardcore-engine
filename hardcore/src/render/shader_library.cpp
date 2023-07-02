#include <pch.hpp>

#include <render/shader_library.hpp>

std::unordered_map<std::string, ENGINE_NAMESPACE::shader> shaders;

namespace ENGINE_NAMESPACE
{
	namespace shader_library
	{
		const shader& add(shader&& s)
		{
			const char* name = s.name();
			shaders.insert(std::pair(name, std::move(s)));
			return shaders[name];
		}

		const shader& get(const char* name)
		{
			return shaders.at(name);
		}

		bool has(const char* name)
		{
			//C++20 has map::contains, could consider upgrading (TODO)
			return shaders.find(name) == shaders.end();
		}

		void clear()
		{
			shaders.clear();
		}
	}
}
