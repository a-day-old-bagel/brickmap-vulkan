#include "frame.h"
#include "device_context.h"
#include "render_context.h"

namespace rebel_road
{

	namespace vulkan
	{

		void frame::begin_frame()
		{
			ZoneScopedN( "Begin Frame" );

			{
				ZoneScopedN( "Fence Wait" );
				uint64_t one_second = 10'000'000'000;
				VK_CHECK( context.device->device.waitForFences( 1, &render_fence, true, one_second ) );
				VK_CHECK( context.device->device.resetFences( 1, &render_fence ) );
			}

			dynamic_data.reset();
			frame_deletion_queue.flush();
			dynamic_descriptor_allocator->reset_pools();
			command_buffer.reset( {} );

			{
				ZoneScopedN( "Aquire Image" );
				swapchain_image_index = context.device->device.acquireNextImageKHR( context.device->swapchain, 0, present_semaphore );
			}
		}

		void frame::submit_and_present()
		{
			ZoneScopedN( "Submit" );

			vk::CommandBuffer cmd = command_buffer;

			auto submit_info = vulkan::submit_info( &cmd );
			vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			submit_info.pWaitDstStageMask = &wait_stage;
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &present_semaphore;
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &render_semaphore;
			VK_CHECK( context.render->get_graphics_queue().submit( 1, &submit_info, render_fence ) );

			auto present_info = vulkan::present_info();
			present_info.pSwapchains = &context.device->swapchain;
			present_info.swapchainCount = 1;
			present_info.pWaitSemaphores = &render_semaphore;
			present_info.waitSemaphoreCount = 1;
			present_info.pImageIndices = &swapchain_image_index;

			{
				ZoneScopedN( "Queue Present" );
				VK_CHECK( context.render->get_graphics_queue().presentKHR( present_info ) );
			}
		}

	} // namespace vulkan

} // namespace rebel_road