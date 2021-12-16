#pragma once

#include "containers/deletion_queue.h"
#include "vulkan/buffer.h"
#include "stage/camera.h"
#include "world.h"

namespace rebel_road
{

    namespace vulkan
    {
        class render_context;
        class worker;
        class image;
    }

    namespace voxel
    {
        constexpr uint32_t rq_buf_size = 2 * 1'048'576; // Adjusted for the initial settings. Not optimized.

        struct gpu_ray
        {
            glm::vec4 origin;
            glm::vec4 direction;
            glm::vec4 throughput;
            glm::vec4 normal;
            float distance {};
            int identifier {};
            int bounces {};
            uint32_t pixel_index {};
        };

        struct gpu_shadow_ray
        {
            glm::vec4 origin;
            glm::vec4 direction;
            glm::vec4 color;
            uint32_t pixel_index {};
            uint32_t pad0;
            uint32_t pad1;
            uint32_t pad2;
        };

        struct gpu_wavefront_state
        {
            uint32_t start_position {};
            uint32_t primary_ray_count {};
            uint32_t shadow_ray_count {};
            uint32_t ray_number_primary {};
            uint32_t ray_number_extend {};
            uint32_t ray_number_shade {};
            uint32_t ray_number_connect {};
            uint32_t ray_queue_buffer_size { rq_buf_size };
        };

        struct gpu_push_constants
        {
            uint32_t frame {};
            uint32_t render_width {};
            uint32_t render_height {};
            uint32_t pad0 {};
            glm::vec4 camera_direction {};
            glm::vec4 camera_up {};
            glm::vec4 camera_right {};
            glm::vec4 camera_position {};
            float focal_distance {};
            float lens_radius {};
            uint32_t enable_depth_of_field {};
            uint32_t render_mode {};
            glm::vec2 sun_position {};
        };

        class ray_tracer
        {
        public:
            static std::unique_ptr<ray_tracer> create( vulkan::render_context* in_render_ctx, vk::Extent2D in_render_extent );
            ray_tracer() = delete;
            ray_tracer( vulkan::render_context* in_render_ctx, vk::Extent2D in_render_extent );

            void shutdown();

            void bind_world( std::shared_ptr<world> in_world );
            void update_camera( stage::camera& camera );

            void compute_rays();
            void draw( vk::CommandBuffer cmd );

            // temp:
            glm::vec2 sun_position { 0.005, 0.1 };
            uint32_t render_mode{};

        private:
            void init();
            void init_primary_rays();
            void init_global_state();
            void init_extend();
            void init_shade();
            void init_connect();

            vulkan::buffer<gpu_wavefront_state> global_state;
            vk::DescriptorSet global_state_set;
            vk::Pipeline global_state_pipeline;
            vk::PipelineLayout global_state_layout;

            vulkan::buffer<gpu_ray> primary_rays[2];
            vk::DescriptorSet primary_rays_set[2];
            gpu_push_constants push_constants;
            vk::Pipeline primary_rays_pipeline;
            vk::PipelineLayout primary_rays_layout;
            uint32_t primary_rays_index {};

            vulkan::buffer<gpu_shadow_ray> shadow_rays;
            vk::DescriptorSet shadow_rays_set;

            vk::Pipeline extend_pipeline;
            vk::PipelineLayout extend_layout;
            vk::DescriptorSet extend_set;
            vk::DescriptorSet world_set;

            vk::Pipeline shade_pipeline;
            vk::PipelineLayout shade_layout;

            vk::Pipeline connect_pipeline;
            vk::PipelineLayout connect_layout;

            vulkan::buffer<glm::vec4> blit_buf;
            vk::DescriptorSet blit_set_c;
            vk::DescriptorSet blit_set_f;

            vk::Extent2D render_extent;
            vk::RenderPass render_pass;
            vk::Pipeline render_pipeline;
            vk::PipelineLayout render_layout;
            std::vector<vk::Framebuffer> framebuffers;
            std::vector<std::shared_ptr<vulkan::image>> render_targets;

            vk::CommandPool command_pool;
            vk::CommandBuffer command_buffer;

            vulkan::render_context* render_ctx {};
            vulkan::device_context* device_ctx {};
            std::shared_ptr<vulkan::worker> worker;

            util::deletion_queue deletion_queue;

            uint32_t frame {};

            std::shared_ptr<world> voxel_world;
        };

    }

}