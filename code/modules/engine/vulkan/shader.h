#pragma once

#include "device_context.h"

namespace rebel_road
{
    namespace vulkan
    {
        constexpr uint32_t max_descriptor_sets { 4 };   // On PC, some devices are limited to 4 sets.

        struct shader
        {
            vk::Pipeline pipeline {};
            vk::PipelineLayout layout {};
            std::string debug_name;

            bool operator==( const shader& other ) const
            {
                return pipeline == other.pipeline && layout == other.layout && debug_name == other.debug_name;
            }

            explicit operator bool() const
            {
                return ( pipeline && layout );
            }
        };

        struct shader_module
        {
            std::vector<uint32_t> code; // A block of shader code suitable for reflection and analysis.
            vk::ShaderModule module {}; // The vulkan handle to the loaded shader.
        };

        class shader_layout_builder
        {
        public:

            struct reflection_override
            {
                const char* name {};
                vk::DescriptorType overriden_type {};
                uint32_t array_max {};
                vk::DescriptorBindingFlags binding_flags {};
            };

            struct reflected_descriptor_set
            {
                uint32_t set_number {};
                vk::DescriptorSetLayoutCreateInfo create_info;
                std::vector<vk::DescriptorBindingFlags> binding_flags;
                std::vector<vk::DescriptorSetLayoutBinding> bindings;
                vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT flags_info {};
            };

            vk::PipelineLayout get_layout() const;
            std::vector<vk::PipelineShaderStageCreateInfo> get_stage_create_info() const;

            // To build a shader effect add one or more modules associated with a given stage (vertex, fragment, compute, etc...)
            // Then call build to complete the build process.
            static shader_layout_builder begin( device_context* device_ctx );
            shader_layout_builder& set_cache_key( const std::string& key ) { cache_key = key; return *this; }
            shader_layout_builder& add_stage( const shader_module* module, const vk::ShaderStageFlagBits stage );
            shader_layout_builder& build( const std::vector<reflection_override>* overrides = nullptr );

        private:
            // The build process (from shader code to a pipeline layout):
            std::pair<std::vector<reflected_descriptor_set>, std::vector<vk::PushConstantRange>> reflect_spirv( const std::vector<reflection_override>* overrides );
            void merge_and_create_descriptor_layouts( const std::vector<reflected_descriptor_set> reflected_sets );
            void create_pipeline_layout( const std::vector<vk::PushConstantRange> constant_ranges );

            struct shader_stage
            {
                const shader_module* module {};         // Shader module allocations are not managed by shader_effect.
                vk::ShaderStageFlagBits stage {};
            };

            std::vector<shader_stage> stages;           // The code to execute for each stage in this pipeline.
            vk::PipelineLayout layout {};               // Vulkan handle to the built shader pipeline. Only valid after build is called.
            std::string cache_key {};

            std::vector<vk::DescriptorSetLayout> set_layouts;
            device_context* device_ctx {};
        };

        std::pair<vk::Pipeline, vk::PipelineLayout> load_compute_shader( const std::string& shader_path, device_context* device_ctx );
    }
}