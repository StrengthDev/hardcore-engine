#pragma once

#include <hardcore/core/core.hpp>

#include "shader.hpp"
#include "resource.hpp"

namespace ENGINE_NAMESPACE
{
	enum class pipeline_t : u8
	{
		NONE = 0,
		RENDER,
		PIXEL,
		COMPUTE
	};

	enum class pipeline_s : u8
	{
		DISABLED = 0,
		ACTIVE,
		PASSIVE
	};

	class render_pipeline;
	//TODO change implementation, tasks should be represented by an actual class instead of just a reference like everything else
	//meaning freed in the class destructor
	template<pipeline_t PipelineType>
	class ENGINE_API volatile_pipeline_task_ref final
	{
	public:
		volatile_pipeline_task_ref() = delete;

		//volatile_pipeline_task_ref(const volatile_pipeline_task_ref&) = delete;
		//volatile_pipeline_task_ref(volatile_pipeline_task_ref&&) = delete;
		//volatile_pipeline_task_ref& operator=(const volatile_pipeline_task_ref&) = delete;
		//volatile_pipeline_task_ref& operator=(volatile_pipeline_task_ref&&) = delete;

		volatile_pipeline_task_ref<PipelineType>& set_instances(u32 num);

		volatile_pipeline_task_ref<PipelineType>& set_descriptor(u32 descriptor_idx, const uniform& buffer);
		volatile_pipeline_task_ref<PipelineType>& set_descriptor(u32 descriptor_idx, const unmapped_uniform& buffer);
		volatile_pipeline_task_ref<PipelineType>& set_descriptor(u32 descriptor_idx, const storage_array& buffer);
		volatile_pipeline_task_ref<PipelineType>& set_descriptor(u32 descriptor_idx, const dynamic_storage_array& buffer);
		volatile_pipeline_task_ref<PipelineType>& set_descriptor(u32 descriptor_idx, const storage_vector& buffer);
		volatile_pipeline_task_ref<PipelineType>& set_descriptor(u32 descriptor_idx, const dynamic_storage_vector& buffer);

	private:
		volatile_pipeline_task_ref(void* pipeline_p, std::size_t task_id) noexcept : 
			pipeline_p(pipeline_p), task_id(task_id)
		{
			static_assert(PipelineType != pipeline_t::NONE, "Invalid pipeline type");
		}

		template<typename Resource,
			std::enable_if_t<std::is_base_of<resource, Resource>::value && std::is_final<Resource>::value, bool> = true>
		volatile_pipeline_task_ref<PipelineType>& t_set_descriptor(u32 descriptor_idx, const Resource& buffer);

		void* pipeline_p;
		std::size_t task_id;

		friend class ::ENGINE_NAMESPACE::render_pipeline;
	};

	class ENGINE_API pipeline
	{
	protected:
		pipeline() = default;

		pipeline(const pipeline&) = delete;
		pipeline& operator=(const pipeline&) = delete;

		pipeline(pipeline&& other) noexcept : id(std::exchange(other.id, std::numeric_limits<u32>::max())),
			status(std::exchange(other.status, pipeline_s::DISABLED)), type(other.type)
		{}

		pipeline& operator=(pipeline&& other)
		{
			this->~pipeline();
			id = std::exchange(other.id, std::numeric_limits<u32>::max());
			status = std::exchange(other.status, pipeline_s::DISABLED);
			type = std::exchange(other.type, pipeline_t::NONE);
			return *this;
		}

		pipeline(u32 id, pipeline_t type, pipeline_s status) noexcept : id(id), status(status), type(type) {}

		~pipeline();

		u32 id = std::numeric_limits<u32>::max(); //TODO tchange to key
		pipeline_s status = pipeline_s::DISABLED;
		pipeline_t type = pipeline_t::NONE;

	public:
		inline bool valid() const
		{
			return id != std::numeric_limits<u32>::max();
		}

		inline pipeline_t get_type() const noexcept { return type; }
	};

	class ENGINE_API render_pipeline : public pipeline
	{
	private:
		typedef volatile_pipeline_task_ref<pipeline_t::RENDER> vptr_t;

	public:
		render_pipeline() = default;
		render_pipeline(const shader& vertex, const shader& fragment);

		vptr_t add(const mesh& object);

		vptr_t set_instances(const mesh& object, u32 num);

		vptr_t set_descriptor(const mesh& object, u32 descriptor_idx, const uniform& buffer);
		vptr_t set_descriptor(const mesh& object, u32 descriptor_idx, const unmapped_uniform& buffer);
		vptr_t set_descriptor(const mesh& object, u32 descriptor_idx, const storage_array& buffer);
		vptr_t set_descriptor(const mesh& object, u32 descriptor_idx, const dynamic_storage_array& buffer);
		vptr_t set_descriptor(const mesh& object, u32 descriptor_idx, const storage_vector& buffer);
		vptr_t set_descriptor(const mesh& object, u32 descriptor_idx, const dynamic_storage_vector& buffer);
	};
}
