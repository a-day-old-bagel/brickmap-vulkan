#include "image.h"
#include "buffer.h"
#include "worker.h"

namespace rebel_road
{
    namespace vulkan
    {

        image::image( uint32_t in_width, uint32_t in_height, uint32_t in_mip_levels, vk::Format in_format, bool in_cubemap )
            : width( in_width ), height( in_height ), mip_levels( in_mip_levels ), format( in_format ), is_cubemap( in_cubemap )
        {
            auto device_ctx = vulkan::device_context_locator::get();

            VmaAllocationCreateInfo image_alloc_info { .usage = VMA_MEMORY_USAGE_GPU_ONLY };

            vk::Extent3D image_extent { width, height, 1 };
            vk::ImageCreateInfo image_create_info { vulkan::image_create_info( format, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, image_extent, mip_levels ) };

            if ( is_cubemap )
            {
                image_create_info.arrayLayers = 6; // In Vulkan, cube faces are array layers.
                image_create_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;
            }

            auto [img, alloc] = device_ctx->create_image( image_create_info, image_alloc_info );
            vk_image = img;
            vk_allocation = alloc;
        }

        void image::upload_pixels( const buffer<uint8_t>& staging_buffer )
        {
            auto device_ctx = vulkan::device_context_locator::get();

            auto worker = worker::create( device_ctx );

            vk::Extent3D image_extent { width, height, 1 };


            // Copy the staging buffer contents to the image.
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::ImageSubresourceRange range = vulkan::image_subresource_range( 0, 1 );

                    // ??? -> Transfer Dst
                    transition_layout( cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, vk::AccessFlagBits::eTransferWrite, range );

                    // Copy the staging buffer bits into the image.
                    vk::BufferImageCopy copy_region = vulkan::buffer_image_copy( image_extent );
                    cmd.copyBufferToImage( staging_buffer.buf, vk_image, vk::ImageLayout::eTransferDstOptimal, 1, &copy_region );

                    // Transfer Dst -> Transfer Src
                    transition_layout( cmd, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, range );

                } );

            // Create the image mips.
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    for ( int i = 1; i < mip_levels; i++ )
                    {
                        vk::ImageSubresourceRange mip_sub_range = vulkan::image_subresource_range( i, 1 );

                        // ??? -> Transfer Dst
                        transition_layout( cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, vk::AccessFlagBits::eTransferWrite, mip_sub_range );

                        // Write the mip map.
                        vk::ImageBlit image_blit = vulkan::image_blit( width, height, i );
                        assert( width >> i );
                        assert( height >> i );
                        cmd.blitImage( vk_image, vk::ImageLayout::eTransferSrcOptimal, vk_image, vk::ImageLayout::eTransferDstOptimal, 1, &image_blit, vk::Filter::eLinear );

                        // Transfer Dst -> Transfer Src
                        transition_layout( cmd, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, mip_sub_range );
                    }
                } );

            // Convert the whole image to a shader readable format.
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::ImageSubresourceRange range = vulkan::image_subresource_range( 0, mip_levels );
                    range.layerCount = is_cubemap ? 6 : 1;
                    transition_layout( cmd, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, range );
                } );

            // Create a default image view.
            vk::ImageViewCreateInfo image_view_create_info { vulkan::image_view_create_info( format, vk_image, vk::ImageAspectFlagBits::eColor, mip_levels, is_cubemap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D ) };
            default_view = device_ctx->create_image_view( image_view_create_info );
        }

        void image::upload_cubemap( const buffer<uint8_t>& staging_buffer, const std::vector<image_face> faces )
        {
            auto device_ctx = vulkan::device_context_locator::get();

            auto worker = worker::create( device_ctx );

            vk::Extent3D image_extent { width, height, 1 };

            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::ImageSubresourceRange range = vulkan::image_subresource_range( 0, mip_levels );
                    range.layerCount = 6;

                    // ??? -> Transfer Dst
                    transition_layout( cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, vk::AccessFlagBits::eTransferWrite, range );

                    std::vector<vk::BufferImageCopy> buffer_copies;
                    for ( auto& subimage : faces )
                    {
                        vk::Extent3D image_extent { subimage.width, subimage.height, 1 };
                        vk::BufferImageCopy copy_region = vulkan::buffer_image_copy( image_extent, vk::ImageAspectFlagBits::eColor, subimage.mip, subimage.face, 1, subimage.offset );

                        buffer_copies.push_back( copy_region );
                    }
                    cmd.copyBufferToImage( staging_buffer.buf, vk_image, vk::ImageLayout::eTransferDstOptimal, buffer_copies );

                    // Transfer Dst -> Transfer Src
                    transition_layout( cmd, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, range );
                }
            );

            // Convert the whole image to a shader readable format.
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::ImageSubresourceRange range = vulkan::image_subresource_range( 0, mip_levels );
                    range.layerCount = is_cubemap ? 6 : 1;
                    transition_layout( cmd, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, range );
                } );

            // Create a default image view.
            vk::ImageViewCreateInfo image_view_create_info { vulkan::image_view_create_info( format, vk_image, vk::ImageAspectFlagBits::eColor, mip_levels, is_cubemap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D ) };
            default_view = device_ctx->create_image_view( image_view_create_info );
        }

        void image::transition_layout( vk::CommandBuffer cmd, vk::ImageLayout layout_from, vk::ImageLayout layout_to, vk::AccessFlagBits access_from, vk::AccessFlagBits access_to, vk::ImageSubresourceRange range )
        {
            vk::ImageMemoryBarrier barrier { vulkan::image_barrier( vk_image, access_from, access_to, layout_from, layout_to, {} ) };
            barrier.subresourceRange = range;
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, 0, nullptr, 0, nullptr, 1, &barrier );
        }

        void image::transition_layout( vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout layout_from, vk::ImageLayout layout_to, vk::AccessFlagBits access_from, vk::AccessFlagBits access_to, vk::ImageSubresourceRange range )
        {
            vk::ImageMemoryBarrier barrier { vulkan::image_barrier( image, access_from, access_to, layout_from, layout_to, {} ) };
            barrier.subresourceRange = range;
            cmd.pipelineBarrier( vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, 0, nullptr, 0, nullptr, 1, &barrier );
        }

    } // namespace vulkan

} // namespace rebel_road
