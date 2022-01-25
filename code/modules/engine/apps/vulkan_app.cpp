#include "vulkan_app.h"
#include "log/critical_sink.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace rebel_road
{
    namespace apps
    {

        static void glfw_error_callback( int error, const char* desc )
        {

        }

        void glfw_window_size_callback( GLFWwindow* window, int width, int height )
        {
            auto app = static_cast<vulkan_app*>( glfwGetWindowUserPointer( window ) );
            app->internal_resize_window( width, height );
        }

        void vulkan_app::shutdown()
        {
            device_ctx->device.waitIdle();
            deletion_queue.flush();
        }

        void vulkan_app::init( std::string in_app_name, uint32_t width, uint32_t height, bool in_use_validation_layers )
        {
            app_name = in_app_name;
            use_validation_layers = in_use_validation_layers;

            window_extent = vk::Extent2D( width, height );

            init_logging();
            create_window();
            init_vulkan();
        }

        void vulkan_app::init_logging()
        {
            spdlog::set_default_logger( spdlog::stdout_color_mt( "console" ) );
            auto critical_sink = std::make_shared<critical_sink_mt>();
            spdlog::get( "console" )->sinks().push_back( critical_sink );
        }

        void vulkan_app::create_window()
        {
            glfwInit();
            glfwSetErrorCallback( glfw_error_callback );
            glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );     // Tell glfw to not create an opengl context, since we are using vulkan.
            glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

            window = glfwCreateWindow( window_extent.width, window_extent.height, app_name.c_str(), nullptr, nullptr );
            glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
            if ( glfwRawMouseMotionSupported() )
                glfwSetInputMode( window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE );
            else
                spdlog::critical( "Platform does not support raw mouse input." );
            glfwSetWindowSizeCallback( window, glfw_window_size_callback );
            glfwSetWindowUserPointer( window, this );

            deletion_queue.push_function( [=,this] ()
                {
                    glfwDestroyWindow( window );
                    glfwTerminate();
                } );
        };

        void vulkan_app::init_vulkan()
        {
            auto dci = get_device_create_info();
            device_ctx = vulkan::device_context::create( dci );
            deletion_queue.push_function( [=,this] () { device_ctx->shutdown(); } );

            render_extent = device_ctx->find_render_extent( window_extent.width, window_extent.height );
        }

        void vulkan_app::internal_resize_window( int width, int height )
        {           
            window_extent = vk::Extent2D( width, height );
            device_ctx->resize( width, height );
            render_extent = device_ctx->find_render_extent( window_extent.width, window_extent.height );

            resize( width, height );
        }

        void vulkan_app::toggle_mouse_visible()
        {
            mouse_visible = !mouse_visible;
            glfwSetInputMode( window, GLFW_RAW_MOUSE_MOTION, !mouse_visible );
            glfwSetInputMode( window, GLFW_CURSOR, mouse_visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED );
            //input_signals::mouse_visible( mouse_visible );
        }

    } // namespace apps

} // namespace rebel_road