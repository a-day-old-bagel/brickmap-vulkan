#pragma once

#include "apps/vulkan_app.h"
#include "stage/camera.h"
#include "imgui/imgui_context.h"
#include "voxel/ray_tracer.h"
#include "voxel/world.h"

namespace rebel_road
{

    namespace apps
    {

        class brickmap_vulkan_app : public vulkan_app
        {
        public:
            ~brickmap_vulkan_app() {}
            brickmap_vulkan_app() : vulkan_app() {}

            brickmap_vulkan_app( const brickmap_vulkan_app& ) = delete;
            brickmap_vulkan_app( brickmap_vulkan_app&& ) = delete;

            void main();
            void on_keyboard( int key, int scancode, int action, int mods );

        private:
            void update_input();
            virtual vulkan::device_context::create_info get_device_create_info();

            void render( float delta_time );
            void render_imgui( float delta_time );

            virtual app_state on_init();
            virtual app_state on_running();
            virtual app_state on_cleanup();

            virtual void resize( int width, int height );

            void generate_world();

            stage::camera camera;
            glm::vec2 sun_position { 0.005, 0.1 };
            bool enable_shadows { true };
            int render_mode {};

            vk::RenderPass render_pass;
            std::vector<vk::Framebuffer> framebuffers;

            std::shared_ptr<voxel::ray_tracer> ray_tracer;
            std::shared_ptr<voxel::world> voxel_world;

            std::unique_ptr<imgui::imgui_context> imgui_ctx;

            float previous_time { 0.f };
        };
    }

}