#include <pch.hpp>

#include <render/renderer_internal.hpp>
#include <render/shader.hpp>

#include <debug/log_internal.hpp>
#include <io/file.hpp>

#include <shaderc/shaderc.h>
#include <spirv_cross/spirv_reflect.hpp>

namespace ENGINE_NAMESPACE
{
	inline shader_t stage_from_filename(const char* filename)
	{
		std::string fn(filename);
		std::size_t offset_0 = fn.rfind('.');
		std::string extension = fn.substr(offset_0);

		if (extension == shader_ext::SPIRV)
		{
			std::size_t offset_1 = fn.rfind('.', offset_0 - 1);
			extension = fn.substr(offset_1, offset_0 - offset_1);
		}

		if (extension == shader_ext::VERTEX)					return shader_t::VERTEX;
		if (extension == shader_ext::FRAGMENT)					return shader_t::FRAGMENT;
		if (extension == shader_ext::COMPUTE)					return shader_t::COMPUTE;
		if (extension == shader_ext::TESSELATION_CONTROL)		return shader_t::TESSELATION_CONTROL;
		if (extension == shader_ext::TESSELATION_EVALUATION)	return shader_t::TESSELATION_EVALUATION;
		if (extension == shader_ext::GEOMETRY)					return shader_t::GEOMETRY;
		if (extension == shader_ext::RAY_GENERATION)			return shader_t::RAY_GENERATION;
		if (extension == shader_ext::RAY_INTERSECTION)			return shader_t::RAY_INTERSECTION;
		if (extension == shader_ext::RAY_ANY_HIT)				return shader_t::RAY_ANY_HIT;
		if (extension == shader_ext::RAY_CLOSEST_HIT)			return shader_t::RAY_CLOSEST_HIT;
		if (extension == shader_ext::RAY_MISS)					return shader_t::RAY_MISS;
		if (extension == shader_ext::RAY_CALLABLE)				return shader_t::RAY_CALLABLE;
		if (extension == shader_ext::MESH)						return shader_t::MESH;
		if (extension == shader_ext::TASK)						return shader_t::TASK;

		INTERNAL_ASSERT(false, "Unknown shader type");
		return shader_t::NONE;
	}

	/**
	 * @brief Helper function to allocate and copy an identical C string. The returned copy MUST be manually freed.
	 * @param name C string to copy
	 * @return Newly allocated C string copy
	*/
	char* create_cstr(const char* name)
	{
		const std::size_t size = std::strlen(name) + 1;
		char* res = t_malloc<char>(size);
		std::memcpy(res, name, size * sizeof(*res));
		return res;
	}

	inline shaderc_shader_kind to_shaderc(shader_t stage)
	{
		switch (stage)
		{
		case shader_t::VERTEX:					return shaderc_vertex_shader;
		case shader_t::FRAGMENT:				return shaderc_fragment_shader;
		case shader_t::COMPUTE:					return shaderc_compute_shader;
		case shader_t::TESSELATION_CONTROL:		return shaderc_tess_control_shader;
		case shader_t::TESSELATION_EVALUATION:	return shaderc_tess_evaluation_shader;
		case shader_t::GEOMETRY:				return shaderc_geometry_shader;
		case shader_t::RAY_GENERATION:			return shaderc_raygen_shader;
		case shader_t::RAY_INTERSECTION:		return shaderc_intersection_shader;
		case shader_t::RAY_ANY_HIT:				return shaderc_anyhit_shader;
		case shader_t::RAY_CLOSEST_HIT:			return shaderc_closesthit_shader;
		case shader_t::RAY_MISS:				return shaderc_miss_shader;
		case shader_t::RAY_CALLABLE:			return shaderc_callable_shader;
		case shader_t::MESH:					return shaderc_mesh_shader;
		case shader_t::TASK:					return shaderc_task_shader;
		default:
			INTERNAL_ASSERT(false, "Invalid shader type");
		}
		return shaderc_glsl_infer_from_source;
	}

	inline void compile_shader(const char* code, std::size_t code_size, const char* entry_point, shader_t stage, 
		const char* error_name, u32** out_data, std::size_t* out_datasize)
	{
		shaderc_compiler_t compiler = shaderc_compiler_initialize();

		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		//TODO: add options, maybe

		shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, code, code_size, to_shaderc(stage),
			error_name, entry_point, options);

		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		LOG_INTERNAL_ERROR(shaderc_result_get_error_message(result))

		std::size_t n_errors = shaderc_result_get_num_errors(result);
		if (n_errors)
		{
			LOG_INTERNAL_ERROR(n_errors << " error(s) during shader compilation. [" << error_name << ']')
		}

