#pragma once

namespace rebel_road
{
    namespace vulkan
    {
        // We do a naughty thing and return a pointer to the contents of a vector.
        // In order to prevent reallocation shifts in the underlying data between calls to bind_*() and build()
        // we will reserve a certain amount of space. No problem increasing this if it isn't enough.
        constexpr uint32_t reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set = 10;

        // Pool sizes are defined as a proportion of max_pool_size.
        constexpr uint32_t max_pool_size = 1000;

        // https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/
        class descriptor_allocator
        {
        public:

            struct pool_sizes
            {
                std::vector<std::pair<vk::DescriptorType, float>> sizes =
                {
                    { vk::DescriptorType::eSampler, 0.5f },
                    { vk::DescriptorType::eCombinedImageSampler, 4.f },
                    { vk::DescriptorType::eSampledImage, 4.f },
                    { vk::DescriptorType::eStorageImage, 1.f },
                    { vk::DescriptorType::eUniformTexelBuffer, 1.f },
                    { vk::DescriptorType::eStorageTexelBuffer, 1.f },
                    { vk::DescriptorType::eUniformBuffer, 2.f },                // Up to 64k, faster access.
                    { vk::DescriptorType::eStorageBuffer, 2.f },                // Large buffers, slower access.
                    { vk::DescriptorType::eUniformBufferDynamic, 1.f },
                    { vk::DescriptorType::eStorageBufferDynamic, 1.f },
                    { vk::DescriptorType::eInputAttachment, 0.5f },
                    { vk::DescriptorType::eAccelerationStructureKHR, 1.f }
                };
            };

            void init( vk::Device new_device );

            void destroy_pools();
            void reset_pools();

            bool allocate( vk::DescriptorSet& set, vk::DescriptorSetLayout layout, uint32_t variable_alloc_count=0 );

            vk::Device device;

        private:
            vk::DescriptorPool grab_pool();
    		vk::DescriptorPool create_pool( vk::Device device, int count, vk::DescriptorPoolCreateFlags flags );

            vk::DescriptorPool current_pool { VK_NULL_HANDLE };
            pool_sizes descriptor_sizes;
            std::vector<vk::DescriptorPool> used_pools;
            std::vector<vk::DescriptorPool> free_pools;
        };

        class descriptor_layout_cache
        {
        public:

            void init( vk::Device newDevice );
            void shutdown();
            void render_stats();

            vk::DescriptorSetLayout create_descriptor_set_layout( const vk::DescriptorSetLayoutCreateInfo& info );

            struct descriptor_layout_key
            {
                struct combined_binding
                {
                    vk::DescriptorSetLayoutBinding binding;
                    vk::DescriptorBindingFlags flags;
                };
                std::vector<combined_binding> bindings;

                bool operator==( const descriptor_layout_key& other ) const;
                size_t hash() const;
            };

        private:

            struct descriptor_layout_hash
            {
                std::size_t operator()( const descriptor_layout_key& k ) const
                {
                    return k.hash();
                }
            };

            std::unordered_map< descriptor_layout_key, vk::DescriptorSetLayout, descriptor_layout_hash> layout_cache;
            vk::Device device;
        };

        class descriptor_builder
        {
        public:
            static descriptor_builder begin( descriptor_layout_cache* layout_cache, descriptor_allocator* allocator );

            descriptor_builder& bind_buffer( uint32_t binding, vk::DescriptorBufferInfo buffer_info, vk::DescriptorType type, vk::ShaderStageFlags stage_flags );
            descriptor_builder& bind_image( uint32_t binding, vk::DescriptorImageInfo image_info, vk::DescriptorType type, vk::ShaderStageFlags stage_flags );
            descriptor_builder& bind_image_array( uint32_t binding, vk::DescriptorImageInfo* desc_image_infos, uint32_t current_count, uint32_t max_count, vk::DescriptorType type, vk::ShaderStageFlags stage_flags );
            descriptor_builder& bind_acceleration_structure( uint32_t binding, vk::WriteDescriptorSetAccelerationStructureKHR as_info );

            bool build( vk::DescriptorSet& set, vk::DescriptorSetLayout& layout );
            bool build( vk::DescriptorSet& set );

        private:
            std::vector<vk::WriteDescriptorSet> writes;
            std::vector<vk::DescriptorSetLayoutBinding> bindings;
            std::vector<vk::DescriptorBindingFlags> binding_flags;
            std::vector<vk::DescriptorImageInfo> image_infos;
            std::vector<vk::DescriptorBufferInfo> buffer_infos;
            std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> as_infos;

            descriptor_layout_cache* cache {};
            descriptor_allocator* alloc {};
            uint32_t variable_alloc_count{};
        };
    }
}