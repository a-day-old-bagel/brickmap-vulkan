#include "worker.h"
#include "device_context.h"

namespace rebel_road
{
    namespace vulkan
    {
        worker::~worker()
        {
            device_ctx->device.destroyFence( fence );
            device_ctx->device.destroyCommandPool( command_pool );
        }

        std::shared_ptr<worker> worker::create( device_context* in_ctx )
        {
            return std::make_shared<worker>( in_ctx );
        }

        worker::worker( device_context* in_ctx ) : device_ctx( in_ctx )
        {
    		auto fence_info = vulkan::fence_create_info();
            fence = device_ctx->device.createFence( fence_info );

		    auto command_pool_info = vulkan::command_pool_create_info( device_ctx->graphics_queue_family );
        	command_pool = device_ctx->device.createCommandPool( command_pool_info );
        }

        void worker::immediate_submit( std::function<void( vk::CommandBuffer cmd )>&& func )
        {
    		auto device = device_ctx->device;

            vk::CommandBufferAllocateInfo cmd_alloc_info = vulkan::command_buffer_allocate_info( command_pool, 1 );
            vk::CommandBuffer cmd = device.allocateCommandBuffers( cmd_alloc_info )[0];

            auto cmd_begin_info = vulkan::command_buffer_begin_info( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );
            VK_CHECK( cmd.begin( &cmd_begin_info ) );

            func( cmd );

            cmd.end();

            vk::SubmitInfo submit_info = vulkan::submit_info( &cmd );
            VK_CHECK( device_ctx->graphics_queue.submit( 1, &submit_info, fence ) );
            VK_CHECK( device.waitForFences( 1, &fence, true, std::numeric_limits<int>::max() ) );
            VK_CHECK( device.resetFences( 1, &fence ) );

            device.resetCommandPool( command_pool );
        }
    }
}