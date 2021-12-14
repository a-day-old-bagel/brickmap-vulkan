#pragma once

#include "buffer.h"

namespace rebel_road
{
    namespace vulkan
    {
        class push_buffer
        {
        public:
            template<typename T>
            uint32_t push( const T& data );
            uint32_t push( const void* data, const size_t size );

            void init( const buffer<uint8_t> source_buffer, const uint32_t alignment );
            void reset();
            void free();

            uint32_t pad_uniform_buffer_size( uint32_t original_size ) const;

            vk::DescriptorBufferInfo get_info( VkDeviceSize offset = 0 ) const { return source.get_info(); }
        private:
            buffer<uint8_t> source;
            uint32_t align {};
            uint32_t current_offset {};
        };

        template<typename T>
        uint32_t push_buffer::push( const T& data )
        {
            return push( &data, sizeof( T ) );
        }
    }
}