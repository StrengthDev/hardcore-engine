#include <pch.hpp>

#include <spiral/render/render_core.hpp>
#include <spiral/render/pipeline.hpp>
#include <spiral/render/renderer_internal.hpp>

namespace Spiral
{
	pipeline::pipeline(const char* vert, const char* frag) : type(pipeline_t::RENDER)
	{

	}

	pipeline::~pipeline()
	{
		if (valid())
		{

		}
	}
}
