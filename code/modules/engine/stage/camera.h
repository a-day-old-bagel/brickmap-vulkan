#pragma once

#include <glm/glm.hpp>

#define PI 3.1415926535897932384626433832795

//#include <numbers>
// gcc doesn't yet support std::numbers

struct GLFWwindow;

namespace rebel_road
{
    namespace stage
    {
        struct camera
        {
            glm::vec3 position = { 64, 64, 800 };
            glm::vec3 direction = { 1, 0, 0 };
            glm::vec3 up = { 0, 0, 1 };

            float focal_distance = 1;
            float lens_radius = 0.0f;
            bool enable_depth_of_field = false;

            double fov = 70;
            double aspect_ratio = 1920 / 1080;
            double fov_rad = ( PI / 180.0 ) * static_cast<double>( fov );
            double tan_height = 2.0 * tan( fov_rad * 0.5 );

            double horizontal_angle = 0.0;
            double vertical_angle = 0.0;

            void handle_input( GLFWwindow* window, double delta );
            void update();

            bool locked {};
            void set_locked( bool is_locked );
            void toggle_locked() { set_locked( !locked ); }
        };

    }
}