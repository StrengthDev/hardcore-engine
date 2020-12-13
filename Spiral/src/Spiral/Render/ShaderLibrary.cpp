#include "pch.hpp"

#include "ShaderLibrary.hpp"
#include "ShaderAPI.hpp"

Spiral::Shader* shaders;
uint32_t nShaders;
uint32_t capacity;

namespace Spiral
{
	void ShaderLibrary::init()
	{
		capacity = INITIAL_NUM_SHADERS;
		shaders = (Shader*)malloc(sizeof(Shader) * capacity);
		nShaders = 0;
	}

	void ShaderLibrary::terminate()
	{
		for (uint32_t i = 0; i < nShaders; i++)
		{
			free(shaders[i].source);
		}
		free(shaders);
	}

	uint32_t ShaderLibrary::add(Shader s)
	{
		if (capacity < nShaders + 1)
		{
			uint32_t i;
			capacity += INCREASE_STEP;
			Shader* temp = (Shader*)malloc(sizeof(Shader) * capacity);
			for (i = 0; i < nShaders; i++)
			{
				temp[i] = shaders[i];
			}
			free(shaders);
			shaders = temp;
		}
		shaders[nShaders] = s;
		nShaders++;
		return nShaders - 1;
	}

	Shader& ShaderLibrary::get(int i)
	{
		return shaders[i];
	}

	uint32_t ShaderLibrary::load(const char* filename, ShaderType type)
	{
		//TODO: this assumes the file is in SPIR-V format, need to handle uncompiled case
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			DEBUG_BREAK;
			return 0xFFFFFFFF;
		}
		Shader shader = {};
		size_t filesize = (size_t)file.tellg();
		char* t = (char*)malloc(filesize);
		file.seekg(0);
		file.read(t, filesize);
		file.close();
		shader.source = (uint32_t*)t;
		shader.size = filesize;
		switch (type)
		{
		case ShaderType::Vertex:
			shader.type = VK_SHADER_STAGE_VERTEX_BIT;
			SPRL_CORE_DEBUG("Vertex shader loaded (ID-{0} SIZE: {1})", nShaders, filesize);
			break;
		case ShaderType::Fragment:
			shader.type = VK_SHADER_STAGE_FRAGMENT_BIT;
			SPRL_CORE_DEBUG("Fragment shader loaded (ID-{0} SIZE: {1})", nShaders, filesize);
			break;
		}
		return add(shader);
	}
}