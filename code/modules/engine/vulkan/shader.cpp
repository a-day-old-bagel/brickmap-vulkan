#include "shader.h"
#include "strings/string_utils.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <spirv_reflect.h>

namespace rebel_road
{

	namespace vulkan
	{

		shader_layout_builder shader_layout_builder::begin( device_context* device_ctx )
		{
			shader_layout_builder builder {};
			builder.device_ctx = device_ctx;
			return builder;
		}

		shader_layout_builder& shader_layout_builder::add_stage( const shader_module* module, const vk::ShaderStageFlagBits stage )
		{
			for ( auto const& s : stages )
			{
				if ( s.stage == stage )
				{
					spdlog::critical( "Attempted to add a shader module to already occupied stage {}", vk::to_string( stage ) );
				}
			}

			shader_stage new_stage = { module, stage };
			stages.push_back( new_stage );

			return *this;
		}

		shader_layout_builder& shader_layout_builder::build( const std::vector<reflection_override>* overrides )
		{
			if ( !cache_key.empty() )
			{
				auto found_layout = device_ctx->find_pipeline_layout( cache_key );
				if ( found_layout )
				{
					layout = found_layout;
					return *this;
				}
			}

			auto [reflected_sets, constant_ranges] = reflect_spirv( overrides );

			merge_and_create_descriptor_layouts( reflected_sets );

			create_pipeline_layout( constant_ranges );

			return *this;
		}

		std::pair<std::vector<shader_layout_builder::reflected_descriptor_set>, std::vector<vk::PushConstantRange>> shader_layout_builder::reflect_spirv( const std::vector<reflection_override>* overrides )
		{
			std::vector<reflected_descriptor_set> reflected_sets;
			std::vector<vk::PushConstantRange> constant_ranges;

			// We must find all the overrides, otherwise we trip a critcial error.
			// This is to prevent code bugs in renaming a property in the shader and forgetting to rename it in C++.
			std::vector<bool> found_overrides;
			if ( overrides )
			{
				found_overrides.resize( overrides->size() );
			}

			// Perform reflection on each stage to determine the shape of descriptors that need to be built for this shader.
			for ( auto& s : stages )
			{
				// Reflect - Shader Code

				SpvReflectShaderModule spvmodule;
				SpvReflectResult result = spvReflectCreateShaderModule( s.module->code.size() * sizeof( uint32_t ), s.module->code.data(), &spvmodule );

				// Reflect - Descriptor Sets

				uint32_t count {};
				result = spvReflectEnumerateDescriptorSets( &spvmodule, &count, NULL );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );

				std::vector<SpvReflectDescriptorSet*> refl_sets( count );
				result = spvReflectEnumerateDescriptorSets( &spvmodule, &count, refl_sets.data() );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );

				for ( auto refl_set : refl_sets )
				{
					reflected_descriptor_set reflected_set {};
					reflected_set.set_number = refl_set->set;

					reflected_set.bindings.resize( refl_set->binding_count );
					for ( uint32_t i_binding = 0; i_binding < refl_set->binding_count; ++i_binding )
					{
						// Convert the reflected set from the spirv-reflect format to one that is ready to be used.
						const SpvReflectDescriptorBinding& from_binding = *( refl_set->bindings[i_binding] );
						vk::DescriptorSetLayoutBinding& to_binding = reflected_set.bindings[i_binding];
						to_binding.binding = from_binding.binding;
						to_binding.descriptorType = static_cast<vk::DescriptorType>( from_binding.descriptor_type );
						to_binding.descriptorCount = 1;
						for ( uint32_t i_dim = 0; i_dim < from_binding.array.dims_count; ++i_dim )
						{
							// this seems broken (can write -1)
							to_binding.descriptorCount *= from_binding.array.dims[i_dim];
						}
						to_binding.stageFlags = static_cast<vk::ShaderStageFlagBits>( spvmodule.shader_stage );
						reflected_set.binding_flags.push_back( {} );

						// Override what we've read from reflection -- or crash if our expectations are not met.
						if ( overrides )
						{
							for ( int i_override = 0; i_override < overrides->size(); i_override++ )
							{
								auto override = ( *overrides )[i_override];
								if ( strcmp( from_binding.name, override.name ) == 0 )
								{
									if ( override.overriden_type != vk::DescriptorType() )
										to_binding.descriptorType = override.overriden_type;
									if ( override.array_max )
										to_binding.descriptorCount = override.array_max;
									if ( override.binding_flags )
										reflected_set.binding_flags.back() = override.binding_flags;
									found_overrides[i_override] = true;
									break;
								}
							}
						}
					}

					reflected_set.create_info.bindingCount = refl_set->binding_count;
					reflected_set.create_info.pBindings = reflected_set.bindings.data();

					reflected_sets.push_back( reflected_set );
				}

				// Reflect - Push Constants

				result = spvReflectEnumeratePushConstantBlocks( &spvmodule, &count, NULL );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );

				std::vector<SpvReflectBlockVariable*> pconstants( count );
				result = spvReflectEnumeratePushConstantBlocks( &spvmodule, &count, pconstants.data() );
				assert( result == SPV_REFLECT_RESULT_SUCCESS );

