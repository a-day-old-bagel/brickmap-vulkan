#pragma once

#include "device_context.h"
#include "frame.h"

namespace tracy { class VkCtx; }

namespace rebel_road
{

    namespace vulkan
    {

        constexpr uint32_t FRAME_OVERLAP = 2;

        class device_context;
        class descriptor_allocator;
        class descriptor_layout_cache;

        class render_context
        {
        public:
            static std::shared_ptr<render_context> create( device_context* in_context );

            ~render_context();
            render_context() = delete;
            render_context( device_context* in_context );

            void shutdown();

            void render_stats();

            vk::CommandBuffer begin_frame();
            void end_frame();
            void submit_and_present();

            vk::Queue get_graphics_queue() { return device_ctx->graphics_queue; }

            uint32_t get_frame_number() { return frame_number; } 
            frame& current_frame();
            frame& last_frame();

            uint32_t get_swapchain_image_index();
            vk::Image get_swapchain_image();

            descriptor_layout_cache* get_descriptor_layout_cache() const { return descriptor_layout_cache; }
            descriptor_allocator* get_descriptor_allocator() const { return descriptor_allocator; }

            device_context* get_device_context() const { return device_ctx; }

            uint32_t get_frame_count() { return FRAME_OVERLAP; }

        private:

            void init_commands();
            void init_sync_structures();
            void init_profiling();
            void init_descriptor_allocator();

            frame frames[FRAME_OVERLAP];

            descriptor_allocator* descriptor_allocator { nullptr };
            descriptor_layout_cache* descriptor_layout_cache { nullptr };

            uint32_t frame_number { 0 };

            device_context* device_ctx {};
            tracy::VkCtx* graphics_profiling_context;
            util::deletion_queue deletion_queue;

            vk::CommandBuffer frame_cmd; // Valid between begin_frame and end_frame;
        };

    } // namespace vulkan

} // namespace rebel_road