		switch (shaderc_result_get_compilation_status(result))
		{
		case shaderc_compilation_status_success:
			LOG_INTERNAL_INFO("Sucessfuly compiled shader:\t" << error_name)
			break;
		case shaderc_compilation_status_invalid_stage:  // error stage deduction
			LOG_INTERNAL_ERROR("Failed to deduce correct shader stage. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_compilation_error:
			LOG_INTERNAL_ERROR("Shader compilation failed due to code errors. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_internal_error:  // unexpected failure
			LOG_INTERNAL_ERROR("Unexpected error during shader compilation. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_null_result_object: //this shouldn't happen
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_invalid_assembly:
			LOG_INTERNAL_ERROR("Invalid shader assembly. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_validation_error:
			LOG_INTERNAL_ERROR("Shader compilation failed due to validation errors. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_transformation_error:
			LOG_INTERNAL_ERROR("A transformation error has occured during shader compilation. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		case shaderc_compilation_status_configuration_error:
			LOG_INTERNAL_ERROR("Invalid configuration for shader compilation. [" << error_name << ']')
			DEBUG_BREAK;
			break;
		default:
			INTERNAL_ASSERT(false, "Unhandled error in shader compilation");
		}

		std::size_t datasize = shaderc_result_get_length(result);
		*out_data = static_cast<u32*>(ex_malloc(datasize));
		memcpy(*out_data, shaderc_result_get_bytes(result), datasize);
		*out_datasize = datasize;

		shaderc_result_release(result);
	}

	struct input_resource
	{
		spirv_cross::SPIRType type;
	};

	constexpr std::pair<data_layout::type, data_layout::component_type> cross_to_internal_type(const spirv_cross::SPIRType& type)
	{
		typedef spirv_cross::SPIRType::BaseType cross_t;
		typedef data_layout::component_type component_t;
		typedef data_layout::type vector_t;

		std::pair<vector_t, component_t> res;

		switch (type.basetype)
		{
		case cross_t::Half:		res.second = component_t::FLOAT16; break;
		case cross_t::Float:	res.second = component_t::FLOAT32; break;
		case cross_t::Double:	res.second = component_t::FLOAT64; break;
		case cross_t::SByte:	res.second = component_t::INT8; break;
		case cross_t::Short:	res.second = component_t::INT16; break;
		case cross_t::Int:		res.second = component_t::INT32; break;
		case cross_t::Int64:	res.second = component_t::INT64; break;
		case cross_t::UByte:	res.second = component_t::UINT8; break;
		case cross_t::UShort:	res.second = component_t::UINT16; break;
		case cross_t::UInt:		res.second = component_t::UINT32; break;
		case cross_t::UInt64:	res.second = component_t::UINT64; break;
		default:
			INTERNAL_ASSERT(false, "Invalid component type");
		}

		switch (type.vecsize)
		{
		case 1: res.first = vector_t::SCALAR; break;
		case 2: res.first = vector_t::VEC2; break;
		case 3: res.first = vector_t::VEC3; break;
		case 4: res.first = vector_t::VEC4; break;
		default:
			INTERNAL_ASSERT(false, "Invalid vector size");
		}

		return res;
	}

	shader::shader(const char* filename, const char* entry_point, const char* name, shader_t stage) :
		stage(stage == shader_t::NONE ? stage_from_filename(filename) : stage), 
		entry_point(create_cstr(entry_point)), _name(create_cstr(name))
	{
		std::size_t filesize = 0;
		void* filedata = read_binary_file(filename, filesize);
		
		std::string fn(filename);
		std::string extension = fn.substr(fn.rfind('.'));

		//check if shader needs to be compiled
		if (extension != shader_ext::SPIRV)
		{
			//TODO implement read text file and change the read function depending if there is a need to compile
			compile_shader(static_cast<char*>(filedata), filesize, entry_point, this->stage, filename, &data, &size);

			std::free(filedata);
		}
		else
		{
			size = filesize;
			data = static_cast<u32*>(filedata);
		}

		reflect();
	}

	shader::shader(const char* name, const char* entry_point, shader_t stage, const char* code) :
		_name(create_cstr(name)), entry_point(create_cstr(entry_point)), stage(stage)
	{
		compile_shader(code, std::strlen(code), entry_point, this->stage, name, &data, &size);
		reflect();
	}

	shader::~shader()
	{
		if (data)
		{
			std::free(data);
			std::free(_name);
			std::free(entry_point);
			data = nullptr;
		}
	}

	const data_layout* shader::inputs(std::size_t& out_size) const noexcept
	{
		out_size = _inputs.size();
		return _inputs.data();
	}

	inline std::vector<data_layout> reflect_inputs(const spirv_cross::Compiler& reflection, const spirv_cross::ShaderResources& resources)
	{
		std::vector<std::vector<input_resource>> inputs_vector;
		for (const spirv_cross::Resource& input : resources.stage_inputs)
		{
			u32 binding = reflection.get_decoration(input.id, spv::DecorationBinding);

			const std::size_t min_binding_size = static_cast<std::size_t>(binding + 1);
			if (inputs_vector.size() < min_binding_size) inputs_vector.resize(min_binding_size);

			auto& binding_vector = inputs_vector[binding];
			u32 location = reflection.get_decoration(input.id, spv::DecorationLocation);

			const std::size_t min_location_size = static_cast<std::size_t>(location + 1);
			if (binding_vector.size() < min_location_size) binding_vector.resize(min_location_size);

			binding_vector[location].type = reflection.get_type(input.type_id);
		}

		std::vector<data_layout> inputs(inputs_vector.size());

		u32 i = 0;
		for (const auto& binding : inputs_vector)
		{
			u32 k = 0;
			inputs[i] = data_layout(static_cast<u8>(binding.size()));
			for (const auto& location : binding)
			{
				const auto [vt, ct] = cross_to_internal_type(location.type);
				inputs[i].set_type(k, vt, ct);
				k++;
			}
			i++;
		}

		return inputs;
	}

	template<shader::descriptor_t Type>
	inline void reflect_descriptors(const spirv_cross::Compiler& reflection, const spirv_cross::ShaderResources& resources,
		std::vector<shader::descriptor_data>& descriptors)
	{
		const spirv_cross::SmallVector<spirv_cross::Resource>* buffers;
		if constexpr (Type == shader::UNIFORM) buffers = &resources.uniform_buffers;
		else if constexpr (Type == shader::STORAGE) buffers = &resources.storage_buffers;
		//else if constexpr (Type == shader::PUSH_CONSTANT) buffers = *resources.push_constant_buffers;
		else if constexpr (Type == shader::SAMPLER) buffers = &resources.sampled_images;
		else if constexpr (Type == shader::TEXTURE) buffers = &resources.separate_images;
		else if constexpr (Type == shader::IMAGE) buffers = &resources.storage_images;
		//else if constexpr (Type == shader::SAMPLER_BUFFER) buffers = *resources.separate_images; //type.image.dim = DimBuffer
		//else if constexpr (Type == shader::IMAGE_BUFFER) buffers = *resources.storage_images; //type.image.dim = DimBuffer
		else if constexpr (Type == shader::SAMPLER_SHADOW) buffers = &resources.separate_samplers;
		else static_assert(force_eval<Type>::value, "Unimplemented type");

		for (const spirv_cross::Resource& var : *buffers)
		{
			shader::descriptor_data v = {};
			v.set = reflection.get_decoration(var.id, spv::DecorationDescriptorSet);
			v.binding = reflection.get_decoration(var.id, spv::DecorationBinding);
			
			if constexpr (Type == shader::UNIFORM) v.type = shader::UNIFORM;
			else if constexpr (Type == shader::STORAGE) v.type = shader::STORAGE;
			else if constexpr (Type == shader::SAMPLER) v.type = shader::SAMPLER;
			else if constexpr (Type == shader::TEXTURE)
			{
				if (reflection.get_type(var.base_type_id).image.dim != spv::DimBuffer) v.type = shader::TEXTURE;
				else v.type = shader::SAMPLER_BUFFER;
			}
			else if constexpr (Type == shader::IMAGE)
			{
				if (reflection.get_type(var.base_type_id).image.dim != spv::DimBuffer) v.type = shader::IMAGE;
				else v.type = shader::IMAGE_BUFFER;
			}
			else if constexpr (Type == shader::SAMPLER_SHADOW) v.type = shader::SAMPLER_SHADOW;
			else static_assert(force_eval<Type>::value, "Unimplemented type");

			v.count = 1;
			const spirv_cross::SPIRType& type = reflection.get_type(var.type_id);
			if (!type.array.empty())
			{
				for (std::size_t i = 0; i < type.array.size(); i++)
				{
					if (type.array_size_literal[i]) v.count *= type.array[i];
					//TODO handle specialisation constant defined sizes 
				}
			}

			descriptors.push_back(v);
		}
	}

	void shader::reflect()
	{
		spirv_cross::Compiler reflection(data, size / sizeof(*data));
		spirv_cross::ShaderResources resources = reflection.get_shader_resources();
		
		_inputs = reflect_inputs(reflection, resources);
		//TODO: reflect_outputs to check shader compatibility maybe, or set write mask in VkPipelineColorBlendAttachmentState
		//TODO: reflect_subpass_inputs not sure what this is

		reflect_descriptors<UNIFORM>(reflection, resources, _descriptors);
		reflect_descriptors<STORAGE>(reflection, resources, _descriptors);
		reflect_descriptors<SAMPLER>(reflection, resources, _descriptors);
		reflect_descriptors<TEXTURE>(reflection, resources, _descriptors);
		reflect_descriptors<IMAGE>(reflection, resources, _descriptors);
		reflect_descriptors<SAMPLER_SHADOW>(reflection, resources, _descriptors);

		_descriptors.shrink_to_fit();

		//TODO: reflect_push_constants
		//TODO: reflect_specialisation_constants
	}

	VkShaderModule internal_shader::create_shader_module(const shader& shader, VkDevice& handle, 
		VkShaderStageFlagBits* out_stage, const char** out_entry_point)
	{
		//cast should cause no issues, as internal shader is merely a helper class to help access base class variables
		const internal_shader& cast_shader = static_cast<const internal_shader&>(shader);

		VkShaderModule module;

		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.pCode = cast_shader.data;
		create_info.codeSize = cast_shader.size;

		VK_CRASH_CHECK(vkCreateShaderModule(handle, &create_info, nullptr, &module), "Failed to create shader module");

		*out_stage = cast_shader.get_stage();
		*out_entry_point = create_cstr(cast_shader.entry_point);

		return module;
	}
}
