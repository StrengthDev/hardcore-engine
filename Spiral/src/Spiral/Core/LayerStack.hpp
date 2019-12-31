#pragma once

#include "pch.hpp"

#include "Core.hpp"
#include "Layer.hpp"

namespace Spiral
{
	class SPIRAL_API LayerStack
	{
	public:
		LayerStack();

		~LayerStack();

		void push(Layer* layer, uint32_t tier);

		void pop(uint32_t tier);

	private:

	};
}