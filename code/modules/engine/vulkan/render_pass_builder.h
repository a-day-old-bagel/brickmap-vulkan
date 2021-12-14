#pragma once

namespace rebel_road
{
    namespace vulkan
    {
        constexpr uint32_t reasonable_upper_bound_on_attachments = 10;

        class device_context;

        class render_pass_builder
        {
        public:
            static render_pass_builder begin( device_context* device_ctx );

            render_pass_builder& add_color_attachment( vk::Format format, vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined, vk::ImageLayout final_layout = vk::ImageLayout::eShaderReadOnlyOptimal, vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eClear );
            render_pass_builder& add_depth_attachment( vk::Format format, vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined, vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eClear );
            render_pass_builder& add_default_subpass_dependency();
            render_pass_builder& add_subpass_dependency( uint32_t src, uint32_t dst, vk::PipelineStageFlags src_mask, vk::PipelineStageFlags dst_mask, vk::AccessFlags src_access, vk::AccessFlags dst_access );

            render_pass_builder& build();

            vk::RenderPass get_render_pass() const { return render_pass; } 

        private:
            device_context* device_ctx {};

            struct
            {
                std::vector<vk::AttachmentDescription> attachments;
                std::vector<vk::AttachmentReference> attachment_ref;
                vk::SubpassDescription subpass;
                std::vector<vk::SubpassDependency> dependencies;
            } props;

            vk::RenderPass render_pass;
        };
    }
}