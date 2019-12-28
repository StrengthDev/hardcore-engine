#pragma once

#include "Core.hpp"
#include "Spiral/Test.hpp"

namespace Spiral
{
	class SPIRAL_API Client
	{
	public:
		Client();

		virtual ~Client();

		void Run();
	};

	//Should be defined in client
	Client* Start();
}