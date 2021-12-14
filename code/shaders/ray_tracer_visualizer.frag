#version 450
#extension GL_GOOGLE_include_directive : require
#include "common_brickmap.glsl"
#include "common_raytrace.glsl"

layout (push_constant) uniform push_constants
{
    // Ray Gen Properties
    uint frame;
    uint render_width;
    uint render_height;
    uint pad_0;

    // Camera Properties
    vec4 camera_direction;
    vec4 camera_up;
    vec4 camera_right;
    vec4 camera_position;
    float focal_distance;
    float lens_radius;
    vec2 sun_position;
};

layout (set = 0, binding = 0) buffer blit_buffer
{
    vec4 colors[];
};

layout (location = 0) out vec4 out_color;

void main()
{
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
    uint pixel_index = y * render_width + x;

    out_color = colors[pixel_index];
}