				if ( count > 0 )
				{
					vk::PushConstantRange pcs {};
					pcs.offset = pconstants[0]->offset;
					pcs.size = pconstants[0]->size;
					pcs.stageFlags = s.stage;

					constant_ranges.push_back( pcs );
				}

				spvReflectDestroyShaderModule( &spvmodule );
			}

			// Check that all overrides were encountered.
			bool found_all_overrides { true };
			for ( bool found : found_overrides )
			{
				found_all_overrides &= found;
			}
			if ( !found_all_overrides )
			{
				spdlog::critical( "Failed to find all overrides while building shader!" );
			}

			return std::make_pair( reflected_sets, constant_ranges );
		}

		void shader_layout_builder::merge_and_create_descriptor_layouts( const std::vector<shader_layout_builder::reflected_descriptor_set> reflected_sets )
		{
			// Different stages may only make use of some of the bindings in a given set.
			// We need to merge all of the bindings for a given set into a single descriptor.

			for ( uint32_t i = 0; i < max_descriptor_sets; i++ )
			{
				reflected_descriptor_set merged_set;
				merged_set.set_number = i;

				std::unordered_map<int, std::pair<vk::DescriptorSetLayoutBinding, vk::DescriptorBindingFlags>> binds;
				for ( auto& reflected_set : reflected_sets )
				{
					if ( reflected_set.set_number == i )
					{
						for ( uint32_t bind_index = 0; bind_index < reflected_set.bindings.size(); bind_index++ )
						{
							auto b = reflected_set.bindings[bind_index];
							auto it = binds.find( b.binding );
							if ( it == binds.end() )
							{
								binds[b.binding] = std::make_pair( b, reflected_set.binding_flags[bind_index] );
							}
							else
							{
								binds[b.binding].first.stageFlags |= b.stageFlags;
							}
						}
					}
				}

				for ( auto [k, v] : binds )
				{
					merged_set.bindings.push_back( v.first );
					merged_set.binding_flags.push_back( v.second );
				}

				// Update the create info for this descriptor set.
				merged_set.flags_info.bindingCount = static_cast<uint32_t>( merged_set.binding_flags.size() );
				merged_set.flags_info.pBindingFlags = merged_set.binding_flags.data();
				merged_set.create_info.pNext = &merged_set.flags_info;
				merged_set.create_info.bindingCount = (uint32_t) merged_set.bindings.size();
				merged_set.create_info.pBindings = merged_set.bindings.data();
				merged_set.create_info.flags = {};

				if ( merged_set.create_info.bindingCount > 0 )
				{
					// Allocate the real descriptor set on the vulkan device.
					set_layouts.push_back( device_ctx->create_descriptor_set_layout( merged_set.create_info ) );
				}
			}
		}

		void shader_layout_builder::create_pipeline_layout( const std::vector<vk::PushConstantRange> constant_ranges )
		{
			vk::PipelineLayoutCreateInfo pipeline_layout_info { vulkan::pipeline_layout_create_info() };
			pipeline_layout_info.pPushConstantRanges = constant_ranges.data();
			pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>( constant_ranges.size() );
			pipeline_layout_info.setLayoutCount = static_cast<uint32_t>( set_layouts.size() );
			pipeline_layout_info.pSetLayouts = set_layouts.data();

			layout = device_ctx->create_pipeline_layout( pipeline_layout_info, cache_key );
		}

		vk::PipelineLayout shader_layout_builder::get_layout() const
		{
			if ( !layout )
			{
				spdlog::critical( "Attempted to access shader_effect before it was built!" );
			}

			return layout;
		}

		std::vector<vk::PipelineShaderStageCreateInfo> shader_layout_builder::get_stage_create_info() const
		{
			std::vector<vk::PipelineShaderStageCreateInfo> pipeline_stages;
			for ( auto& s : stages )
			{
				pipeline_stages.push_back( vulkan::pipeline_shader_stage_create_info( s.stage, s.module->module ) );
			}
			return pipeline_stages;
		}

        std::pair<vk::Pipeline, vk::PipelineLayout> load_compute_shader( const std::string& shader_path, device_context* device_ctx )
        {
            // fixme: we should cache these modules
            vulkan::shader_module* compute_module = device_ctx->create_shader_module( shader_path );

            auto compute_layout_builder = shader_layout_builder::begin( device_ctx )
                .add_stage( compute_module, vk::ShaderStageFlagBits::eCompute )
                .build();

            vk::PipelineLayout layout = compute_layout_builder.get_layout();

			vk::ComputePipelineCreateInfo pipeline_info {};
			pipeline_info.stage = pipeline_shader_stage_create_info( vk::ShaderStageFlagBits::eCompute, compute_module->module );
			pipeline_info.layout = layout;

			vk::Pipeline pipeline = device_ctx->create_compute_pipeline( pipeline_info );

            return std::make_pair( pipeline, layout );
        }

	} // namespace vulkan

} // namespace rebel_road