#include <pch.hpp>

#include <render/render_core.hpp>
#include <render/pipeline.hpp>
#include <render/renderer_internal.hpp>

namespace ENGINE_NAMESPACE
{
	volatile_pipeline_task_ref<pipeline_t::RENDER>& volatile_pipeline_task_ref<pipeline_t::RENDER>::set_instances(u32 num)
	{
		graphics_pipeline& pipeline = *static_cast<graphics_pipeline*>(pipeline_p);
		pipeline.set_instances(task_id, num);
		return *this;
	}

#define SET_DESCRIPTOR(Return, Arg)																										\
volatile_pipeline_task_ref<Return>& volatile_pipeline_task_ref<Return>::set_descriptor(u32 descriptor_idx, const Arg& buffer)	\
{																																		\
	return t_set_descriptor(descriptor_idx, buffer);																					\
}

#define SET_DESCRIPTORS(Return)					\
SET_DESCRIPTOR(Return, uniform)					\
SET_DESCRIPTOR(Return, unmapped_uniform)		\
SET_DESCRIPTOR(Return, storage_array)			\
SET_DESCRIPTOR(Return, dynamic_storage_array)	\
SET_DESCRIPTOR(Return, storage_vector)			\
SET_DESCRIPTOR(Return, dynamic_storage_vector)	

	SET_DESCRIPTORS(pipeline_t::RENDER);

#undef SET_DESCRIPTORS
#undef SET_DESCRIPTOR

	template<>
	template<typename Resource, std::enable_if_t<std::is_base_of<resource, Resource>::value && std::is_final<Resource>::value, bool>>
	volatile_pipeline_task_ref<pipeline_t::RENDER>& volatile_pipeline_task_ref<pipeline_t::RENDER>::t_set_descriptor(
		u32 descriptor_idx, const Resource& buffer)
	{
		graphics_pipeline& pipeline = *static_cast<graphics_pipeline*>(pipeline_p);
		pipeline.set_descriptor(task_id, descriptor_idx, buffer);
		return *this;
	}

	pipeline::~pipeline()
	{
		if (valid())
		{

		}
	}

	render_pipeline::render_pipeline(const shader& vertex, const shader& fragment)
		: pipeline(std::numeric_limits<u32>::max(), pipeline_t::RENDER, pipeline_s::PASSIVE)
	{
		id = renderer::get_device().add_graphics_pipeline(vertex, fragment);
	}
	
	render_pipeline::vptr_t render_pipeline::add(const mesh& object)
	{
		graphics_pipeline& pipeline = renderer::get_device().get_graphics_pipeline(id);
		return volatile_pipeline_task_ref<pipeline_t::RENDER>(&pipeline, pipeline.add(object));
	}

	render_pipeline::vptr_t render_pipeline::set_instances(const mesh& object, u32 num)
	{
		graphics_pipeline& pipeline = renderer::get_device().get_graphics_pipeline(id);
		return volatile_pipeline_task_ref<pipeline_t::RENDER>(&pipeline, pipeline.set_instances(object, num));
	}

#define SET_DESCRIPTOR(Arg)																										\
render_pipeline::vptr_t render_pipeline::set_descriptor(const mesh& object, u32 descriptor_idx, const Arg& buffer)	\
{																																\
	graphics_pipeline& pipeline = renderer::get_device().get_graphics_pipeline(id);										\
	return volatile_pipeline_task_ref<pipeline_t::RENDER>(&pipeline, pipeline.set_descriptor(object, descriptor_idx, buffer));	\
}

	SET_DESCRIPTOR(uniform);
	SET_DESCRIPTOR(unmapped_uniform);
	SET_DESCRIPTOR(storage_array);
	SET_DESCRIPTOR(dynamic_storage_array);
	SET_DESCRIPTOR(storage_vector);
	SET_DESCRIPTOR(dynamic_storage_vector);
#undef SET_DESCRIPTOR
}
