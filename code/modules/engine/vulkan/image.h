#pragma once

#include "buffer.h"

// Helper for wrapping vulkan.hpp bindings for vma.
VkResult vmaCreateImage( VmaAllocator allocator, const vk::ImageCreateInfo* pImageCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo,
    vk::Image* pImage, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo );

namespace rebel_road
{
    namespace vulkan
    {
       struct image_face
        {
            size_t offset{};
            uint32_t width{};
            uint32_t height{};
            uint32_t face{};
            uint32_t mip{};
        };
        
        class image
        {
        public:
            image() = default;
            image( uint32_t in_width, uint32_t in_height, uint32_t in_mip_levels, vk::Format in_format, bool in_cubemap );

            void upload_pixels( const buffer<uint8_t>& staging_buffer );
            void upload_cubemap( const buffer<uint8_t>& staging_buffer, const std::vector<image_face> faces );

//            void write_to_ktx( const std::string path );

            uint32_t width {};
            uint32_t height {};
            int mip_levels { 1 };
            bool is_cubemap{ false };

            vk::Image vk_image {};
            VmaAllocation vk_allocation;
            vk::ImageView default_view {};
            vk::Format format {};

            void transition_layout( vk::CommandBuffer cmd, vk::ImageLayout layout_from, vk::ImageLayout layout_to, vk::AccessFlagBits access_from, vk::AccessFlagBits access_to, vk::ImageSubresourceRange range );
        };

    } // namespace vulkan

} // namespace rebel_road