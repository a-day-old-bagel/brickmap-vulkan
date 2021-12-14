#include "initializers.h"

namespace rebel_road {

	namespace vulkan {

		vk::CommandPoolCreateInfo command_pool_create_info( uint32_t queue_family_index, vk::CommandPoolCreateFlags flags )
		{
			vk::CommandPoolCreateInfo info {};
			info.queueFamilyIndex = queue_family_index;
			info.flags = flags;
			return info;
		}

		vk::CommandBufferAllocateInfo command_buffer_allocate_info( vk::CommandPool pool, uint32_t count, vk::CommandBufferLevel level )
		{
			vk::CommandBufferAllocateInfo info {};
			info.commandPool = pool;
			info.commandBufferCount = count;
			info.level = level;
			return info;
		}

		vk::CommandBufferBeginInfo command_buffer_begin_info( vk::CommandBufferUsageFlags flags )
		{
			vk::CommandBufferBeginInfo info {};
			info.pInheritanceInfo = nullptr;
			info.flags = flags;
			return info;
		}

		vk::FramebufferCreateInfo framebuffer_create_info( vk::RenderPass render_pass, vk::Extent2D extent )
		{
			vk::FramebufferCreateInfo info {};
			info.renderPass = render_pass;
			info.attachmentCount = 1;
			info.width = extent.width;
			info.height = extent.height;
			info.layers = 1;
			return info;
		}

		vk::FenceCreateInfo fence_create_info( vk::FenceCreateFlags flags )
		{
			vk::FenceCreateInfo info {};
			info.flags = flags;
			return info;
		}

		vk::SemaphoreCreateInfo semaphore_create_info( vk::SemaphoreCreateFlags flags )
		{
			vk::SemaphoreCreateInfo info {};
			info.flags = flags;
			return info;
		}

		vk::SubmitInfo submit_info( vk::CommandBuffer* cmd )
		{
			vk::SubmitInfo info {};
			info.waitSemaphoreCount = 0;
			info.pWaitSemaphores = nullptr;
			info.pWaitDstStageMask = nullptr;
			info.commandBufferCount = 1;
			info.pCommandBuffers = cmd;
			info.signalSemaphoreCount = 0;
			info.pSignalSemaphores = nullptr;

			return info;
		}

		vk::PresentInfoKHR present_info()
		{
			vk::PresentInfoKHR info {};
			info.swapchainCount = 0;
			info.pSwapchains = nullptr;
			info.pWaitSemaphores = nullptr;
			info.waitSemaphoreCount = 0;
			info.pImageIndices = nullptr;
			return info;
		}

		vk::RenderPassBeginInfo renderpass_begin_info( vk::RenderPass render_pass, vk::Extent2D window_extent, vk::Framebuffer framebuffer )
		{
			vk::RenderPassBeginInfo info {};
			info.renderPass = render_pass;
			info.renderArea.offset.x = 0;
			info.renderArea.offset.y = 0;
			info.renderArea.extent = window_extent;
			info.clearValueCount = 0;
			info.pClearValues = nullptr;
			info.framebuffer = framebuffer;
			return info;
		}

		vk::PipelineShaderStageCreateInfo pipeline_shader_stage_create_info( vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module )
		{
			vk::PipelineShaderStageCreateInfo info {};

			info.stage = stage;
			info.module = shader_module;
			info.pName = "main";
			return info;
		}

		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info()
		{
			vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
			vertex_input_info.vertexBindingDescriptionCount = 0;
			vertex_input_info.vertexAttributeDescriptionCount = 0;
			return vertex_input_info;
		}

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info( vk::PrimitiveTopology topology )
		{
			vk::PipelineInputAssemblyStateCreateInfo input_assembly {};
			input_assembly.topology = topology;
			input_assembly.primitiveRestartEnable = VK_FALSE;
			return input_assembly;
		}

		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info( vk::PolygonMode polygon_mode, vk::CullModeFlagBits cull_mode, vk::Bool32 depth_bias_enable, vk::FrontFace front_face )
		{
			vk::PipelineRasterizationStateCreateInfo rasterizer {};
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = polygon_mode;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = cull_mode;
			rasterizer.frontFace = front_face;
			rasterizer.depthBiasEnable = depth_bias_enable;
			rasterizer.depthBiasConstantFactor = 0.0f;
			rasterizer.depthBiasClamp = 0.0f;
			rasterizer.depthBiasSlopeFactor = 0.0f;
			return rasterizer;
		}

		vk::PipelineMultisampleStateCreateInfo multisampling_state_create_info( vk::SampleCountFlagBits sample_count )
		{
			vk::PipelineMultisampleStateCreateInfo multisampling {};
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = sample_count;
			multisampling.minSampleShading = 1.0f;
			multisampling.pSampleMask = nullptr;
			multisampling.alphaToCoverageEnable = VK_FALSE;
			multisampling.alphaToOneEnable = VK_FALSE;
			return multisampling;
		}

