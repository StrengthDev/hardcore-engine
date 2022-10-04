#include <pch.hpp>

#include <spiral/render/render_core.hpp>
#include <spiral/render/pipeline.hpp>
#include <spiral/render/renderer_internal.hpp>

namespace Spiral
{
	pipeline::~pipeline()
	{
		if (valid())
		{

		}
	}

	render_pipeline::render_pipeline(const shader& vertex, const shader& fragment)
		: pipeline(std::numeric_limits<std::uint32_t>::max(), pipeline_t::RENDER, pipeline_s::PASSIVE)
	{
		id = renderer::get_device().add_graphics_pipeline(vertex, fragment);
	}
	
	void render_pipeline::link(const object_resource& object)
	{
		renderer::get_device().get_graphics_pipeline(id).link(object);
	}

	void render_pipeline::link(const object_resource& object, const instance& instance)
	{
		renderer::get_device().get_graphics_pipeline(id).link(object, instance);
	}
}
