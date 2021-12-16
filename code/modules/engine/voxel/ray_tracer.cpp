#include "ray_tracer.h"
#include "vulkan/render_context.h"
#include "vulkan/worker.h"
#include "vulkan/render_pass_builder.h"
#include "vulkan/pipeline_builder.h"
#include "vulkan/shader.h"
#include "vulkan/image.h"

namespace rebel_road
{
    namespace voxel
    {

        std::unique_ptr<ray_tracer> ray_tracer::create( vulkan::render_context* in_render_ctx, vk::Extent2D in_render_extent )
        {
            return std::make_unique<ray_tracer>( in_render_ctx, in_render_extent );
        }

        ray_tracer::ray_tracer( vulkan::render_context* in_render_ctx, vk::Extent2D in_render_extent )
            : render_ctx( in_render_ctx ), device_ctx( in_render_ctx->get_device_context() ), render_extent( in_render_extent )
        {
            worker = vulkan::worker::create( device_ctx );

            init();
            init_primary_rays();
            init_global_state();
            init_extend();
            init_shade();
            init_connect();
        }

        void ray_tracer::shutdown()
        {
            shadow_rays.free();
            primary_rays[0].free();
            primary_rays[1].free();
            global_state.free();
            blit_buf.free();
            worker.reset();
            voxel_world.reset();
        }

        void ray_tracer::init()
        {
            // Command Pool
            command_pool = device_ctx->create_command_pool( device_ctx->graphics_queue_family, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
            auto cmd_alloc_info = vulkan::command_buffer_allocate_info( command_pool );
            command_buffer = device_ctx->device.allocateCommandBuffers( cmd_alloc_info )[0];

            auto render_pass_builder = vulkan::render_pass_builder::begin( device_ctx )
                .add_color_attachment( device_ctx->get_swapchain_image_format(), vk::ImageLayout::eUndefined,
                    vk::ImageLayout::ePresentSrcKHR, vk::AttachmentLoadOp::eClear )
                .add_default_subpass_dependency()
                .build();

            render_pass = render_pass_builder.get_render_pass();

            for ( int i = 0; i < 2; i++ )
            {
                vk::Extent3D image_extent3D { render_extent.width, render_extent.height, 1 };
                render_targets.push_back( device_ctx->create_render_target( device_ctx->get_swapchain_image_format(), vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, image_extent3D, vk::ImageAspectFlagBits::eColor, 1 ) );
                worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                    {
                        vk::ImageSubresourceRange range = vulkan::image_subresource_range( 0, 1 );
                        render_targets[i]->transition_layout( cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits::eColorAttachmentWrite, range );
                    }
                );

                std::vector<vk::ImageView> attachments { render_targets[i]->default_view };
                framebuffers.push_back( device_ctx->create_framebuffer( render_pass, attachments, render_extent ) );
            }

            vulkan::shader_module* ray_tracer_vert = device_ctx->create_shader_module( "ray_tracer_visualizer.vert.spv" );
            vulkan::shader_module* ray_tracer_frag = device_ctx->create_shader_module( "ray_tracer_visualizer.frag.spv" );

            auto shader_builder = vulkan::shader_layout_builder::begin( device_ctx )
                .add_stage( ray_tracer_vert, vk::ShaderStageFlagBits::eVertex )
                .add_stage( ray_tracer_frag, vk::ShaderStageFlagBits::eFragment )
                .build();

            render_layout = shader_builder.get_layout();

            auto pipeline_builder = vulkan::graphics_pipeline_builder::begin( device_ctx )
                .set_cache_key( "octree tracer" )
                .use_default_color_blend()
                .disable_depth_stencil()
                .use_default_multisampling()
                .set_rasterization( vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, false, vk::FrontFace::eCounterClockwise )
                .set_input_assembly( vk::PrimitiveTopology::eTriangleList )
                .add_dynamic_state( vk::DynamicState::eViewport )
                .add_dynamic_state( vk::DynamicState::eScissor )
                .set_shaders( shader_builder )
                .build( render_pass );

            render_pipeline = pipeline_builder.get_pipeline();
        }

        void ray_tracer::init_primary_rays()
        {
            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "rt_0_primary_rays.comp.spv", device_ctx );
            primary_rays_pipeline = pipe; primary_rays_layout = layout;

            // buffers
            primary_rays[0].allocate( rq_buf_size * sizeof( gpu_ray ), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
            primary_rays[1].allocate( rq_buf_size * sizeof( gpu_ray ), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
            shadow_rays.allocate( rq_buf_size * sizeof( gpu_shadow_ray ), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );

            // descriptor set
            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, primary_rays[0].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, primary_rays[1].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( primary_rays_set[0] );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, primary_rays[1].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, primary_rays[0].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( primary_rays_set[1] );
        }

        void ray_tracer::init_global_state()
        {
            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "rt_1_update_global_state.comp.spv", device_ctx );
            global_state_pipeline = pipe; global_state_layout = layout;

            // buffer
            global_state.allocate( sizeof( gpu_wavefront_state ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_CPU_TO_GPU );

            // Upload initial zero initialized state. The buffer size will be non-zero.
            gpu_wavefront_state initial_state;
            global_state.upload_to_buffer( &initial_state, sizeof( initial_state ) );

            // descriptor set
            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, global_state.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( global_state_set );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, global_state.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, shadow_rays.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( shadow_rays_set );
        }

        void ray_tracer::init_extend()
        {
            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "rt_2_extend.comp.spv", device_ctx );
            extend_pipeline = pipe; extend_layout = layout;

            // descriptor set
            // will be created when a world is bound
        }

