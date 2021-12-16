#include "descriptors.h"
#include "hash/hash_utils.h"

#include <imgui.h>

// Based on https://vkguide.dev/

namespace rebel_road
{

	namespace vulkan
	{

		void descriptor_allocator::init( vk::Device new_device )
		{
			device = new_device;
		}

		void descriptor_allocator::destroy_pools()
		{
			for ( auto p : free_pools )
			{
				device.destroyDescriptorPool( p );
			}
			for ( auto p : used_pools )
			{
				device.destroyDescriptorPool( p );
			}
		}

		void descriptor_allocator::reset_pools()
		{
			for ( auto p : used_pools )
			{
				device.resetDescriptorPool( p, {} );
				free_pools.push_back( p );
			}

			used_pools.clear();

			current_pool = VK_NULL_HANDLE;
		}

		vk::DescriptorPool descriptor_allocator::create_pool( vk::Device device, int count, vk::DescriptorPoolCreateFlags flags )
		{
			std::vector<vk::DescriptorPoolSize> sizes;
			sizes.reserve( descriptor_sizes.sizes.size() );
			for ( auto sz : descriptor_sizes.sizes )
			{
				sizes.push_back( { sz.first, uint32_t( sz.second * count ) } );
			}

			vk::DescriptorPoolCreateInfo pool_info {};
			pool_info.flags = flags;
			pool_info.maxSets = count;
			pool_info.poolSizeCount = static_cast<uint32_t>( sizes.size() );
			pool_info.pPoolSizes = sizes.data();

			return device.createDescriptorPool( pool_info );
		}

		vk::DescriptorPool descriptor_allocator::grab_pool()
		{
			if ( free_pools.size() > 0 )
			{
				vk::DescriptorPool pool = free_pools.back();
				free_pools.pop_back();
				return pool;
			}
			else
			{
				return create_pool( device, max_pool_size, {} );
			}
		}

		bool descriptor_allocator::allocate( vk::DescriptorSet& set, vk::DescriptorSetLayout layout, uint32_t variable_alloc_count )
		{
			if ( !current_pool )
			{
				current_pool = grab_pool();
				used_pools.push_back( current_pool );
			}

			vk::DescriptorSetAllocateInfo alloc_info {};
			alloc_info.pSetLayouts = &layout;
			alloc_info.descriptorPool = current_pool;
			alloc_info.descriptorSetCount = 1;

			vk::DescriptorSetVariableDescriptorCountAllocateInfo var_alloc_info {};
			if ( variable_alloc_count > 0 )
			{
				var_alloc_info.descriptorSetCount = 1;
				var_alloc_info.pDescriptorCounts = &variable_alloc_count;
				alloc_info.pNext = &var_alloc_info;
			}

			bool need_reallocate { false };

			try
			{
				set = std::move( device.allocateDescriptorSets( alloc_info ).front() );
			}
			catch ( vk::FragmentedPoolError& )
			{
				need_reallocate = true;
			}
			catch ( vk::OutOfPoolMemoryError& )
			{
				need_reallocate = true;
			}
			catch ( vk::SystemError& )
			{
				return false;
			}

			if ( !need_reallocate )
				return true;

			current_pool = grab_pool();
			used_pools.push_back( current_pool );
			alloc_info.descriptorPool = current_pool;
			// Gather stats on the kind of allocation that exceeded the pool size?

			try
			{
				set = std::move( device.allocateDescriptorSets( alloc_info ).front() );
			}
			catch ( vk::SystemError& )
			{
				spdlog::critical( "Failed to reallocate descriptor set!" );
				return false;
			}

			return true;
		}

		void descriptor_layout_cache::init( vk::Device new_device )
		{
			device = new_device;
		}

		void descriptor_layout_cache::shutdown()
		{
			for ( auto pair : layout_cache )
			{
				device.destroyDescriptorSetLayout( pair.second );
			}
		}