		vk::PipelineColorBlendAttachmentState color_blend_attachment_state()
		{
			vk::PipelineColorBlendAttachmentState color_blend_attachment {};
			color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			color_blend_attachment.blendEnable = VK_FALSE;
			/*
			color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
			color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
			color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
			color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
			*/
			return color_blend_attachment;
		}

		vk::PipelineLayoutCreateInfo pipeline_layout_create_info()
		{
			vk::PipelineLayoutCreateInfo pipeline_layout {};
			pipeline_layout.flags = {};
			pipeline_layout.setLayoutCount = 0;
			pipeline_layout.pSetLayouts = nullptr;
			pipeline_layout.pushConstantRangeCount = 0;
			pipeline_layout.pPushConstantRanges = nullptr;
			return pipeline_layout;
		}

		vk::ImageCreateInfo image_create_info( vk::Format format, vk::ImageUsageFlags usage_flags, vk::Extent3D extent, int mip_levels )
		{
			vk::ImageCreateInfo info {};
			info.imageType = vk::ImageType::e2D;
			info.format = format;
			info.extent = extent;
			info.mipLevels = mip_levels;
			info.arrayLayers = 1;
			info.samples = vk::SampleCountFlagBits::e1;
			info.tiling = vk::ImageTiling::eOptimal;
			info.usage = usage_flags;
			return info;
		}

		vk::ImageViewCreateInfo image_view_create_info( vk::Format format, vk::Image image, vk::ImageAspectFlags aspect_flags, int mip_levels, vk::ImageViewType image_view_type )
		{
			//build a image-view for the depth image to use for rendering
			vk::ImageViewCreateInfo info {};
			info.viewType = image_view_type;
			info.image = image;
			info.format = format;
			info.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
			info.subresourceRange.aspectMask = aspect_flags;
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.levelCount = mip_levels;
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.layerCount = ( image_view_type == vk::ImageViewType::eCube ) ? 6 : 1;
			return info;
		}

		vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info( bool depth_test, bool depth_write, vk::CompareOp compare_op )
		{
			vk::PipelineDepthStencilStateCreateInfo info {};
			info.depthTestEnable = depth_test ? VK_TRUE : VK_FALSE;
			info.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
			info.depthCompareOp = depth_test ? compare_op : vk::CompareOp::eAlways;
			info.depthBoundsTestEnable = VK_FALSE;
			info.minDepthBounds = 0.0f; // Optional
			info.maxDepthBounds = 1.0f; // Optional
			info.stencilTestEnable = VK_FALSE;
			return info;
		}

		vk::DescriptorSetLayoutBinding descriptor_set_layout_binding( vk::DescriptorType type, vk::ShaderStageFlags stage_flags, uint32_t binding, uint32_t count )
		{
			vk::DescriptorSetLayoutBinding setbind {};
			setbind.binding = binding;
			setbind.descriptorCount = count;
			setbind.descriptorType = type;
			setbind.pImmutableSamplers = nullptr;
			setbind.stageFlags = stage_flags;
			return setbind;
		}

		vk::WriteDescriptorSet write_descriptor_buffer( vk::DescriptorType type, vk::DescriptorSet dst_set, vk::DescriptorBufferInfo* buffer_info, uint32_t binding )
		{
			vk::WriteDescriptorSet write {};
			write.dstBinding = binding;
			write.dstSet = dst_set;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pBufferInfo = buffer_info;
			return write;
		}

		vk::WriteDescriptorSet write_descriptor_image( vk::DescriptorType type, vk::DescriptorSet dst_set, vk::DescriptorImageInfo* image_info, uint32_t binding )
		{
			vk::WriteDescriptorSet write {};
			write.dstBinding = binding;
			write.dstSet = dst_set;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pImageInfo = image_info;
			return write;
		}

		vk::SamplerCreateInfo sampler_create_info( vk::Filter min_filter, vk::Filter mag_filter, vk::SamplerAddressMode sampler_address_mode_u, vk::SamplerAddressMode sampler_address_mode_v, vk::SamplerAddressMode sampler_address_mode_w, int mip_levels, float max_anisotropy, bool anisotropy_enable )
		{
			vk::SamplerCreateInfo info {};
			info.magFilter = mag_filter;
			info.minFilter = min_filter;
			info.addressModeU = sampler_address_mode_u;
			info.addressModeV = sampler_address_mode_v;
			info.addressModeW = sampler_address_mode_w;
			info.mipmapMode = vk::SamplerMipmapMode::eLinear;
			info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
			info.compareOp = vk::CompareOp::eNever;
			info.maxLod = (float) mip_levels;
			info.maxAnisotropy = max_anisotropy;
			info.anisotropyEnable = anisotropy_enable;
			return info;
		}

        vk::SamplerCreateInfo sampler_create_info( vk::Filter filter, vk::SamplerAddressMode sampler_address_mode )
		{
			return sampler_create_info( filter, filter, sampler_address_mode, sampler_address_mode, sampler_address_mode );
		}

