#pragma once

#include "data_layout.hpp"

#include <hardcore/core/core.hpp>

#include <vector>

namespace ENGINE_NAMESPACE
{
	namespace shader_ext
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

	enum class shader_t : std::uint8_t
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
		shader() = default;
		
		shader(const char* filename, const char* entry_point, const char* name, shader_t stage = shader_t::NONE);
		shader(const char* filename, const char* entry_point, shader_t stage = shader_t::NONE) : shader(filename, entry_point, filename, stage) {}

		shader(const char* name, const char* entry_point, shader_t stage, const char* code);

		~shader();

		shader(shader&& other) noexcept : data(std::exchange(other.data, nullptr)), size(std::exchange(other.size, 0)),
			stage(std::exchange(other.stage, shader_t::NONE)), entry_point(std::exchange(other.entry_point, nullptr)),
			_inputs(std::move(other._inputs)), _descriptors(std::move(other._descriptors)),
			_name(std::exchange(other._name, nullptr))
		{ }

		shader& operator=(shader&& other) noexcept
		{
			this->~shader();

			data = std::exchange(other.data, nullptr);
			size = std::exchange(other.size, 0);
			stage = std::exchange(other.stage, shader_t::NONE);
			entry_point = std::exchange(other.entry_point, nullptr);
			_inputs = std::move(other._inputs);
			_descriptors = std::move(other._descriptors);
			_name = std::exchange(other._name, nullptr);
			return *this;
		}

		shader(const shader&) = delete;
		shader& operator=(const shader&) = delete;

		const char* name() const noexcept { return _name; }
		const data_layout* inputs(std::size_t& out_size) const noexcept;

		//uniform types
		enum descriptor_t : std::uint8_t
		{
			NONE = 0,
			UNIFORM,
			STORAGE,
			//PUSH_CONSTANT,
			SAMPLER,
			TEXTURE,
			IMAGE,
			SAMPLER_BUFFER,
			IMAGE_BUFFER,
			SAMPLER_SHADOW,
		};

		//for shader variables, unlike shader inputs, there wont be a representation for every possible set/binding pair
		//because a shader wont necessarily have all sets and bindings declared, some can be in a different shader in the
		//same pipeline
		struct descriptor_data
		{
			std::uint32_t set;
			std::uint32_t binding;
			descriptor_t type;
			std::uint32_t count;
			//data_layout layout;
		};

	//this is protected to help with some operations in shader_internal
	protected:
		std::uint32_t* data = nullptr;
		std::size_t size = 0; //data size in bytes
		shader_t stage = shader_t::NONE;
		char* entry_point = nullptr;

		std::vector<data_layout> _inputs;
		std::vector<descriptor_data> _descriptors;

	private:
		char* _name = nullptr;

		void reflect();
	};
}
