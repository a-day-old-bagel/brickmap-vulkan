#include "pipeline_builder.h"

namespace rebel_road
{

	namespace vulkan
	{

		graphics_pipeline_builder graphics_pipeline_builder::begin( device_context* device_ctx )
		{
			graphics_pipeline_builder builder;
			builder.device_ctx = device_ctx;
			return builder;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::disable_color_blend()
		{
			props.color_blend_attachment = color_blend_attachment_state();
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::use_default_color_blend()
		{
			props.color_blend_attachment = color_blend_attachment_state();
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::use_translucent_color_blend()
		{
			props.color_blend_attachment = color_blend_attachment_state();
			props.color_blend_attachment.blendEnable = VK_TRUE;
			props.color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
			props.color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
			props.color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			props.color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			props.color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
			props.color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			props.color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			return *this;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::disable_multisampling()
		{
			props.multisampling = multisampling_state_create_info();
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::use_default_multisampling()
		{
			props.multisampling = multisampling_state_create_info();
			return *this;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::set_multisampling( vk::SampleCountFlagBits sample_count )
		{
			props.multisampling = multisampling_state_create_info( sample_count );
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::disable_depth_stencil()
		{
			props.depth_stencil = depth_stencil_create_info( false, false, vk::CompareOp::eAlways );
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::use_default_depth_stencil()
		{
			props.depth_stencil = depth_stencil_create_info( true, true, vk::CompareOp::eLess );
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::use_inverse_depth_stencil()
		{
			props.depth_stencil = depth_stencil_create_info( true, true, vk::CompareOp::eGreaterOrEqual );
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::set_rasterization( vk::PolygonMode polygon_mode, vk::CullModeFlagBits cull_mode_flags, bool depth_bias_enable, vk::FrontFace front_face )
		{
			props.rasterizer = vulkan::rasterization_state_create_info( polygon_mode, cull_mode_flags, depth_bias_enable, front_face );
			return *this;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::enable_conservative_raster()
		{
			props.conservative_info.conservativeRasterizationMode = vk::ConservativeRasterizationModeEXT::eOverestimate;
			props.conservative_info.extraPrimitiveOverestimationSize = device_ctx->conservative_raster_props.maxExtraPrimitiveOverestimationSize;
							
			props.rasterizer.pNext = &props.conservative_info;
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::set_input_assembly( vk::PrimitiveTopology primitive_topology )
		{
			props.input_assembly = vulkan::input_assembly_create_info( primitive_topology );
			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::set_shaders( const shader_layout_builder& shader_builder )
		{
			shader_stage_create_info.clear();
			shader_stage_create_info = shader_builder.get_stage_create_info();
			shader_pipeline_layout = shader_builder.get_layout();

			return *this;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::add_dynamic_state( vk::DynamicState dynamic_state )
		{
			dynamic_states.push_back( dynamic_state );

			return *this;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::set_scissor( vk::Rect2D scissor )
		{
			props.scissor = scissor;

			return *this;
		}

        graphics_pipeline_builder& graphics_pipeline_builder::set_viewport( vk::Viewport viewport )
		{
			props.viewport = viewport;

			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::set_vertex_description( const vertex_input_description& description )
		{
			props.vertex_description = description;

			return *this;
		}

		graphics_pipeline_builder& graphics_pipeline_builder::build( const vk::RenderPass& pass )
		{
			if ( !cache_key.empty() )
			{
				auto found_pipeline = device_ctx->find_graphics_pipeline( cache_key );
				if ( found_pipeline )
				{
					pipeline = found_pipeline;
					return *this;
				}
			}

			if ( shader_stage_create_info.size() == 0 )
			{
				spdlog::critical( "Tried to create a graphics pipeline without shaders, did you forget to call set_shaders?" );
			}

			auto vertex_input_info = vertex_input_state_create_info();
			vertex_input_info.pVertexAttributeDescriptions = props.vertex_description.attributes.data();
			vertex_input_info.vertexAttributeDescriptionCount = (uint32_t) props.vertex_description.attributes.size();
			vertex_input_info.pVertexBindingDescriptions = props.vertex_description.bindings.data();
			vertex_input_info.vertexBindingDescriptionCount = (uint32_t) props.vertex_description.bindings.size();

			vk::PipelineViewportStateCreateInfo viewport_state {};
			viewport_state.viewportCount = 1;
			viewport_state.pViewports = &props.viewport;
			viewport_state.scissorCount = 1;
			viewport_state.pScissors = &props.scissor;

			vk::PipelineColorBlendStateCreateInfo color_blending {};
			color_blending.logicOpEnable = VK_FALSE;
			color_blending.logicOp = vk::LogicOp::eCopy;
			color_blending.attachmentCount = 1;
			color_blending.pAttachments = &props.color_blend_attachment;


			vk::GraphicsPipelineCreateInfo pipeline_info {};
			pipeline_info.stageCount = static_cast<uint32_t>( shader_stage_create_info.size() );
			pipeline_info.pStages = shader_stage_create_info.data();
			pipeline_info.pVertexInputState = &vertex_input_info;
			pipeline_info.pInputAssemblyState = &props.input_assembly;
			pipeline_info.pViewportState = &viewport_state;
			pipeline_info.pRasterizationState = &props.rasterizer;
			pipeline_info.pMultisampleState = &props.multisampling;
			pipeline_info.pColorBlendState = &color_blending;
			pipeline_info.pDepthStencilState = &props.depth_stencil;
			pipeline_info.layout = shader_pipeline_layout;
			pipeline_info.renderPass = pass;
			pipeline_info.subpass = 0;
			pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

			vk::PipelineDynamicStateCreateInfo dynamic_state {};
			dynamic_state.pDynamicStates = dynamic_states.data();
			dynamic_state.dynamicStateCount = (uint32_t) dynamic_states.size();
			pipeline_info.pDynamicState = &dynamic_state;

			pipeline = device_ctx->create_graphics_pipeline( pipeline_info, cache_key );

			return *this;
		}

	} // namespace vulkan

} // namespace rebel_road