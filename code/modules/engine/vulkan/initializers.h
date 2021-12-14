#pragma once

#include <vulkan/vulkan.hpp>

namespace rebel_road
{
    namespace vulkan
    {
        vk::CommandPoolCreateInfo command_pool_create_info( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlags() );
        vk::CommandBufferAllocateInfo command_buffer_allocate_info( vk::CommandPool pool, uint32_t count = 1, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary );
        vk::CommandBufferBeginInfo command_buffer_begin_info( vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlags() );

        vk::FramebufferCreateInfo framebuffer_create_info( vk::RenderPass render_pass, vk::Extent2D extent );
        vk::FenceCreateInfo fence_create_info( vk::FenceCreateFlags flags = vk::FenceCreateFlags() );
        vk::SemaphoreCreateInfo semaphore_create_info( vk::SemaphoreCreateFlags flags = vk::SemaphoreCreateFlags() );
        vk::SubmitInfo submit_info( vk::CommandBuffer* cmd );
        vk::PresentInfoKHR present_info();
        vk::RenderPassBeginInfo renderpass_begin_info( vk::RenderPass render_pass, vk::Extent2D window_extent, vk::Framebuffer framebuffer );

        vk::PipelineShaderStageCreateInfo pipeline_shader_stage_create_info( vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module );
        vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info();
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info( vk::PrimitiveTopology topology );
        vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info( vk::PolygonMode polygon_mode, vk::CullModeFlagBits cull_mode = vk::CullModeFlagBits::eBack, vk::Bool32 depth_bias_enable = false, vk::FrontFace front_face = vk::FrontFace::eClockwise );
        vk::PipelineMultisampleStateCreateInfo multisampling_state_create_info( vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1 );
        vk::PipelineColorBlendAttachmentState color_blend_attachment_state();
        vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info( bool depth_test, bool depth_write, vk::CompareOp compare_op );
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info();

        vk::SamplerCreateInfo sampler_create_info( vk::Filter min_filter = vk::Filter::eLinear, vk::Filter mag_filter = vk::Filter::eLinear, vk::SamplerAddressMode sampler_address_mode_u = vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode sampler_address_mode_v = vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode sampler_address_mode_w = vk::SamplerAddressMode::eRepeat, int mip_levels = {}, float max_anisotropy = 8.f, bool anisotropy_enable=true );
        vk::SamplerCreateInfo sampler_create_info( vk::Filter filter, vk::SamplerAddressMode sampler_address_mode );

        vk::ImageCreateInfo image_create_info( vk::Format format, vk::ImageUsageFlags usage_flags, vk::Extent3D extent, int mip_levels = 1 );
        vk::ImageViewCreateInfo image_view_create_info( vk::Format format, vk::Image image, vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor, int mip_levels = 1, vk::ImageViewType image_view_type = vk::ImageViewType::e2D );
        vk::ImageBlit image_blit( int width, int height, int index, vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor );
        vk::ImageSubresourceRange image_subresource_range( int index, int level_count, int layer_count = 1, vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor );
        vk::ImageMemoryBarrier image_barrier( vk::Image image, vk::AccessFlags src_access_mask, vk::AccessFlags dst_access_mask, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::ImageAspectFlags aspect_mask );
        vk::BufferImageCopy buffer_image_copy( vk::Extent3D image_extent, vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor, uint32_t mip_level=0, uint32_t array_layer=0, uint32_t layer_count=1, size_t buffer_offset=0 );
        vk::ImageCopy image_copy( vk::Extent3D image_extent, uint32_t mip_level, uint32_t base_layer );

        vk::DescriptorSetLayoutBinding descriptor_set_layout_binding( vk::DescriptorType type, vk::ShaderStageFlags stage_flags, uint32_t binding, uint32_t count = 1 );
        vk::WriteDescriptorSet write_descriptor_buffer( vk::DescriptorType type, vk::DescriptorSet dst_set, vk::DescriptorBufferInfo* buffer_info, uint32_t binding );
        vk::WriteDescriptorSet write_descriptor_image( vk::DescriptorType type, vk::DescriptorSet dst_set, vk::DescriptorImageInfo* image_info, uint32_t binding );
        vk::BufferMemoryBarrier buffer_barrier( vk::Buffer buffer, uint32_t queue );

        vk::AttachmentDescription attachment_description( vk::Format format, vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined, vk::ImageLayout final_layout = vk::ImageLayout::eShaderReadOnlyOptimal, vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eClear );
        vk::AttachmentReference color_attachment_reference();

        uint32_t aligned_size( uint32_t value, uint32_t alignment );
    }
}