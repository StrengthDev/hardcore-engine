#pragma once

#include <spiral/core/core.hpp>

#include "data_layout.hpp"

namespace ENGINE_NAMESPACE
{
	namespace shader_extensions
	{
		static const char VERTEX[]					= ".vert";
		static const char FRAGMENT[]				= ".frag";
		static const char COMPUTE[]					= ".comp";
		static const char MESH[]					= ".mesh";
		static const char TESSELATION_CONTROL[]		= ".tesc";
		static const char TESSELATION_EVALUATION[]	= ".tese";
		static const char GEOMETRY[]				= ".geom";
		static const char TASK[]					= ".task";
		static const char RAY_GENERATION[]			= ".rgen";
		static const char RAY_INTERSECTION[]		= ".rint";
		static const char RAY_ANY_HIT[]				= ".rahit";
		static const char RAY_CLOSEST_HIT[]			= ".rchit";
		static const char RAY_MISS[]				= ".rmiss";
		static const char RAY_CALLABLE[]			= ".rcall";


		static const char SPIRV[] = ".spv";
		static const char CONFIG[] = ".conf";
	}

	enum class shader_t : uint8_t
	{
		NONE = 0,
		VERTEX, FRAGMENT,
		COMPUTE,
		TESSELATION_CONTROL, TESSELATION_EVALUATION,
		GEOMETRY,
		RAY_GENERATION, RAY_INTERSECTION, RAY_ANY_HIT, RAY_CLOSEST_HIT, RAY_MISS, RAY_CALLABLE,
		MESH, TASK,
	};

	class ENGINE_API shader
	{
	public:
		shader() {};
		
		shader(const char* filename, const char* entry_point, const char* name, shader_t stage = shader_t::NONE);
		shader(const char* filename, const char* entry_point, shader_t stage = shader_t::NONE) : shader(filename, entry_point, filename, stage) {}

		shader(const char* name, const char* entry_point, shader_t stage, const char* code);

		~shader();

		shader(shader&& other) noexcept;
		shader& operator=(shader&& other) noexcept;

		shader(const shader& other) = delete;
		shader& operator=(const shader& other) = delete;

		const char* get_name() const noexcept { return name; }
		const data_layout* get_inputs() const noexcept { return inputs; }

	//this is protected to help with some operations in shader_internal
	protected:
		std::uint32_t* data = nullptr;
		std::size_t size = 0; //data size in bytes
		shader_t stage = shader_t::NONE;
		char* entry_point = nullptr;

		std::uint8_t n_inputs = 0;
		data_layout* inputs = nullptr;


	private:
		char* name = nullptr;
	};
}
