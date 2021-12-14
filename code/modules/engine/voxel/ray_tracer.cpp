#include "ray_tracer.h"
#include "vulkan/render_context.h"
#include "vulkan/worker.h"
#include "vulkan/render_pass_builder.h"
#include "vulkan/pipeline_builder.h"
#include "vulkan/shader.h"

namespace rebel_road
{
    namespace voxel
    {

        std::unique_ptr<ray_tracer> ray_tracer::create( vulkan::render_context* in_render_ctx, vk::Extent2D in_render_extent, std::shared_ptr<voxel::world> in_world )
        {
            return std::make_unique<ray_tracer>( in_render_ctx, in_render_extent, in_world );
        }

        ray_tracer::ray_tracer( vulkan::render_context* in_render_ctx, vk::Extent2D in_render_extent, std::shared_ptr<voxel::world> in_world )
            : render_ctx( in_render_ctx ), device_ctx( in_render_ctx->get_device_context() ), render_extent( in_render_extent ), world( in_world )
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
            world.reset();
        }

        void ray_tracer::init()
        {
            auto render_pass_builder = vulkan::render_pass_builder::begin( device_ctx )
                .add_color_attachment( device_ctx->get_swapchain_image_format(), vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR, vk::AttachmentLoadOp::eClear )
                .add_default_subpass_dependency()
                .build();

            render_pass = render_pass_builder.get_render_pass();
            framebuffers = device_ctx->create_swap_chain_framebuffers( render_pass, render_extent, {} );

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
                .build( primary_rays_set[0] );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, primary_rays[1].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( primary_rays_set[1] );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, primary_rays[0].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment )
                .build( primary_rays_vis_set[0] );

            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, primary_rays[1].get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment )
                .build( primary_rays_vis_set[1] );
        }

        void ray_tracer::init_global_state()
        {
            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "rt_1_update_global_state.comp.spv", device_ctx );
            global_state_pipeline = pipe; global_state_layout = layout;

            // buffer
            global_state.allocate( sizeof( gpu_wavefront_state ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY );

            // Upload initial zero initialized state. The buffer size will be non-zero.
            gpu_wavefront_state initial_state;
            vulkan::buffer<gpu_wavefront_state> global_state_staging;
            global_state_staging.allocate( sizeof( gpu_wavefront_state ), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY );
            global_state_staging.upload_to_buffer( &initial_state, sizeof( gpu_wavefront_state ) );

            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::BufferCopy copy {};
                    copy.size = global_state_staging.size;
                    cmd.copyBuffer( global_state_staging.buf, global_state.buf, 1, &copy );
                } );

            global_state_staging.free();

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
            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, world->get_supergrid_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, world->get_load_queue_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( extend_set );
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

        void ray_tracer::update_camera( stage::camera& camera )
        {
            camera.update();
	        glm::vec3 camera_right = glm::normalize(glm::cross(camera.direction, camera.up)) * 1.5f * ( (float)render_extent.width / render_extent.height );
        	glm::vec3 camera_up = glm::normalize(glm::cross(camera_right, camera.direction)) * 1.5f;

            push_constants.camera_direction = glm::vec4( camera.direction, 1 );
            push_constants.camera_right = glm::vec4( camera_right, 1 );
            push_constants.camera_up = glm::vec4( camera_up, 1 );
            push_constants.camera_position = glm::vec4( camera.position, 1 );
            push_constants.focal_distance = 1.f;//camera.focal_distance;
            push_constants.lens_radius = 0.02f;//camera.lens_radius;

            push_constants.sun_position = sun_position;
        }

        void ray_tracer::blit( vk::CommandBuffer cmd )
        {
            vk::RenderPassBeginInfo rp_info = vulkan::renderpass_begin_info( render_pass, render_extent, framebuffers[render_ctx->get_swapchain_image_index()] );
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

        void ray_tracer::render( vk::CommandBuffer cmd )
        {
            // Compute - Primary Rays

            const int num_dispatch = ( rq_buf_size / 128 );
            std::vector<vk::DescriptorSet> descriptor_sets;

            push_constants.frame = frame;
            push_constants.render_width = render_extent.width;
            push_constants.render_height = render_extent.height;
            cmd.pushConstants( primary_rays_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof( push_constants ), &push_constants );

            descriptor_sets = { primary_rays_set[primary_rays_index], global_state_set, blit_set_c };

            cmd.bindPipeline( vk::PipelineBindPoint::eCompute, primary_rays_pipeline );
            cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, primary_rays_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
            cmd.dispatch( num_dispatch, 1, 1 );
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 0, nullptr );

            // Compute - Update Global State

            descriptor_sets = { primary_rays_set[primary_rays_index], global_state_set };

            cmd.bindPipeline( vk::PipelineBindPoint::eCompute, global_state_pipeline );
            cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, global_state_layout, 0, 1, &global_state_set, 0, nullptr );
            cmd.dispatch( 1, 1, 1 );
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 0, nullptr );

            // Compute - Extend

            descriptor_sets = { primary_rays_set[primary_rays_index], global_state_set, extend_set, blit_set_c };

            cmd.bindPipeline( vk::PipelineBindPoint::eCompute, extend_pipeline );
            cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, extend_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
            cmd.dispatch( num_dispatch, 1, 1 );
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 0, nullptr );

            // Compute - Shade

            descriptor_sets = { primary_rays_set[primary_rays_index], primary_rays_set[( primary_rays_index + 1 ) % 2], shadow_rays_set, blit_set_c };

            cmd.bindPipeline( vk::PipelineBindPoint::eCompute, shade_pipeline );
            cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, shade_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
            cmd.dispatch( num_dispatch, 1, 1 );
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 0, nullptr );

            // Compute - Connect

            descriptor_sets = { shadow_rays_set, extend_set, blit_set_c };

            cmd.bindPipeline( vk::PipelineBindPoint::eCompute, connect_pipeline );
            cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, connect_layout, 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr );
            cmd.dispatch( num_dispatch, 1, 1 );
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 0, nullptr );

            blit( cmd );

            frame++;
            primary_rays_index = ( primary_rays_index + 1 ) % 2;
        }

    }
}