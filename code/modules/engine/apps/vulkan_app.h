#pragma once
#include "app.h"
#include "containers/deletion_queue.h"
#include "vulkan/device_context.h"
#include "vulkan/render_context.h"

namespace rebel_road
{
    namespace apps
    {

        class vulkan_app_settings
        {
        public:
            void load_from_file( const std::string& settings_file );

            std::string app_name;
            uint32_t default_width {};
            uint32_t default_height {};
            bool use_validation_layers {};
        };

        class vulkan_app : public app
        {
        public:
            ~vulkan_app() {}
            vulkan_app() : app() {}

            vulkan_app( const app& ) = delete;
            vulkan_app( app&& ) = delete;

        protected:
            void init( const std::string& settings_file );
            void init_logging();
            void create_window();
            void init_vulkan();
            virtual void shutdown();

            void toggle_mouse_visible();

            virtual vulkan::device_context::create_info get_device_create_info() = 0;

            vulkan_app_settings settings;

            util::deletion_queue deletion_queue;

            GLFWwindow* window { nullptr };

            std::unique_ptr<vulkan::device_context> device_ctx;
            std::shared_ptr<vulkan::render_context> render_ctx;

            vk::Extent2D window_extent;

            bool mouse_visible { false };
        };

    } // namespace apps

} // namespace rebel_road