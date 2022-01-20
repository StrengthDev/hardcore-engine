#pragma once

#include <spiral/core/core.hpp>
#include "mesh.hpp"
#include "shader.hpp"

namespace Spiral
{
	class SPIRAL_API Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void m_presentFrame() = 0;
		virtual void m_loadMesh(Mesh mesh, uint32_t vertexShaderId, uint32_t fragShaderId) = 0;

		static Renderer* init(program_id engineProps, program_id clientProps);
		static void loadMesh(Mesh mesh, uint32_t vertexShaderId, uint32_t fragShaderId);
		//TODO: static functions
	};
}