#include <pch.hpp>

#include <io/file.hpp>

#include <debug/log_internal.hpp>

namespace ENGINE_NAMESPACE
{
	void* read_binary_file(const char* filepath, std::size_t& out_filesize)
	{
		std::filesystem::path path(filepath);

		if (!std::filesystem::is_regular_file(path))
		{
			LOG_INTERNAL_ERROR("Path is not a regular file, or does not exist: " << filepath);
			DEBUG_BREAK;
			out_filesize = 0;
			return nullptr;
		}

		const std::uintmax_t filesize_mt = std::filesystem::file_size(path);

		if (std::numeric_limits<std::size_t>::max() < filesize_mt)
		{
			//This function is meant to read a whole files contents and put them into a single buffer in memory,
			//if a file is larger than the maximum size_t, then a different method to read the file should probably be
			//used anyways
			DEBUG_BREAK;
			CRASH("File too big");
		}

		const std::size_t filesize = filesize_mt;

		std::ifstream file(path, std::ios::in | std::ios::binary);
		if (!file.is_open())
		{
			CRASH("Failed to open file");
		}

		char* filedata = t_malloc<char>(filesize);

		constexpr std::size_t streamsize_max = std::numeric_limits<std::streamsize>::max();
		std::size_t offset = 0;

		while (file.read(filedata + offset, streamsize_max))
			offset += streamsize_max;

		INTERNAL_ASSERT(file.eof(), "Did not reach end of file");

		file.close();

		out_filesize = filesize;
		return filedata;
	}
}
