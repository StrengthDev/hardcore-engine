#pragma once

#include "render_core.hpp"
#include "swapchain.hpp"
#include "shader_internal.hpp"
#include "memory.hpp"
#include <render/resource.hpp>

#include <core/exception.hpp>

#include <vector>
#include <unordered_map>

namespace ENGINE_NAMESPACE
{
	class device;

	class graphics_pipeline
	{
	public:
		graphics_pipeline(device& owner, const std::vector<const shader*>& shaders);
		graphics_pipeline(device& owner, const std::vector<const shader*>& shaders, const VkExtent2D& extent);
		~graphics_pipeline();

		graphics_pipeline(const graphics_pipeline&) = delete;
		inline graphics_pipeline& operator=(const graphics_pipeline&) = delete;

		graphics_pipeline(graphics_pipeline&& other) noexcept : owner(other.owner), 
			n_descriptor_set_layouts(std::exchange(other.n_descriptor_set_layouts, 0)),
			descriptor_set_layouts(std::exchange(other.descriptor_set_layouts, VK_NULL_HANDLE)),
			pipeline_layout(std::exchange(other.pipeline_layout, VK_NULL_HANDLE)),
			handle(std::exchange(other.handle, VK_NULL_HANDLE)),
			n_descriptor_pool_sizes{ std::exchange(other.n_descriptor_pool_sizes[0], 0), 
				std::exchange(other.n_descriptor_pool_sizes[1], 0) },
			descriptor_pool_sizes{ std::exchange(other.descriptor_pool_sizes[0], nullptr),
				std::exchange(other.descriptor_pool_sizes[1], nullptr) },
			descriptor_sets(std::exchange(other.descriptor_sets, nullptr)),
			object_descriptor_set_capacity(std::exchange(other.object_descriptor_set_capacity, 0)),
			frame_descriptors(std::move(other.frame_descriptors)),
			frame_descriptor_count(std::exchange(other.frame_descriptor_count, 0)),
			n_object_bindings(std::exchange(other.n_object_bindings, 0)),
			object_binding_types(std::exchange(other.object_binding_types, nullptr)),
			n_dynamic_descriptors(std::exchange(other.n_dynamic_descriptors, 0)),
			object_refs(std::move(other.object_refs)),
			objects(std::move(other.objects)),
			cached_object_bindings(std::move(other.cached_object_bindings))
		{}

		inline graphics_pipeline& operator=(graphics_pipeline&& other) noexcept
		{
			this->~graphics_pipeline();
			owner = other.owner;
			n_descriptor_set_layouts = std::exchange(other.n_descriptor_set_layouts, 0);
			descriptor_set_layouts = std::exchange(other.descriptor_set_layouts, VK_NULL_HANDLE);
			pipeline_layout = std::exchange(other.pipeline_layout, VK_NULL_HANDLE);
			handle = std::exchange(other.handle, VK_NULL_HANDLE);
			n_descriptor_pool_sizes[0] = std::exchange(other.n_descriptor_pool_sizes[0], 0);
			n_descriptor_pool_sizes[1] = std::exchange(other.n_descriptor_pool_sizes[1], 0);
			descriptor_pool_sizes[0] = std::exchange(other.descriptor_pool_sizes[0], nullptr);
			descriptor_pool_sizes[1] = std::exchange(other.descriptor_pool_sizes[1], nullptr);
			descriptor_sets = std::exchange(other.descriptor_sets, nullptr);
			object_descriptor_set_capacity = std::exchange(other.object_descriptor_set_capacity, 0);
			frame_descriptors = std::move(other.frame_descriptors);
			frame_descriptor_count = std::exchange(other.frame_descriptor_count, 0);
			n_object_bindings = std::exchange(other.n_object_bindings, 0);
			object_binding_types = std::exchange(other.object_binding_types, nullptr);
			n_dynamic_descriptors = std::exchange(other.n_dynamic_descriptors, 0);
			object_refs = std::move(other.object_refs);
			objects = std::move(other.objects);
			cached_object_bindings = std::move(other.cached_object_bindings);
			return *this;
		}

		void record_commands(VkCommandBuffer& buffer, std::uint8_t current_frame);
		void update_descriptor_sets(std::uint8_t previous_frame, std::uint8_t current_frame, std::uint8_t next_frame);

