#include <pch.hpp>

#include <render/renderer_internal.hpp>
#include <render/shader.hpp>

#include <debug/log_internal.hpp>

#include <shaderc/shaderc.h>
#include <spirv_cross/spirv_reflect.hpp>

namespace ENGINE_NAMESPACE
{
//This macro is implemented as a std function in C++20, could consider updating (TODO)
#define ENDS_WITH(str, size, suffix) (size >= sizeof(suffix) - 1 && !str.compare(size - (sizeof(suffix) - 1), sizeof(suffix) - 1, suffix))

#define CASE(str, size, suffix, result) if (ENDS_WITH(str, size, suffix)) return result

	inline shader_t stage_from_filename(const char* filename)
	{
		std::string fn(filename);
		std::size_t fnl = fn.length();
		if (ENDS_WITH(fn, fnl, shader_extensions::SPIRV))
		{
			std::size_t pos = fn.rfind(shader_extensions::SPIRV);
			fn = fn.substr(0, pos);
			fnl = fn.length();
		}

		CASE(fn, fnl, shader_extensions::VERTEX, shader_t::VERTEX);
		CASE(fn, fnl, shader_extensions::FRAGMENT, shader_t::FRAGMENT);
		CASE(fn, fnl, shader_extensions::COMPUTE, shader_t::COMPUTE);
		CASE(fn, fnl, shader_extensions::TESSELATION_CONTROL, shader_t::TESSELATION_CONTROL);
		CASE(fn, fnl, shader_extensions::TESSELATION_EVALUATION, shader_t::TESSELATION_EVALUATION);
		CASE(fn, fnl, shader_extensions::GEOMETRY, shader_t::GEOMETRY);
		CASE(fn, fnl, shader_extensions::RAY_GENERATION, shader_t::RAY_GENERATION);
		CASE(fn, fnl, shader_extensions::RAY_INTERSECTION, shader_t::RAY_INTERSECTION);
		CASE(fn, fnl, shader_extensions::RAY_ANY_HIT, shader_t::RAY_ANY_HIT);
		CASE(fn, fnl, shader_extensions::RAY_CLOSEST_HIT, shader_t::RAY_CLOSEST_HIT);
		CASE(fn, fnl, shader_extensions::RAY_MISS, shader_t::RAY_MISS);
		CASE(fn, fnl, shader_extensions::RAY_CALLABLE, shader_t::RAY_CALLABLE);
		CASE(fn, fnl, shader_extensions::MESH, shader_t::MESH);
		CASE(fn, fnl, shader_extensions::TASK, shader_t::TASK);

		INTERNAL_ASSERT(false, "Unknown shader type");
		return shader_t::NONE;
	}

#undef CASE

	/**
	 * @brief Helper function to allocate and copy an identical C string.
	 * @param name C string to copy
	 * @return Newly allocated C string copy
	*/
	char* create_cstr(const char* name)
	{
		const std::size_t size = std::strlen(name) + 1;
		char* res = t_malloc<char>(size);
		std::memcpy(res, name, size * sizeof(char));
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
		const char* error_name, std::uint32_t** out_data, std::size_t* out_datasize)
	{
		shaderc_compiler_t compiler = shaderc_compiler_initialize();

		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		//TODO: add options, maybe

		shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, code, code_size / sizeof(char), to_shaderc(stage),
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

		*out_datasize = shaderc_result_get_length(result);
		*out_data = reinterpret_cast<std::uint32_t*>(ex_malloc(*out_datasize));
		memcpy(*out_data, shaderc_result_get_bytes(result), *out_datasize);

		shaderc_result_release(result);
	}

	struct input_resource
	{
		spirv_cross::SPIRType type;
	};

#define BASE_CASE(cross_type, internal_type) case spirv_cross::SPIRType::BaseType::cross_type: *out_ct = data_layout::component_type::internal_type; break;
	inline void cross_to_internal_type(const spirv_cross::SPIRType& type, data_layout::type* out_t, data_layout::component_type* out_ct)
	{
		switch (type.basetype)
		{
			BASE_CASE(Half, FLOAT16);
			BASE_CASE(Float, FLOAT32);
			BASE_CASE(Double, FLOAT64);
			BASE_CASE(SByte, INT8);
			BASE_CASE(Short, INT16);
			BASE_CASE(Int, INT32);
			BASE_CASE(Int64, INT64);
			BASE_CASE(UByte, UINT8);
			BASE_CASE(UShort, UINT16);
			BASE_CASE(UInt, UINT32);
			BASE_CASE(UInt64, UINT64);
		default:
			INTERNAL_ASSERT(false, "Invalid component type");
		}

		switch (type.vecsize)
		{
		case 1: *out_t = data_layout::type::SCALAR; break;
		case 2: *out_t = data_layout::type::VEC2; break;
		case 3: *out_t = data_layout::type::VEC3; break;
		case 4: *out_t = data_layout::type::VEC4; break;
		default:
			INTERNAL_ASSERT(false, "Invalid vector size");
		}
	}
#undef BASE_CASE

