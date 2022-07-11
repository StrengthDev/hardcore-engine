#pragma once

#include "mesh.hpp"
#include "shader.hpp"

namespace Spiral
{
	namespace renderer
	{
		void loadMesh(Mesh mesh, uint32_t vertexShaderId, uint32_t fragShaderId);
	};
}
