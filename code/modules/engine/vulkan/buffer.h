#pragma once

#include "containers/deletion_queue.h"
#include "vulkan/device_context.h"

// Helper for wrapping vulkan.hpp bindings for vma.
VkResult vmaCreateBuffer( VmaAllocator VMA_NOT_NULL allocator, const vk::BufferCreateInfo* VMA_NOT_NULL pBufferCreateInfo, const VmaAllocationCreateInfo* VMA_NOT_NULL pAllocationCreateInfo,
    vk::Buffer VMA_NULLABLE_NON_DISPATCHABLE* VMA_NOT_NULL pBuffer, VmaAllocation VMA_NULLABLE* VMA_NOT_NULL pAllocation, VmaAllocationInfo* VMA_NULLABLE pAllocationInfo );

namespace rebel_road
{
    namespace vulkan
    {
        void* get_vma_allocation_mapped_data( const VmaAllocation allocation );

        template<typename T>
        class buffer
        {
        public:
            void allocate( size_t alloc_size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage, vk::MemoryPropertyFlags required_flags = {} )
            {
                vk::BufferCreateInfo buffer_info {};
                buffer_info.size = alloc_size;
                buffer_info.usage = usage;

                size = alloc_size;

                const VmaAllocationCreateInfo vma_alloc_info
                {
                    // The VMA documentation says that it's okay to leave this bit set for most buffers (excluding ones that might become lost).
                    // It will be ignored for any GPU only buffers. It may become an issue if an extremely large buffer is mapped?
                    .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                    .usage = memory_usage,
                    .requiredFlags = static_cast<VkMemoryPropertyFlags>( required_flags )
                };

                vmaCreateBuffer( vulkan::device_context_locator::get()->allocator, &buffer_info, &vma_alloc_info, &buf, &allocation, nullptr );
            }

            void free()
            {
                if ( buf )
                {
                    vmaDestroyBuffer( vulkan::device_context_locator::get()->allocator, buf, allocation );
                }
            }

            T* mapped_data() const
            {
                return reinterpret_cast<T*>( get_vma_allocation_mapped_data( allocation ) );
            }

            void upload_to_buffer( const T* data, size_t upload_size, size_t offset = 0 )
            {
                T* address = mapped_data();
                memcpy( reinterpret_cast<std::byte*>( address ) + offset, data, upload_size );
            }

            inline vk::DescriptorBufferInfo get_info( VkDeviceSize offset = 0 ) const
            {
                vk::DescriptorBufferInfo info {};
                info.buffer = buf;
                info.offset = offset;
                info.range = size;
                return info;
            }

            vk::BufferMemoryBarrier get_memory_barrier( vk::AccessFlags src_access_mask, vk::AccessFlags dst_access_mask, uint32_t src_queue_family = VK_QUEUE_FAMILY_IGNORED, uint32_t dst_queue_family = VK_QUEUE_FAMILY_IGNORED ) const
            {
                vk::BufferMemoryBarrier barrier {};
                barrier.size = size;
                barrier.buffer = buf;
                barrier.srcAccessMask = src_access_mask;
                barrier.dstAccessMask = dst_access_mask;
                barrier.srcQueueFamilyIndex = src_queue_family;
                barrier.dstQueueFamilyIndex = dst_queue_family;
                return barrier;
            }

            void set_debug_tag( const std::string& tag )
            {
                #ifdef DEBUG_MARKER_ENABLE

                VkBuffer vk_buf = buf;
                vk::DebugMarkerObjectNameInfoEXT name_info {};
                name_info.objectType = vk::DebugReportObjectTypeEXT::eBuffer;
                name_info.object = (uint64_t) vk_buf;
                name_info.pObjectName = tag.c_str();
                VK_CHECK( vulkan::device_context_locator::get()->device.debugMarkerSetObjectNameEXT( &name_info ) );

                #endif
            }

        public:
            vk::Buffer buf {};
            VmaAllocation allocation {};
            vk::DeviceSize size { 0 };
        };

        vk::DeviceAddress get_buffer_device_address( const vk::Buffer& buf );

    }
}
