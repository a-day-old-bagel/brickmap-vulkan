#pragma once

#include "containers/deletion_queue.h"
#include "containers/service_locator.h"
#include "cache_keys.h"

struct GLFWwindow;

namespace rebel_road
{

    namespace gfx { class image; } // tmp??

    namespace vulkan
    {

        class render_context;
        class image;
        class render_pass_cache;        
        class shader_module;

        class device_context
        {
        public:
            struct create_info
            {
                std::string app_name {};
                bool use_validation_layers {};
                GLFWwindow* window {};
                vk::PhysicalDeviceFeatures required_features {};
                vk::PhysicalDeviceVulkan12Features required_features_12 {};
                std::vector<const char*> required_extensions {};
                VmaAllocatorCreateFlags alloc_flags;
            };

            static std::unique_ptr<device_context> create( const create_info& ci );

            ~device_context();

            void init_device( const create_info& ci );
            void init_swapchain();
            void init_queues();

            void shutdown();

            void render_stats();

            void resize( int width, int height );

            uint32_t get_swapchain_image_count();
            vk::ImageView get_swapchain_image_view( int idx );
            vk::Format get_swapchain_image_format();

            std::shared_ptr<render_context> create_render_context();
            std::shared_ptr<image> create_render_target( vk::Format image_format, vk::ImageUsageFlags image_usage, vk::Extent3D image_extent, vk::ImageAspectFlagBits image_aspect, uint32_t mip_levels = 1, bool swap_chain_lifetime = false );
            vk::RenderPass create_render_pass( vk::RenderPassCreateInfo render_pass_info );
            vk::Framebuffer create_framebuffer( vk::RenderPass render_pass, const std::vector<vk::ImageView>& attachments, vk::Extent2D extent, const std::string& cache_key = "", bool swap_chain_lifetime = false );
            shader_module* create_shader_module( const std::string& name );
            vk::DescriptorSetLayout create_descriptor_set_layout( const vk::DescriptorSetLayoutCreateInfo& create_info );
            vk::PipelineLayout create_pipeline_layout( const vk::PipelineLayoutCreateInfo& create_info, const std::string& key = "" );
            vk::PipelineLayout find_pipeline_layout( const std::string& key );
            vk::Pipeline create_graphics_pipeline( const vk::GraphicsPipelineCreateInfo& create_info, const std::string& key = "" );
            vk::Pipeline find_graphics_pipeline( const std::string& key );
            vk::Pipeline create_compute_pipeline( const vk::ComputePipelineCreateInfo& create_info );
            vk::ImageView create_image_view( const vk::ImageViewCreateInfo& create_info, bool swap_chain_lifetime = false );
            std::pair<vk::Image, VmaAllocation> create_image( const vk::ImageCreateInfo& create_info, const VmaAllocationCreateInfo& alloc_info, bool swap_chain_lifetime = false );
            vk::Sampler create_sampler( const vk::SamplerCreateInfo& create_info );
            std::vector<vk::Framebuffer> create_swap_chain_framebuffers( vk::RenderPass render_pass, vk::Extent2D render_extent, std::vector<vk::ImageView> attachments = {} );
            vk::Semaphore create_semaphore( const vk::SemaphoreCreateInfo create_info );
            vk::CommandPool create_command_pool( uint32_t family, vk::CommandPoolCreateFlags flags );

            vkb::Instance vkb_instance;
            vkb::Device vkb_device;
            vkb::Swapchain vkb_swapchain;

            vk::PhysicalDevice defaultGPU;
            vk::Device device;
            vk::SurfaceKHR surface;
            vk::SwapchainKHR swapchain;

            vk::PhysicalDeviceProperties gpu_props;
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_props {};
            vk::PhysicalDeviceConservativeRasterizationPropertiesEXT conservative_raster_props {};

            VmaAllocator allocator;

            std::vector<vk::Image> swapchain_images;
            std::vector<vk::ImageView> swapchain_image_views;

            vk::Queue compute_queue;
            uint32_t compute_queue_family;
            vk::Queue graphics_queue;
            uint32_t graphics_queue_family;
            vk::Queue transfer_queue;
            uint32_t transfer_queue_family;

            // Vulkan resource caches.
            // Strings are error prone, poor man's cache keys that will work for now.
            std::unordered_map<render_pass_key, vk::RenderPass, render_pass_hash> render_pass_cache;
            std::unordered_map<std::string, vk::Framebuffer> framebuffer_cache;
            std::unordered_map<std::string, std::unique_ptr<shader_module>> shader_module_cache;
            std::unordered_map<std::string, vk::PipelineLayout> pipeline_layout_cache; // Could later be changed to be keyed by create info.
            std::unordered_map<std::string, vk::Pipeline> pipeline_cache; // Could later be changed to be keyed by create info.

        private:
            util::deletion_queue resize_deletion_queue;
            util::deletion_queue deletion_queue;
        };

        using device_context_locator = service_locator<device_context>;

    } // namespace vulkan

} // namespace rebel_road