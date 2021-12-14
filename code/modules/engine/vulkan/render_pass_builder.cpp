#include "render_pass_builder.h"
#include "device_context.h"

#include <imgui.h>

namespace rebel_road
{
    namespace vulkan
    {

        render_pass_builder render_pass_builder::begin( device_context* device_ctx )
        {
            render_pass_builder builder;
            builder.device_ctx = device_ctx;
            builder.props.attachment_ref.reserve( reasonable_upper_bound_on_attachments );
            return builder;
        }

        render_pass_builder& render_pass_builder::add_color_attachment( vk::Format format, vk::ImageLayout initial_layout, vk::ImageLayout final_layout, vk::AttachmentLoadOp load_op )
        {
            vk::AttachmentReference attachment_ref {};
            attachment_ref.attachment = props.attachments.size();
            attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
            props.attachment_ref.push_back( attachment_ref );

            auto attachment_desc = attachment_description( format, initial_layout, final_layout, load_op );
            props.attachments.push_back( attachment_desc );

            props.subpass.colorAttachmentCount = 1;
            props.subpass.pColorAttachments = &props.attachment_ref.back();

            return *this;
        }

        render_pass_builder& render_pass_builder::add_depth_attachment( vk::Format format, vk::ImageLayout initial_layout, vk::AttachmentLoadOp load_op )
        {
            vk::AttachmentReference attachment_ref {};
            attachment_ref.attachment = props.attachments.size();
            attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            props.attachment_ref.push_back( attachment_ref );

            // fixme: clean up order of arguments
            auto attachment_desc = attachment_description( format, initial_layout, vk::ImageLayout::eDepthStencilAttachmentOptimal, load_op );
            props.attachments.push_back( attachment_desc );

            props.subpass.pDepthStencilAttachment = &props.attachment_ref.back();

            return *this;
        }

        render_pass_builder& render_pass_builder::add_default_subpass_dependency()
        {
            vk::SubpassDependency dependency {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
            dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
            dependency.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            props.dependencies.push_back( dependency );

            return *this;
        }

        render_pass_builder& render_pass_builder::add_subpass_dependency( uint32_t src, uint32_t dst, vk::PipelineStageFlags src_mask, vk::PipelineStageFlags dst_mask, vk::AccessFlags src_access, vk::AccessFlags dst_access )
        {
            vk::SubpassDependency dependency {};
            dependency.srcSubpass = src;
            dependency.dstSubpass = dst;
            dependency.srcStageMask = src_mask;
            dependency.dstStageMask = dst_mask;
            dependency.srcAccessMask = src_access;
            dependency.dstAccessMask = dst_access;
            props.dependencies.push_back( dependency );

            return *this;
        }

        render_pass_builder& render_pass_builder::build()
        {
            props.subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

            vk::RenderPassCreateInfo render_pass_info {};
            render_pass_info.attachmentCount = props.attachments.size();
            render_pass_info.pAttachments = props.attachments.data();
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &props.subpass;
            render_pass_info.dependencyCount = props.dependencies.size();
            render_pass_info.pDependencies = props.dependencies.data();

            render_pass = device_ctx->create_render_pass( render_pass_info );

            return *this;
        }

    }

}