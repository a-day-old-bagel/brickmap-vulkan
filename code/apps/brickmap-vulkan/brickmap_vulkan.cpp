#include "brickmap_vulkan.h"
#include "vulkan/render_pass_builder.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include "main.h"

namespace rebel_road
{

    namespace apps
    {

        void key_callback( GLFWwindow* window, int key, int scancode, int action, int mods )
        {
            auto app = reinterpret_cast<brickmap_vulkan_app*>( glfwGetWindowUserPointer( window ) );
            app->on_keyboard( key, scancode, action, mods );
        }

        app_state brickmap_vulkan_app::on_cleanup()
        {
            device_ctx->device.waitIdle();

            imgui_ctx->shutdown();

            voxel_world.reset();
            ray_tracer->shutdown();
            render_ctx->shutdown();

            vulkan_app::shutdown();

            return apps::app::on_cleanup();
        }

        vulkan::device_context::create_info brickmap_vulkan_app::get_device_create_info()
        {
            vk::PhysicalDeviceFeatures required_features {};
            required_features.pipelineStatisticsQuery = true;
            required_features.shaderInt64 = true;                   // Required for 64 bit ints for octrees.

            vk::PhysicalDeviceVulkan12Features required_features_12 {};
            required_features_12.bufferDeviceAddress = true;        // Required for khr_buffer_device_address

            std::vector<const char*> required_extensions;
            required_extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
            required_extensions.push_back( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME );       // Provides ability to get pointers to GPU buffers.
            required_extensions.push_back( VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME );   // Required for shader debug printf.

            vulkan::device_context::create_info dci
            {
                .app_name = settings.app_name,
                .use_validation_layers = settings.use_validation_layers,
                .window = window,
                .required_features = required_features,
                .required_features_12 = required_features_12,
                .required_extensions = required_extensions,
                .alloc_flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
            };

            return dci;
        }

        app_state brickmap_vulkan_app::on_init()
        {
            // GLFW & Vulkan Context
            init( "cfg/settings.json" );
            glfwSetWindowUserPointer( window, this );
            glfwSetKeyCallback( window, key_callback );

            // Render Context
            render_ctx = device_ctx->create_render_context();

            // Render Pass
            auto render_pass_builder = vulkan::render_pass_builder::begin( device_ctx.get() )
                .add_color_attachment( device_ctx->get_swapchain_image_format(), vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR, vk::AttachmentLoadOp::eDontCare )
                .add_default_subpass_dependency()
                .build();
            render_pass = render_pass_builder.get_render_pass();

            // Swap Chain Framebuffers
            framebuffers = device_ctx->create_swap_chain_framebuffers( render_pass, window_extent );

            // Voxel World
            voxel_world = voxel::world::create( render_ctx.get() );
            voxel_world->generate();

            // Ray Tracer
            ray_tracer = voxel::ray_tracer::create( render_ctx.get(), window_extent, voxel_world );

            // Imgui
            imgui_ctx = imgui::imgui_context::create( device_ctx.get(), window, render_pass );

            previous_time = glfwGetTime();
            return apps::app::on_init();
        }

        app_state brickmap_vulkan_app::on_running()
        {
            float delta_time = glfwGetTime() - previous_time;
            previous_time = glfwGetTime();

            glfwPollEvents();
            camera.handle_input( window, delta_time );

            ray_tracer->sun_position = sun_position;
            ray_tracer->update_camera( camera );

            render( delta_time );
            voxel_world->process_load_queue();

            return apps::app::on_running();
        }

        void brickmap_vulkan_app::render( float delta_time )
        {
            imgui_ctx->begin_frame();
            render_imgui( delta_time );
            imgui_ctx->render();

            auto cmd = render_ctx->begin_frame();
            {
                ray_tracer->render( cmd );

                vk::RenderPassBeginInfo rp_info = vulkan::renderpass_begin_info( render_pass, window_extent, framebuffers[render_ctx->get_swapchain_image_index()] );
                cmd.beginRenderPass( &rp_info, vk::SubpassContents::eInline );
                {
                    imgui_ctx->draw( cmd );
                }
                cmd.endRenderPass();
            }
            render_ctx->end_frame();
            render_ctx->submit_and_present();
        }

        void brickmap_vulkan_app::on_keyboard( int key, int scancode, int action, int mods )
        {
            if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
            {
                toggle_mouse_visible();
                camera.toggle_locked();
            }
        }

        void brickmap_vulkan_app::render_imgui( float delta_time )
        {
            static std::vector<float> frame_times;
            frame_times.push_back( delta_time );
            if ( frame_times.size() > 200 )
            {
                frame_times.erase( frame_times.begin() );
            }

            static std::vector<float> brick_loads;
            brick_loads.push_back( voxel_world->get_brick_load_count() );
            if ( brick_loads.size() > 200 )
            {
                brick_loads.erase( brick_loads.begin() );
            }

            ImGui::Begin( "Brick Map" );
            {
                ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
                ImGui::PlotHistogram( "", frame_times.data(), frame_times.size(), 0, "Frametimes", 0, FLT_MAX, { 400, 100 } );
                ImGui::PlotHistogram( "", brick_loads.data(), brick_loads.size(), 0, "Brick Loads", 0, FLT_MAX, { 400, 100 } );
                ImGui::Text( "X: %f, Y: %f, Z: %f", camera.position.x, camera.position.y, camera.position.z );
                ImGui::Text( "Hor: %f, Vert: %f", camera.horizontal_angle, camera.vertical_angle );
                ImGui::Text( "Sun X: %f Y: %f", sun_position.x, sun_position.y );
                ImGui::SliderFloat( "FocalDistance", &camera.focalDistance, 0.1f, 100.f );
                //ImGui::SliderFloat("LensRadius", &camera.lensRadius, 0.01f, 80, "%.3f", 1.3f);
                //TODO(Dan): Backspace doesn't trigger. Manually need to call ImGui backend?
                ImGui::InputFloat( "Lens Rad", &camera.lensRadius );

                if ( ImGui::Button( "Quit" ) )
                {
                    request_quit();
                }
            }
            ImGui::End();

        }

    } // namespace apps

} // namespace rebel_road

application( brickmap_vulkan_app );
