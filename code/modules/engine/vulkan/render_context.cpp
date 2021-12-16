#include "render_context.h"
#include "descriptors.h"

#include <imgui.h>

namespace rebel_road
{

    namespace vulkan
    {

        render_context::~render_context()
        {        }

        void render_context::shutdown()
        {
            device_ctx->device.waitIdle();
            deletion_queue.flush();
        }

        std::shared_ptr<render_context> render_context::create( device_context* in_context )
        {
            return std::make_shared<render_context>( in_context );
        }

        render_context::render_context( device_context* in_context ) : device_ctx( in_context )
        {
            for ( int i = 0; i < FRAME_OVERLAP; i++ )
            {
                frames[i].context.device = in_context;
                frames[i].context.render = this;
            }

            init_commands();
            init_sync_structures();
            init_profiling();
            init_descriptor_allocator();
        }

        void render_context::init_commands()
        {
            auto command_pool_info = vulkan::command_pool_create_info( device_ctx->graphics_queue_family, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
            for ( int i = 0; i < FRAME_OVERLAP; i++ )
            {
                frames[i].command_pool = device_ctx->device.createCommandPool( command_pool_info );

                auto cmd_alloc_info = vulkan::command_buffer_allocate_info( frames[i].command_pool );
                frames[i].command_buffer = device_ctx->device.allocateCommandBuffers( cmd_alloc_info )[0];

                deletion_queue.push_function( [=] () { device_ctx->device.destroyCommandPool( frames[i].command_pool ); } );
            }
        }

        void render_context::init_profiling()
        {
            graphics_profiling_context = TracyVkContext( device_ctx->defaultGPU, device_ctx->device, device_ctx->graphics_queue, frames[0].command_buffer );
            deletion_queue.push_function( [=] () { TracyVkDestroy( graphics_profiling_context ); } );
        }

        void render_context::init_sync_structures()
        {
            auto fence_info = vulkan::fence_create_info( vk::FenceCreateFlagBits::eSignaled );
            auto semaphore_info = vulkan::semaphore_create_info();

            for ( int i = 0; i < FRAME_OVERLAP; i++ )
            {
                VK_CHECK( device_ctx->device.createFence( &fence_info, nullptr, &frames[i].render_fence ) );
                VK_CHECK( device_ctx->device.createSemaphore( &semaphore_info, nullptr, &frames[i].present_semaphore ) );
                VK_CHECK( device_ctx->device.createSemaphore( &semaphore_info, nullptr, &frames[i].render_semaphore ) );

                deletion_queue.push_function( [=] ()
                    {
                        device_ctx->device.destroyFence( frames[i].render_fence );
                        device_ctx->device.destroySemaphore( frames[i].present_semaphore );
                        device_ctx->device.destroySemaphore( frames[i].render_semaphore );
                    } );
            }
        }

        void render_context::init_descriptor_allocator()
        {
            descriptor_alloc = new vulkan::descriptor_allocator {};
            descriptor_alloc->init( device_ctx->device );
            deletion_queue.push_function( [=] () { descriptor_alloc->destroy_pools(); } );

            descriptor_lc = new vulkan::descriptor_layout_cache {};
            descriptor_lc->init( device_ctx->device );
            deletion_queue.push_function( [=] () { descriptor_lc->shutdown(); } );

            for ( int i = 0; i < FRAME_OVERLAP; i++ )
            {
                frames[i].dynamic_descriptor_allocator = new vulkan::descriptor_allocator {};
                frames[i].dynamic_descriptor_allocator->init( device_ctx->device );
                deletion_queue.push_function( [=] () { frames[i].dynamic_descriptor_allocator->destroy_pools(); } );

                // 1 Mb Dynamic Data Buffer
                buffer<uint8_t> dynamic_data_buffer;
                dynamic_data_buffer.allocate( 1'000'000, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_ONLY );
                frames[i].dynamic_data.init( dynamic_data_buffer, device_ctx->gpu_props.limits.minUniformBufferOffsetAlignment );
                deletion_queue.push_function( [=] () { frames[i].dynamic_data.free(); } );
            }
        }

        vk::CommandBuffer render_context::begin_frame()
        {
            frame_number++;
            
            current_frame().begin_frame();

            frame_cmd = current_frame().command_buffer;
            auto cmd_begin_info = command_buffer_begin_info( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );
            VK_CHECK( frame_cmd.begin( &cmd_begin_info ) );

            return frame_cmd;
        }

        void render_context::end_frame()
        {
            frame_cmd.end();
        }

        void render_context::submit_and_present()
        {
            current_frame().submit_and_present();
        }

        frame& render_context::current_frame()
        {
            return frames[frame_number % FRAME_OVERLAP];
        }

        frame& render_context::previous_frame()
        {
            return frames[( frame_number - 1 ) % 2];
        }

        vk::Image render_context::get_swapchain_image()
        {
            return device_ctx->swapchain_images[current_frame().swapchain_image_index];
        }

        uint32_t render_context::get_swapchain_image_index()
        {
            return current_frame().swapchain_image_index;
        }

        void render_context::render_stats()
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
            ImGui::Begin( "Render Stats", nullptr, window_flags );
            ImGui::Separator();
            ImGui::Text( "Vulkan Resource Cache" );
            device_ctx->render_stats();
            get_descriptor_layout_cache()->render_stats();
            ImGui::End();
        }

    }

}