		vk::DescriptorSetLayout descriptor_layout_cache::create_descriptor_set_layout( const vk::DescriptorSetLayoutCreateInfo& info )
		{
			const vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT* flags_info = static_cast<const vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT*> ( info.pNext );

			descriptor_layout_key layout_key {};
			layout_key.bindings.reserve( info.bindingCount );
			bool is_sorted { true };
			int last_binding { -1 };

			for ( uint32_t i = 0; i < info.bindingCount; i++ )
			{
				descriptor_layout_key::combined_binding cb;
				cb.binding = info.pBindings[i];
				cb.flags = flags_info->pBindingFlags[i];
				layout_key.bindings.push_back( cb );

				//				layout_info.bindings.push_back( info.pBindings[i] );
				//				layout_info.binding_flags.push_back( flags_info->pBindingFlags[i] );

				if ( (int) info.pBindings[i].binding > last_binding )
				{
					last_binding = info.pBindings[i].binding;
				}
				else
				{
					is_sorted = false;
				}
			}

			if ( !is_sorted )
			{
				std::sort( layout_key.bindings.begin(), layout_key.bindings.end(), [] ( descriptor_layout_key::combined_binding& a, descriptor_layout_key::combined_binding& b )
					{
						if ( a.binding < b.binding )
							return true;
						if ( a.flags < b.flags )
							return true;
						return false;
					} );
			}

			auto it = layout_cache.find( layout_key );
			if ( it != layout_cache.end() )
			{
				return ( *it ).second;
			}
			else
			{
				vk::DescriptorSetLayout layout {};
				layout = device.createDescriptorSetLayout( info );
				layout_cache[layout_key] = layout;
				return layout;
			}
		}

		bool descriptor_layout_cache::descriptor_layout_key::operator==( const descriptor_layout_key& rhs ) const
		{
			if ( rhs.bindings.size() != bindings.size() )
			{
				return false;
			}
			else
			{
				for ( int i = 0; i < bindings.size(); i++ )
				{
					if ( rhs.bindings[i].binding != bindings[i].binding )
						return false;

					if ( rhs.bindings[i].flags != bindings[i].flags )
						return false;
				}

				return true;
			}
		}

		size_t descriptor_layout_cache::descriptor_layout_key::hash() const
		{
			size_t seed { 0 };
			for ( int i = 0; i < bindings.size(); i++ )
			{
				auto b = bindings[i];
				hash_combine( seed, b.binding.binding );
				hash_combine( seed, b.binding.descriptorType );
				hash_combine( seed, b.binding.descriptorCount );
				hash_combine( seed, b.binding.stageFlags );
				hash_combine( seed, b.flags );
			}

			return seed;
		}

		void descriptor_layout_cache::render_stats()
		{
			ImGui::Text( "Descriptor Layouts: %i", static_cast<uint32_t>( layout_cache.size() ) );
		}

		descriptor_builder descriptor_builder::begin( descriptor_layout_cache* layout_cache, descriptor_allocator* allocator )
		{
			descriptor_builder builder {};
			builder.cache = layout_cache;
			builder.alloc = allocator;
			builder.image_infos.reserve( reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set );
			builder.buffer_infos.reserve( reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set );
			builder.as_infos.reserve( reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set );
			return builder;
		}

		descriptor_builder& descriptor_builder::bind_buffer( uint32_t binding, vk::DescriptorBufferInfo buffer_info, vk::DescriptorType type, vk::ShaderStageFlags stage_flags )
		{
			assert( buffer_infos.size() < reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set );

			vk::DescriptorBindingFlags new_binding_flags {};
			binding_flags.push_back( new_binding_flags );

			buffer_infos.push_back( buffer_info );

			vk::DescriptorSetLayoutBinding new_binding {};
			new_binding.descriptorCount = 1;
			new_binding.descriptorType = type;
			new_binding.pImmutableSamplers = nullptr;
			new_binding.stageFlags = stage_flags;
			new_binding.binding = binding;

			bindings.push_back( new_binding );

			vk::WriteDescriptorSet new_write {};
			new_write.descriptorCount = 1;
			new_write.descriptorType = type;
			new_write.pBufferInfo = &buffer_infos.back();
			new_write.dstBinding = binding;

			writes.push_back( new_write );

			variable_alloc_count = 0;

			return *this;
		}