		std::size_t add(const mesh& object);
		void remove(const mesh& object);

		std::size_t set_instances(const mesh& object, std::uint32_t num);
		inline void set_instances(std::size_t object_idx, std::uint32_t num) { objects[object_idx].properties().instances = num; }

		template<typename Type, 
			std::enable_if_t<std::is_base_of<resource, Type>::value && std::is_final<Type>::value, bool> = true>
		inline std::size_t set_descriptor(const mesh& object, std::uint32_t descriptor_idx, const Type& buffer)
		{
			auto it = object_refs.find(std::hash<resource>{}(object));
			if (it == object_refs.end())
				throw std::range_error("Specified mesh not found in pipeline");
			std::size_t idx = it->second;
			set_descriptor(idx, descriptor_idx, buffer);
			return idx;
		}

		template<typename Type,
			std::enable_if_t<std::is_base_of<resource, Type>::value && std::is_final<Type>::value, bool> = true>
		inline void set_descriptor(std::size_t object_idx, std::uint32_t descriptor_idx, const Type& buffer)
		{
			INTERNAL_ASSERT(n_object_bindings > descriptor_idx, "Descriptor index out of bounds");
			if (!type_match(descriptor_idx, buffer))
			{
				std::stringstream stream;
				stream << "Incorrect descriptor type for binding: " << descriptor_idx << " (correct type is: " << 
					debug_descriptor_type(object_binding_types[descriptor_idx]) <<')';
				throw exception::type_mismatch(stream.str());
			}
			set_descriptor(object_idx, descriptor_idx, get_binding(buffer));
		}

	private:
		graphics_pipeline(device& owner, const std::vector<const shader*>& shaders, const VkExtent2D& extent, 
			bool dynamic_viewport);

		inline bool type_match(std::uint32_t descriptor_idx, const uniform&) const noexcept
		{ return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; }
		inline bool type_match(std::uint32_t descriptor_idx, const unmapped_uniform&) const noexcept
		{ return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; }
		inline bool type_match(std::uint32_t descriptor_idx, const storage_array&) const noexcept
		{ return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; }
		inline bool type_match(std::uint32_t descriptor_idx, const dynamic_storage_array&) const noexcept
		{ return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; }
		inline bool type_match(std::uint32_t descriptor_idx, const storage_vector&) const noexcept
		{ return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; }
		inline bool type_match(std::uint32_t descriptor_idx, const dynamic_storage_vector&) const noexcept
		{ return object_binding_types[descriptor_idx] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; }

		buffer_binding_args get_binding(const uniform& resource) const;
		buffer_binding_args get_binding(const unmapped_uniform& resource) const;
		buffer_binding_args get_binding(const storage_array& resource) const;
		buffer_binding_args get_binding(const dynamic_storage_array& resource) const;
		buffer_binding_args get_binding(const storage_vector& resource) const;
		buffer_binding_args get_binding(const dynamic_storage_vector& resource) const;

		void set_descriptor(std::size_t object_idx, std::uint32_t descriptor_idx, buffer_binding_args&& binding);

		device* owner;

		std::uint32_t n_descriptor_set_layouts = 0;
		VkDescriptorSetLayout* descriptor_set_layouts = nullptr;
		VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
		VkPipeline handle = VK_NULL_HANDLE;

		std::uint32_t push_data_size = 0;
		VkShaderStageFlags push_flags = 0;

		//object descriptors => set 0 ; pipeline descriptors => set 1
		std::uint32_t n_descriptor_pool_sizes[2] = { 0, 0 }; //TODO change
		VkDescriptorPoolSize* descriptor_pool_sizes[2] = { nullptr, nullptr };

		VkDescriptorSet* descriptor_sets = nullptr;
		std::uint32_t object_descriptor_set_capacity = 0;

		struct frame_descriptor_data
		{
			VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
			std::uint32_t object_set_cap = 0;
			bool dirty = false; //client made changes this frame
			bool outdated = false; //no changes this frame, but in previous frame
		};

		std::array<frame_descriptor_data, max_frames_in_flight> frame_descriptors;

		std::uint32_t frame_descriptor_count = 0;

		std::uint32_t n_object_bindings = 0;
		VkDescriptorType* object_binding_types = nullptr;
		std::uint32_t n_dynamic_descriptors = 0;

