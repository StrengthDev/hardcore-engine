#pragma once

#include <hardcore/core/core.hpp>

namespace ENGINE_NAMESPACE
{
	ENGINE_API void* read_binary_file(const char* filepath, std::size_t& out_filesize);
}
