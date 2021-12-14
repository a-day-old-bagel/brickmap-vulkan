#pragma once

#include "containers/deletion_queue.h"
#include "vulkan/device_context.h"

struct GLFWwindow;

namespace rebel_road
{

    namespace imgui
    {

        class imgui_context
        {
        public:
            static std::unique_ptr<imgui_context> create( vulkan::device_context* in_context, GLFWwindow* in_window, vk::RenderPass in_render_pass );

            ~imgui_context();
            imgui_context() = delete;
            imgui_context( vulkan::device_context* in_context, GLFWwindow* in_window, vk::RenderPass in_render_pass );

            void init_imgui();
            void shutdown();

            void begin_frame();
            void render();
            void draw( vk::CommandBuffer cmd );

        private:
            vulkan::device_context* device_ctx {};
            util::deletion_queue deletion_queue;

            GLFWwindow* window {};
            vk::RenderPass render_pass;
        };

    } // namespace imgui

} // namespace rebel_road