		class object_vector
		{
		public:
			object_vector() = default;
			object_vector(std::size_t push_data_size, std::uint32_t n_descriptors, std::uint32_t n_dynamic_descriptors);
			~object_vector();

			object_vector(object_vector&& other) noexcept :
				count(std::exchange(other.count, 0)),
				capacity(std::exchange(other.capacity, 0)),
				push_data_size(std::exchange(other.push_data_size, 0)),
				n_descriptors(std::exchange(other.n_descriptors, 0)),
				n_dynamic_descriptors(std::exchange(other.n_dynamic_descriptors, 0)),
				element_stride(std::exchange(other.element_stride, 0)),
				descriptor_stride(std::exchange(other.descriptor_stride, 0)),
				object_data(std::exchange(other.object_data, nullptr)),
				object_descriptor_data(std::exchange(other.object_descriptor_data, nullptr)),
				smp(std::exchange(other.smp, nullptr))
			{ }

			inline object_vector& operator=(object_vector&& other) noexcept
			{
				this->~object_vector();
				count = std::exchange(other.count, 0);
				capacity = std::exchange(other.capacity, 0);
				push_data_size = std::exchange(other.push_data_size, 0);
				n_descriptors = std::exchange(other.n_descriptors, 0);
				n_dynamic_descriptors = std::exchange(other.n_dynamic_descriptors, 0);
				element_stride = std::exchange(other.element_stride, 0);
				descriptor_stride = std::exchange(other.descriptor_stride, 0);
				object_data = std::exchange(other.object_data, nullptr);
				object_descriptor_data = std::exchange(other.object_descriptor_data, nullptr);
				smp = std::exchange(other.smp, nullptr);
				return *this;
			}

			object_vector(const object_vector&) = delete;
			object_vector& operator=(const object_vector&) = delete;

			void remove(std::size_t idx);
			void swap(std::size_t a, std::size_t b);

			inline std::size_t size() const noexcept { return count; }
			
			struct base_object
			{
				//void* push_data = nullptr;

				//std::uint32_t* dynamic_offsets = nullptr;
				//buffer_binding_args* discriptor_args = nullptr;

				std::size_t ref_key = 0;
				buffer_binding_args binding = {};
				std::uint32_t count = 0;
				VkIndexType index_t = VK_INDEX_TYPE_NONE_KHR;
				buffer_binding_args index_binding = {};
				std::uint32_t descriptor_set_idx = 0;
				std::uint32_t instances = 1;
			};

			using properties_t = base_object;
			typedef std::uint32_t offset_t;

			class iterator;

			class object_ref
			{
			public:
				inline properties_t& properties() const noexcept
				{
					return *reinterpret_cast<base_object*>(static_cast<std::byte*>(data_ptr) + push_data_size + offsets_size);
				}

				inline offset_t* dynamic_offsets() const noexcept
				{
					return reinterpret_cast<offset_t*>(static_cast<std::byte*>(data_ptr) + push_data_size);
				}

				inline void* push_data() const noexcept { return data_ptr; }
				inline buffer_binding_args* bindings() const noexcept { return binding_ptr; }

			private:
				object_ref(void* data_ptr, buffer_binding_args* binding_ptr, std::size_t push_data_size, 
					std::size_t offsets_size) noexcept :
					data_ptr(data_ptr), binding_ptr(binding_ptr), push_data_size(push_data_size), offsets_size(offsets_size)
				{ }

				void* data_ptr = nullptr;
				buffer_binding_args* binding_ptr = nullptr;
				std::size_t push_data_size = 0;
				std::size_t offsets_size = 0;

				//ideally youd compare all values, but this class will never be used outside of pipelines
				//and for their use case, comparing only data_ptr should be fine
				//binding_ptr cannot be compared as there can be 0 bindings, meaning both are always nullptr
				friend bool operator== (const object_ref& a, const object_ref& b) { return a.data_ptr == b.data_ptr; };
				friend bool operator!= (const object_ref& a, const object_ref& b) { return a.data_ptr != b.data_ptr; };

				friend class object_vector;
				friend class object_vector::iterator;
			};

			class iterator
			{
			public:
				using iterator_category = std::forward_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = object_ref;
				using pointer = object_ref*;
				using reference = object_ref&;

				inline reference operator*() { return current; }
				inline pointer operator->() { return &current; }

