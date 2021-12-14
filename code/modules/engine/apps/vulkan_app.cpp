#include "vulkan_app.h"
#include "log/critical_sink.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <nlohmann/json.hpp>

using namespace nlohmann;

namespace rebel_road
{
    namespace apps
    {

        static void glfw_error_callback( int error, const char* desc )
        {

        }

        void vulkan_app_settings::load_from_file( const std::string& settings_file )
        {
            std::ifstream ifs( settings_file );
            assert( ifs.is_open() );
            json settings = json::parse( ifs );

            app_name = settings["settings"]["app_name"];
            default_width = settings["settings"]["width"];
            default_height = settings["settings"]["height"];
            use_validation_layers = settings["settings"]["use_validation_layers"];
        }

        void vulkan_app::shutdown()
        {
            device_ctx->device.waitIdle();
            deletion_queue.flush();
        }

        void vulkan_app::init( const std::string& settings_file )
        {
            settings.load_from_file( settings_file );
            window_extent = vk::Extent2D( settings.default_width, settings.default_height );

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
            glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

            window = glfwCreateWindow( settings.default_width, settings.default_height, settings.app_name.c_str(), nullptr, nullptr );
            glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
            if ( glfwRawMouseMotionSupported() )
                glfwSetInputMode( window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE );
            else
                spdlog::critical( "Platform does not support raw mouse input." );

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