	shader::shader(const char* filename, const char* entry_point, const char* name, shader_t stage) :
		stage(stage == shader_t::NONE ? stage_from_filename(filename) : stage), 
		entry_point(create_cstr(entry_point)), name(create_cstr(name))
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			CRASH("Failed to open shader file");
		}

		std::streampos pos = file.tellg();
		std::size_t buffer_size = static_cast<std::int32_t>(pos) > 0 && static_cast<std::int32_t>(pos) < MEGABYTES(1)
			? static_cast<std::int32_t>(pos) : BIT(12);
		std::vector<std::byte> buffer(buffer_size);
		file.seekg(0, std::ios::beg);

		std::vector<std::byte> file_data;
		file_data.reserve(buffer_size);
		std::size_t filesize = 0;
		while (file.read(reinterpret_cast<char*>(buffer.data()), buffer_size))
		{
			file_data.insert(file_data.end(), buffer.begin(), buffer.begin() + file.gcount());
			filesize += file.gcount();
		}
		file.close();
		
		std::string fn(filename);
		std::size_t fnl = fn.length();
		if (!ENDS_WITH(fn, fnl, shader_extensions::SPIRV)) //needs to be compiled
		{
			compile_shader(reinterpret_cast<char*>(file_data.data()), filesize / sizeof(char), entry_point, this->stage, filename,
				&data, &size);
		}
		else
		{
			size = filesize;
			data = reinterpret_cast<std::uint32_t*>(ex_malloc(filesize));
			memcpy(data, file_data.data(), filesize);
		}

		reflect();
	}

#undef ENDS_WITH

	shader::shader(const char* name, const char* entry_point, shader_t stage, const char* code) :
		name(create_cstr(name)), entry_point(create_cstr(entry_point)), stage(stage)
	{
		compile_shader(code, (std::strlen(name) + 1) / sizeof(char), entry_point, this->stage, name, &data, &size);
		reflect();
	}

	shader::~shader()
	{
		if (data)
		{
			for (std::uint8_t i = 0; i < n_inputs; i++) inputs[i].~data_layout();
			
			std::free(data);
			std::free(inputs);
			std::free(name);
			std::free(entry_point);
			data = nullptr;
		}
	}

	inline void reflect_inputs(const spirv_cross::Compiler& reflection, const spirv_cross::ShaderResources& resources,
		std::uint8_t* out_n_inputs, data_layout** out_inputs)
	{
		std::vector<std::vector<input_resource>> inputs_vector;
		for (const spirv_cross::Resource& input : resources.stage_inputs)
		{
			std::uint32_t binding = reflection.get_decoration(input.id, spv::DecorationBinding);

			for (std::size_t i = inputs_vector.size(); i < static_cast<std::size_t>(binding) + 1; i++) inputs_vector.emplace_back();

			auto& binding_vector = inputs_vector[binding];
			std::uint32_t location = reflection.get_decoration(input.id, spv::DecorationLocation);

			for (std::size_t i = binding_vector.size(); i < static_cast<std::size_t>(location) + 1; i++) binding_vector.emplace_back();

			binding_vector[location].type = reflection.get_type(input.type_id);
		}

		std::uint32_t i = 0;
		*out_n_inputs = static_cast<std::uint8_t>(inputs_vector.size());
		*out_inputs = t_malloc<data_layout>(*out_n_inputs);
		for (const auto& binding : inputs_vector)
		{
			std::uint32_t k = 0;
			new (&(*out_inputs)[i]) data_layout(static_cast<std::uint8_t>(binding.size()));
			for (const auto& location : binding)
			{
				data_layout::type t;
				data_layout::component_type ct;
				cross_to_internal_type(location.type, &t, &ct);
				(*out_inputs)[i].set_type(k, t, ct);
				k++;
			}
			i++;
		}
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
				for (std::uint32_t i = 0; i < type.array.size(); i++)
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
		spirv_cross::Compiler reflection(data, size / sizeof(std::uint32_t));
		spirv_cross::ShaderResources resources = reflection.get_shader_resources();
		
		reflect_inputs(reflection, resources, &n_inputs, &inputs);
		//TODO: reflect_outputs to check shader compatibility maybe, or set write mask in VkPipelineColorBlendAttachmentState
		//TODO: reflect_subpass_inputs not sure what this is

		std::vector<descriptor_data> descriptors;
		reflect_descriptors<UNIFORM>(reflection, resources, descriptors);
		reflect_descriptors<STORAGE>(reflection, resources, descriptors);
		reflect_descriptors<SAMPLER>(reflection, resources, descriptors);
		reflect_descriptors<TEXTURE>(reflection, resources, descriptors);
		reflect_descriptors<IMAGE>(reflection, resources, descriptors);
		reflect_descriptors<SAMPLER_SHADOW>(reflection, resources, descriptors);

		if (descriptors.size())
		{
			this->descriptors = t_malloc<descriptor_data>(descriptors.size());
			this->n_descriptors = static_cast<std::uint32_t>(descriptors.size());
			std::memcpy(this->descriptors, descriptors.data(), descriptors.size() * sizeof(descriptor_data));
		}

		//TODO: reflect_push_constants
		//TODO: reflect_specialisation_constants
	}

	VkShaderModule internal_shader::create_shader_module(const shader& shader, VkDevice& handle, 
		VkShaderStageFlagBits* out_stage, const char** out_entry_point)
	{
		//cast should cause no issues, as internal shader is merely a helper class to help access base class variables
		const internal_shader& cast_shader = reinterpret_cast<const internal_shader&>(shader);

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