        void ray_tracer::init_shade()
        {
            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "rt_3_shade.comp.spv", device_ctx );
            shade_pipeline = pipe; shade_layout = layout;

            // blit
            blit_buf.allocate( render_extent.width * render_extent.height * sizeof( glm::vec4 ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY );

            // descriptor set
            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, blit_buf.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( blit_set_c );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, blit_buf.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment )
                .build( blit_set_f );
        }

        void ray_tracer::init_connect()
        {
            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "rt_4_connect.comp.spv", device_ctx );
            connect_pipeline = pipe; connect_layout = layout;
        }

        void ray_tracer::bind_world( std::shared_ptr<world> in_world )
        {
            // Currently this code assumes a world will always be bound just after creation.
            voxel_world = in_world;

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, voxel_world->get_index_buffer_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, voxel_world->get_brick_buffer_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 2, voxel_world->get_world_buffer_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 3, voxel_world->get_load_queue_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( extend_set );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, voxel_world->get_world_buffer_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( world_set );
        }

        void ray_tracer::update_camera( stage::camera& camera )
        {
            ZoneScopedN( "ray tracer - update camera" );

            camera.update();
            glm::vec3 camera_right = glm::normalize( glm::cross( camera.direction, camera.up ) ) * 1.5f * ( (float) render_extent.width / render_extent.height );
            glm::vec3 camera_up = glm::normalize( glm::cross( camera_right, camera.direction ) ) * 1.5f;

            push_constants.camera_direction = glm::vec4( camera.direction, 1 );
            push_constants.camera_right = glm::vec4( camera_right, 1 );
            push_constants.camera_up = glm::vec4( camera_up, 1 );
            push_constants.camera_position = glm::vec4( camera.position, 1 );
            push_constants.focal_distance = camera.focal_distance;
            push_constants.lens_radius = camera.lens_radius;
            push_constants.enable_depth_of_field = camera.enable_depth_of_field;

            push_constants.sun_position = sun_position;
            push_constants.render_mode = render_mode;
        }

        void ray_tracer::draw( vk::CommandBuffer cmd )
        {
            ZoneScopedN( "ray tracer - blit" );
            //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Blit" );

            uint32_t back_buffer = ( primary_rays_index + 1 ) % 2;

            vk::ImageSubresourceRange subresource_range = vulkan::image_subresource_range( 0, VK_REMAINING_MIP_LEVELS );

            render_targets[back_buffer]->transition_layout( cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, {}, vk::AccessFlagBits::eTransferRead, subresource_range );
            vulkan::image::transition_layout( cmd, render_ctx->get_swapchain_image(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, vk::AccessFlagBits::eTransferWrite, subresource_range );

            vk::Extent3D render_extent_3d { render_extent.width, render_extent.height, 1 };
            vk::ImageCopy copy_region = vulkan::image_copy( render_extent_3d, 0, 0 );
            cmd.copyImage( render_targets[back_buffer]->vk_image, vk::ImageLayout::eTransferSrcOptimal, render_ctx->get_swapchain_image(), vk::ImageLayout::eTransferDstOptimal, 1, &copy_region );

            render_targets[back_buffer]->transition_layout( cmd, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits::eShaderRead, subresource_range );
            vulkan::image::transition_layout( cmd, render_ctx->get_swapchain_image(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, {}, vk::AccessFlagBits::eTransferRead, subresource_range );
        }

        void ray_tracer::compute_rays()
        {
            ZoneScopedN( "ray tracer - compute rays" );

            auto& cmd = command_buffer;
            cmd.reset( {} );
            vk::CommandBufferBeginInfo begin_info {};
            cmd.begin( begin_info );
            {
                //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Compute Rays" );

                const int num_dispatch = ( rq_buf_size / 128 );
                std::vector<vk::DescriptorSet> descriptor_sets;

                vk::MemoryBarrier memory_barrier {}; // Barrier to ensure each compute step completes writes before the next step reads.
                memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
                memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                // This seems incorrect.
                global_state.mapped_data()->primary_ray_count = 0;

                // Compute - Primary Rays
                {
                    //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Primary Rays" );

                    push_constants.frame = frame;
                    push_constants.render_width = render_extent.width;
                    push_constants.render_height = render_extent.height;
                    cmd.pushConstants( primary_rays_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof( push_constants ), &push_constants );

                    descriptor_sets = { primary_rays_set[primary_rays_index], global_state_set, blit_set_c };

                    cmd.bindPipeline( vk::PipelineBindPoint::eCompute, primary_rays_pipeline );
                    cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, primary_rays_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
                    cmd.dispatch( num_dispatch, 1, 1 );
                    cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 1, &memory_barrier, 0, nullptr, 0, nullptr );
                }

                // Compute - Update Global State
                {
                    //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Update Global State" );

                    descriptor_sets = { primary_rays_set[primary_rays_index], global_state_set };

                    cmd.bindPipeline( vk::PipelineBindPoint::eCompute, global_state_pipeline );
                    cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, global_state_layout, 0, 1, &global_state_set, 0, nullptr );
                    cmd.dispatch( 1, 1, 1 );
                    cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 1, &memory_barrier, 0, nullptr, 0, nullptr );
                }

                // Compute - Extend
                {
                    //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Extend" );

                    descriptor_sets = { primary_rays_set[primary_rays_index], global_state_set, extend_set, blit_set_c };

                    cmd.bindPipeline( vk::PipelineBindPoint::eCompute, extend_pipeline );
                    cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, extend_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
                    cmd.dispatch( num_dispatch, 1, 1 );
                    cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 1, &memory_barrier, 0, nullptr, 0, nullptr );
                }

                // Compute - Shade
                {
                    //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Shade" );

                    descriptor_sets = { primary_rays_set[primary_rays_index], shadow_rays_set, blit_set_c, world_set };

                    cmd.bindPipeline( vk::PipelineBindPoint::eCompute, shade_pipeline );
                    cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, shade_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
                    cmd.dispatch( num_dispatch, 1, 1 );
                    cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 1, &memory_barrier, 0, nullptr, 0, nullptr );
                }

                // Compute - Connect
                {
                    //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Connect" );

                    descriptor_sets = { shadow_rays_set, extend_set, blit_set_c };

                    cmd.bindPipeline( vk::PipelineBindPoint::eCompute, connect_pipeline );
                    cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, connect_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
                    cmd.dispatch( num_dispatch, 1, 1 );
                    cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 1, &memory_barrier, 0, nullptr, 0, nullptr );
                }

                cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAllGraphics, {}, 1, &memory_barrier, 0, nullptr, 0, nullptr );

                // Render to Framebuffer
                {
                    //TracyVkZone( render_ctx->get_tracy_context(), cmd, "Draw" );

                    vk::RenderPassBeginInfo rp_info = vulkan::renderpass_begin_info( render_pass, render_extent, framebuffers[frame % 2] );
                    vk::ClearValue clear_value { {std::array<float, 4>( { 0.5f, 0.3f, 0.8f, 1.0f } )} };
                    rp_info.pClearValues = &clear_value;
                    rp_info.clearValueCount = 1;

                    std::vector<vk::DescriptorSet> descriptor_sets = { blit_set_f };

                    cmd.beginRenderPass( &rp_info, vk::SubpassContents::eInline );
                    {
                        cmd.bindPipeline( vk::PipelineBindPoint::eGraphics, render_pipeline );
                        cmd.pushConstants( render_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof( push_constants ), &push_constants );
                        cmd.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, render_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );

                        vk::Viewport viewport {};
                        viewport.width = static_cast<float>( render_extent.width );
                        viewport.height = static_cast<float>( render_extent.height );
                        viewport.maxDepth = 1.0f;
                        cmd.setViewport( 0, 1, &viewport );

                        vk::Rect2D scissor {};
                        scissor.offset = vk::Offset2D { 0, 0 };
                        scissor.extent = { { render_extent.width, render_extent.height } };
                        cmd.setScissor( 0, 1, &scissor );

                        cmd.draw( 3, 1, 0, 0 );
                    }
                    cmd.endRenderPass();
                }
            }

            {
                //TracyVkCollect( render_ctx->get_tracy_context(), cmd );
            }
            cmd.end();

            uint64_t wait_value = voxel_world->get_ray_tracer_wait_value();
            uint64_t signal_value = voxel_world->get_ray_tracer_signal_value();

            vk::TimelineSemaphoreSubmitInfo timeline_info;
            timeline_info.waitSemaphoreValueCount = 1;
            timeline_info.pWaitSemaphoreValues = &wait_value;
            timeline_info.signalSemaphoreValueCount = 1;
            timeline_info.pSignalSemaphoreValues = &signal_value;

            vk::SubmitInfo submit_info = vulkan::submit_info( &cmd );
            vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eComputeShader;
            vk::Semaphore wait_semaphore = voxel_world->get_ray_tracer_wait_semaphore();
            vk::Semaphore signal_semaphore = voxel_world->get_ray_tracer_signal_semaphore();

            submit_info.pNext = &timeline_info;
            submit_info.pWaitDstStageMask = &wait_stage;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &wait_semaphore;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &signal_semaphore;

            VK_CHECK( render_ctx->get_graphics_queue().submit( 1, &submit_info, nullptr ) );

            frame++;
            primary_rays_index = ( primary_rays_index + 1 ) % 2;
        }

    }
}