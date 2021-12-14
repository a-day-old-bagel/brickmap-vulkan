#pragma once

#include "shader.h"
#include "render_context.h"

namespace rebel_road
{
    namespace vulkan
    {
        struct vertex_input_description
        {
            std::vector<vk::VertexInputBindingDescription> bindings;
            std::vector<vk::VertexInputAttributeDescription> attributes;

            vk::PipelineVertexInputStateCreateFlags flags {};
        };

        class graphics_pipeline_builder
        {
        public:
            static graphics_pipeline_builder begin( device_context* device_ctx );

            graphics_pipeline_builder& set_cache_key( const std::string& key ) { cache_key = key; return *this; }
            graphics_pipeline_builder& disable_color_blend();
            graphics_pipeline_builder& use_default_color_blend();
            graphics_pipeline_builder& use_translucent_color_blend();
            graphics_pipeline_builder& disable_multisampling();
    		graphics_pipeline_builder& use_default_multisampling();
            graphics_pipeline_builder& set_multisampling( vk::SampleCountFlagBits sample_count );
            graphics_pipeline_builder& disable_depth_stencil();
            graphics_pipeline_builder& use_default_depth_stencil();
            graphics_pipeline_builder& use_inverse_depth_stencil();
            graphics_pipeline_builder& set_rasterization( vk::PolygonMode polygon_mode, vk::CullModeFlagBits cull_mode_flags, bool depth_bias_enable = false, vk::FrontFace front_face = vk::FrontFace::eClockwise );
            graphics_pipeline_builder& enable_conservative_raster();
            graphics_pipeline_builder& set_input_assembly( vk::PrimitiveTopology primitive_topology );
            graphics_pipeline_builder& set_shaders( const shader_layout_builder& shader_builder );
            graphics_pipeline_builder& add_dynamic_state( vk::DynamicState dynamic_state );
            graphics_pipeline_builder& set_scissor( vk::Rect2D scissor );
            graphics_pipeline_builder& set_viewport( vk::Viewport viewport );
            graphics_pipeline_builder& set_vertex_description( const vertex_input_description& description );
            
            graphics_pipeline_builder& build( const vk::RenderPass& pass );

            vk::Pipeline get_pipeline() const { return pipeline; } 

        private:
            device_context* device_ctx {};
            std::string cache_key {};

            vk::Pipeline pipeline {};

            struct
            {
                vk::PipelineColorBlendAttachmentState color_blend_attachment {};
                vk::PipelineMultisampleStateCreateInfo multisampling {};
                vk::PipelineDepthStencilStateCreateInfo depth_stencil {};
                vk::PipelineRasterizationStateCreateInfo rasterizer {};
                vk::PipelineInputAssemblyStateCreateInfo input_assembly {};
                vk::PipelineRasterizationConservativeStateCreateInfoEXT conservative_info{};
                vk::Viewport viewport {};
                vk::Rect2D scissor {};
                vertex_input_description vertex_description {};
            } props;

            std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_info;
            vk::PipelineLayout shader_pipeline_layout {};

			std::vector<vk::DynamicState> dynamic_states;
        };
        
    }
}