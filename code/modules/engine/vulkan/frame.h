#pragma once

#include "push_buffer.h"
#include "descriptors.h"

namespace rebel_road
{

    namespace vulkan
    {

        class device_context;
        class render_context;

        class frame
        {
        public:
            void begin_frame();
            void submit_and_present();

            struct
            {
                device_context* device {};
                render_context* render {};
            } context;

            vk::Semaphore present_semaphore;
            vk::Semaphore render_semaphore;
            vk::Fence render_fence;

            util::deletion_queue frame_deletion_queue;

            vk::CommandPool command_pool;
            vk::CommandBuffer command_buffer;

            push_buffer dynamic_data;
            descriptor_allocator* dynamic_descriptor_allocator;

            uint32_t swapchain_image_index {};
        };

    } // namespace vulkan

} // namepsace rebel_road