		descriptor_builder& descriptor_builder::bind_image( uint32_t binding, vk::DescriptorImageInfo image_info, vk::DescriptorType type, vk::ShaderStageFlags stage_flags )
		{
			assert( image_infos.size() < reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set );

			vk::DescriptorBindingFlags new_binding_flags {};
			binding_flags.push_back( new_binding_flags );

			image_infos.push_back( image_info );

			vk::DescriptorSetLayoutBinding new_binding {};
			new_binding.descriptorCount = 1;
			new_binding.descriptorType = type;
			new_binding.pImmutableSamplers = nullptr;
			new_binding.stageFlags = stage_flags;
			new_binding.binding = binding;

			bindings.push_back( new_binding );

			vk::WriteDescriptorSet new_write {};
			new_write.descriptorCount = 1;
			new_write.descriptorType = type;
			new_write.pImageInfo = &image_infos.back();
			new_write.dstBinding = binding;

			writes.push_back( new_write );

			variable_alloc_count = 0;

			return *this;
		}

		descriptor_builder& descriptor_builder::bind_image_array( uint32_t binding, vk::DescriptorImageInfo* desc_image_infos, uint32_t current_count, uint32_t max_count, vk::DescriptorType type, vk::ShaderStageFlags stage_flags )
		{
			vk::DescriptorBindingFlags new_binding_flags {};
			new_binding_flags = vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound;
			binding_flags.push_back( new_binding_flags );

			vk::DescriptorSetLayoutBinding new_binding {};
			new_binding.descriptorCount = max_count;		// When eVariableDescriptorCount is set, DescriptorSetLayoutBinding::descriptorCount refers to a upper bound on writes.
			new_binding.descriptorType = type;
			new_binding.pImmutableSamplers = nullptr;
			new_binding.stageFlags = stage_flags;
			new_binding.binding = binding;

			bindings.push_back( new_binding );

			vk::WriteDescriptorSet new_write {};
			new_write.descriptorCount = current_count;		// When eVariableDescriptorCount is set, WriteDescriptorSet::descriptorCount is the current number to write.
			new_write.descriptorType = type;
			new_write.pImageInfo = desc_image_infos;
			new_write.dstBinding = binding;

			writes.push_back( new_write );

			// fixme: this is awful and broken
			// Remember the lower-than-max allocation requirement.
			variable_alloc_count = current_count;

			return *this;
		}

		descriptor_builder& descriptor_builder::bind_acceleration_structure( uint32_t binding, vk::WriteDescriptorSetAccelerationStructureKHR as_info )
		{
			assert( as_infos.size() < reasonable_upper_bound_on_things_to_bind_to_one_descriptor_set );

			vk::DescriptorBindingFlags new_binding_flags {};
			binding_flags.push_back( new_binding_flags );

			as_infos.push_back( as_info );

			vk::DescriptorSetLayoutBinding new_binding {};
			new_binding.descriptorCount = 1;
			new_binding.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
			new_binding.pImmutableSamplers = nullptr;
			new_binding.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
			new_binding.binding = binding;

			bindings.push_back( new_binding );

			vk::WriteDescriptorSet new_write {};
			new_write.descriptorCount = 1;
			new_write.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
			new_write.pNext = &as_infos.back();
			new_write.dstBinding = binding;

			writes.push_back( new_write );

			return *this;
		}

		bool descriptor_builder::build( vk::DescriptorSet& set, vk::DescriptorSetLayout& layout )
		{
			vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT flags_info {};
			flags_info.bindingCount = static_cast<uint32_t>( binding_flags.size() );
			flags_info.pBindingFlags = binding_flags.data();

			vk::DescriptorSetLayoutCreateInfo layout_info {};
			layout_info.bindingCount = static_cast<uint32_t>( bindings.size() );
			layout_info.flags = {};
			layout_info.pBindings = bindings.data();
			layout_info.pNext = &flags_info;

			layout = cache->create_descriptor_set_layout( layout_info );

			bool success { alloc->allocate( set, layout, variable_alloc_count ) };
			if ( !success ) { return false; };

			for ( vk::WriteDescriptorSet& w : writes )
			{
				w.dstSet = set;
			}

			try
			{
				alloc->device.updateDescriptorSets( static_cast<uint32_t>( writes.size() ), writes.data(), 0, nullptr );
			}
			catch ( vk::SystemError err )
			{
				spdlog::error( "Failed to build descriptor set! {}", err.what() );
				return false;
			}

			return true;
		}

		bool descriptor_builder::build( vk::DescriptorSet& set )
		{
			vk::DescriptorSetLayout layout {};
			return build( set, layout );
		}

	} // namespace vulkan

} // namespace rebel_road