				inline iterator& operator++() 
				{
					current.data_ptr = static_cast<std::byte*>(current.data_ptr) + 
						push_data_size + offsets_size + element_base_size;
					current.binding_ptr += binding_increment;
					return *this;
				}
				inline iterator operator++(int)
				{
					iterator tmp = *this;
					current.data_ptr = static_cast<std::byte*>(current.data_ptr) +
						push_data_size + offsets_size + element_base_size;
					current.binding_ptr += binding_increment;
					return tmp;
				}

				friend inline bool operator==(const iterator& a, const iterator& b) { return a.current == b.current; };
				friend inline bool operator!=(const iterator& a, const iterator& b) { return a.current != b.current; };

			private:
				iterator() = delete;
				iterator(void* objects, buffer_binding_args* bindings, std::size_t object_increment, 
					std::uint32_t binding_increment, std::size_t push_data_size, std::size_t offsets_size) noexcept :
					current(objects, bindings, push_data_size, offsets_size), object_increment(object_increment), 
					binding_increment(binding_increment), push_data_size(push_data_size), offsets_size(offsets_size)
				{ }

				object_ref current;

				std::size_t object_increment = 0;
				std::uint32_t binding_increment = 0;

				std::size_t push_data_size = 0;
				std::size_t offsets_size = 0;

				friend class object_vector;
			};

			object_ref add();

			inline iterator begin() 
			{
				return iterator(object_data, object_descriptor_data, element_stride, n_descriptors, push_data_size, 
					n_dynamic_descriptors * sizeof(offset_t));
			}
			inline iterator end()
			{
				void* objects = static_cast<std::byte*>(object_data) + count * element_stride;
				buffer_binding_args* bindings = object_descriptor_data + count * n_descriptors;
				return iterator(objects, bindings, element_stride, n_descriptors, push_data_size,
					n_dynamic_descriptors * sizeof(offset_t));
			}

			inline object_ref operator[](std::size_t idx)
			{
				INTERNAL_ASSERT(idx < count, "Index out of bounds");
				void* object_p = static_cast<std::byte*>(object_data) + idx * element_stride;
				buffer_binding_args* binding_p = object_descriptor_data + idx * n_descriptors;
				return object_ref(object_p, binding_p, push_data_size, n_dynamic_descriptors * sizeof(offset_t));
			}

		private:
			std::size_t count = 0;
			std::size_t capacity = 0;

			std::size_t push_data_size = 0;
			std::uint32_t n_descriptors = 0;
			std::uint32_t n_dynamic_descriptors = 0;

			std::size_t element_stride = 0;
			std::size_t descriptor_stride = 0;

			void* object_data = nullptr;
			
			//bindings are only used when updating descriptors, so theyre stored in a seperate buffer
			buffer_binding_args* object_descriptor_data = nullptr;
			
			//smp is a temporary object used for swapping, it is part of the class to avoid allocating memory with every swap
			void* smp = nullptr;

			static const std::size_t element_base_size = sizeof(base_object);
		};

		using task_properties = object_vector::properties_t;
		typedef object_vector::offset_t dynamic_offset_t;

		std::unordered_map<std::size_t, std::size_t> object_refs; //TODO change key to memory_ref to avoid collisions
		object_vector objects;

		std::vector<buffer_binding_args> cached_object_bindings;
		std::vector<VkDescriptorBufferInfo> cached_buffer_infos;

		static void draw_object(VkCommandBuffer& buffer, const task_properties& obj);

		std::vector<VkWriteDescriptorSet> generate_descriptor_write(std::uint8_t current_frame);

		inline static const char* debug_descriptor_type(VkDescriptorType type)
		{
			switch (type)
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:						return "Sampler";
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:			return "Image sampler";
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:					return "Sampled image";
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:					return "Storage image";
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:			return "Texel uniform";
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:			return "Texel storage";
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:					return "Uniform";
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:					return "Storage";
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:			return "Dynamic uniform";
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:			return "Dynamic storage";
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:				return "Input attachment";
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:		return "Inline uniform";
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:		return "Acceleration structure";
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:		return "Acceleration structure NV";
			case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:					return "Mutable valve";
			case VK_DESCRIPTOR_TYPE_MAX_ENUM:						return "MAX ENUM";
			default:
				break;
			}
			return "INVALID TYPE";
		}
	};
}