		vk::BufferMemoryBarrier buffer_barrier( vk::Buffer buffer, uint32_t queue )
		{
			vk::BufferMemoryBarrier barrier {};
			barrier.buffer = buffer;
			barrier.size = VK_WHOLE_SIZE;
			//barrier2.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			//barrier2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.srcQueueFamilyIndex = queue;
			barrier.dstQueueFamilyIndex = queue;
			return barrier;
		}

		vk::ImageMemoryBarrier image_barrier( vk::Image image, vk::AccessFlags src_access_mask, vk::AccessFlags dst_access_mask, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::ImageAspectFlags aspect_mask )
		{
			vk::ImageMemoryBarrier barrier {};
			barrier.srcAccessMask = src_access_mask;
			barrier.dstAccessMask = dst_access_mask;
			barrier.oldLayout = old_layout;
			barrier.newLayout = new_layout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = aspect_mask;
			barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			return barrier;
		}

		vk::AttachmentDescription attachment_description( vk::Format format, vk::ImageLayout initial_layout, vk::ImageLayout final_layout, vk::AttachmentLoadOp load_op )
		{
			vk::AttachmentDescription attachment_description {};
			attachment_description.flags = {};
			attachment_description.format = format;
			attachment_description.samples = vk::SampleCountFlagBits::e1;
			attachment_description.loadOp = load_op;
			attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
			attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			attachment_description.initialLayout = initial_layout;
			attachment_description.finalLayout = final_layout;
			return attachment_description;
		}

		vk::AttachmentReference color_attachment_reference()
		{
			vk::AttachmentReference color_attachment_ref {};
			color_attachment_ref.attachment = 0;
			color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
			return color_attachment_ref;
		}

		vk::ImageBlit image_blit( int width, int height, int index, vk::ImageAspectFlags aspect_flags )
		{
			vk::ImageBlit image_blit {};
			image_blit.srcSubresource.aspectMask = aspect_flags;
			image_blit.srcSubresource.layerCount = 1;
			image_blit.srcSubresource.mipLevel = index - 1;
			image_blit.srcOffsets[1].x = int32_t( width >> ( index - 1 ) );
			image_blit.srcOffsets[1].y = int32_t( height >> ( index - 1 ) );
			image_blit.srcOffsets[1].z = 1;
			image_blit.dstSubresource.aspectMask = aspect_flags;
			image_blit.dstSubresource.layerCount = 1;
			image_blit.dstSubresource.mipLevel = index;
			image_blit.dstOffsets[1].x = int32_t( width >> index );
			image_blit.dstOffsets[1].y = int32_t( height >> index );
			image_blit.dstOffsets[1].z = 1;
			return image_blit;
		}

		vk::ImageSubresourceRange image_subresource_range( int index, int level_count, int layer_count, vk::ImageAspectFlags aspect_flags )
		{
			vk::ImageSubresourceRange sub_range {};
			sub_range.aspectMask = aspect_flags;
			sub_range.baseMipLevel = index;
			sub_range.levelCount = level_count;
			sub_range.layerCount = layer_count;
			return sub_range;
		}

		vk::BufferImageCopy buffer_image_copy( vk::Extent3D image_extent, vk::ImageAspectFlags aspect_flags, uint32_t mip_level, uint32_t array_layer, uint32_t layer_count, size_t buffer_offset )
		{
			vk::BufferImageCopy copy_region {};
			copy_region.bufferOffset = buffer_offset;
			copy_region.bufferRowLength = 0;
			copy_region.bufferImageHeight = 0;
			copy_region.imageSubresource.aspectMask = aspect_flags;
			copy_region.imageSubresource.mipLevel = mip_level;
			copy_region.imageSubresource.baseArrayLayer = array_layer;
			copy_region.imageSubresource.layerCount = layer_count;
			copy_region.imageExtent = image_extent;
			return copy_region;
		}

		vk::ImageCopy image_copy( vk::Extent3D image_extent, uint32_t mip_level, uint32_t base_layer )
		{
			vk::ImageCopy copy_region {};
			copy_region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copy_region.srcSubresource.baseArrayLayer = 0;
			copy_region.srcSubresource.mipLevel = 0;
			copy_region.srcSubresource.layerCount = 1;
			copy_region.srcOffset = 0;
			copy_region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copy_region.dstSubresource.baseArrayLayer = base_layer;
			copy_region.dstSubresource.mipLevel = mip_level;
			copy_region.dstSubresource.layerCount = 1;
			copy_region.dstOffset = 0;
			copy_region.extent = image_extent;
			return copy_region;
		}

		uint32_t aligned_size( uint32_t value, uint32_t alignment )
		{
			return ( value + alignment - 1 ) & ~( alignment - 1 );
		}

	} // namespace vulkan

} // namespace rebel_road