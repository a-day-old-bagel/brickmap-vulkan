#pragma once
#include "app.h"
#include "containers/deletion_queue.h"
#include "vulkan/device_context.h"
#include "vulkan/render_context.h"

namespace rebel_road
{
    namespace apps
    {

        class vulkan_app : public app
        {
        public:
            ~vulkan_app() {}
            vulkan_app() : app() {}

            vulkan_app( const app& ) = delete;
            vulkan_app( app&& ) = delete;

        protected:
            void init( std::string in_app_name, uint32_t width, uint32_t height, bool in_use_validation_layers );
            void init_logging();
            void create_window();
            void init_vulkan();
            virtual void shutdown();

            void toggle_mouse_visible();

            virtual vulkan::device_context::create_info get_device_create_info() = 0;

            util::deletion_queue deletion_queue;

            GLFWwindow* window { nullptr };

            std::unique_ptr<vulkan::device_context> device_ctx;
            std::shared_ptr<vulkan::render_context> render_ctx;

            vk::Extent2D window_extent;
            std::string app_name;
            bool use_validation_layers{ false };

            bool mouse_visible { false };
        };

    } // namespace apps

} // namespace rebel_road