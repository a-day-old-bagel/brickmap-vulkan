#include "buffer.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

VkResult vmaCreateBuffer( VmaAllocator VMA_NOT_NULL allocator, const vk::BufferCreateInfo* VMA_NOT_NULL pBufferCreateInfo, const VmaAllocationCreateInfo* VMA_NOT_NULL pAllocationCreateInfo,
    vk::Buffer VMA_NULLABLE_NON_DISPATCHABLE* VMA_NOT_NULL pBuffer, VmaAllocation VMA_NULLABLE* VMA_NOT_NULL pAllocation, VmaAllocationInfo* VMA_NULLABLE pAllocationInfo )
{
    return vmaCreateBuffer( allocator, reinterpret_cast<const VkBufferCreateInfo*>( pBufferCreateInfo ), pAllocationCreateInfo, reinterpret_cast<VkBuffer*>( pBuffer ), pAllocation, pAllocationInfo );
}

namespace rebel_road
{

    namespace vulkan
    {

        void* get_vma_allocation_mapped_data( const VmaAllocation allocation )
        {
            return allocation->GetMappedData();
        }

        vk::DeviceAddress get_buffer_device_address( const vk::Buffer& buf )
        {
            vk::BufferDeviceAddressInfo da_info {};
            da_info.buffer = buf;
            return vulkan::device_context_locator::get()->device.getBufferAddressKHR( da_info );
        }

    } // namespace vulkan

} // namespace rebel_road