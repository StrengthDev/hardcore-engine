#pragma once

#include <spiral/core/core.hpp>

#include "shader.hpp"
#include "resource.hpp"
#include "instance.hpp"

namespace ENGINE_NAMESPACE
{
	enum class pipeline_t : std::uint8_t
	{
		NONE = 0,
		RENDER,
		PIXEL,
		COMPUTE
	};

	enum class pipeline_s : std::uint8_t
	{
		DISABLED = 0,
		ACTIVE,
		PASSIVE
	};

	class ENGINE_API pipeline
	{
	protected:
		pipeline() = default;

		pipeline(const pipeline&) = delete;
		pipeline& operator=(const pipeline&) = delete;

		pipeline(pipeline&& other) noexcept : id(std::exchange(other.id, std::numeric_limits<std::uint32_t>::max())),
			status(std::exchange(other.status, pipeline_s::DISABLED)), type(other.type)
		{}

		pipeline& operator=(pipeline&& other)
		{
			this->~pipeline();
			id = std::exchange(other.id, std::numeric_limits<std::uint32_t>::max());
			status = std::exchange(other.status, pipeline_s::DISABLED);
			type = std::exchange(other.type, pipeline_t::NONE);
			return *this;
		}

		pipeline(std::uint32_t id, pipeline_t type, pipeline_s status) noexcept : id(id), status(status), type(type) {}

		~pipeline();

		std::uint32_t id = std::numeric_limits<std::uint32_t>::max();
		pipeline_s status = pipeline_s::DISABLED;
		pipeline_t type = pipeline_t::NONE;

	public:
		inline bool valid() const
		{
			return id != std::numeric_limits<std::uint32_t>::max();
		}

		inline pipeline_t get_type() const noexcept { return type; }
	};

	class ENGINE_API render_pipeline : public pipeline
	{
	public:
		render_pipeline() = default;
		render_pipeline(const shader& vertex, const shader& fragment);

		void link(const object_resource& object);
		void link(const object_resource& object, const instance& instance);
	};
}
