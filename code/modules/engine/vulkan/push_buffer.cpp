#include "push_buffer.h"

namespace rebel_road
{

	namespace vulkan
	{

		uint32_t push_buffer::push( const void* data, const size_t size )
		{
			uint32_t offset { current_offset };

			auto* target = source.mapped_data();
			target += current_offset;

			memcpy( target, data, size );
			current_offset += static_cast<uint32_t>( size );
			current_offset = pad_uniform_buffer_size( current_offset );

			return offset;
		}

		void push_buffer::init( const buffer<uint8_t> source_buffer, const uint32_t alignment )
		{
			align = alignment;
			source = source_buffer;
			current_offset = 0;
		}

		void push_buffer::reset()
		{
			current_offset = 0;
		}

		void push_buffer::free()
		{
			reset();
			source.free();
		}

		uint32_t push_buffer::pad_uniform_buffer_size( uint32_t original_size ) const
		{
			size_t min_ubo_alignment { align };
			size_t aligned_size { original_size };
			if ( min_ubo_alignment > 0 )
			{
				aligned_size = ( aligned_size + min_ubo_alignment - 1 ) & ~( min_ubo_alignment - 1 );
			}

			return static_cast<uint32_t>( aligned_size );
		}

	} // namespace vulkan

} // namespace rebel_road