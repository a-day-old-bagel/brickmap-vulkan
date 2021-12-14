#include "world.h"
#include "SimplexNoise.h"
#include "vulkan/worker.h"
#include "vulkan/shader.h"

namespace rebel_road
{
    namespace voxel
    {

        std::shared_ptr<world> world::create( vulkan::render_context* in_render_ctx )
        {
            return std::make_shared<world>( in_render_ctx );
        }

        world::world( vulkan::render_context* in_render_ctx )
            : device_ctx( in_render_ctx->get_device_context() ), render_ctx( in_render_ctx )
        {
            gpu_load_queue.allocate( sizeof( gpu_brick_load_queue ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY );
            gpu_load_queue_host.allocate( sizeof( gpu_brick_load_queue ), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_CPU_ONLY );
            bricks_to_gpu_staging.allocate( brick_load_queue_size * sizeof( brick ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU );
            indices_to_gpu_staging.allocate( brick_load_queue_size * sizeof( uint32_t ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU );

            worker = vulkan::worker::create( device_ctx );

            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "world_upload_bricks.comp.spv", device_ctx );
            upload_bricks_pipeline = pipe; upload_bricks_layout = layout;
        }

        world::~world()
        {
            for ( auto& chunk : chunklist )
            {
                chunk->gpu_bricks.free();
                chunk->gpu_indices.free();
            }

            gpu_supergrid_metadata_buffer.free();

            gpu_load_queue.free();
            gpu_load_queue_host.free();
            bricks_to_gpu_staging.free();
            indices_to_gpu_staging.free();

            worker.reset();
        }

        void world::generate_chunk( int start_x, int start_y, int start_z )
        {
            SimplexNoise noise( 1.f, 1.f, 2.f, 0.5f );

            std::vector<float> heights;
            heights.reserve( chunk_size * brick_size * chunk_size * brick_size );

            for ( int y = 0; y < chunk_size * brick_size; y++ )
            {
                for ( int x = 0; x < chunk_size * brick_size; x++ )
                {
                    float h = 1 - std::abs( noise.fractal( 7, ( start_x * chunk_size * brick_size + x ) / 1024.f, ( start_y * chunk_size * brick_size + y ) / 1024.f ) );
                    h *= grid_height;
                    heights.push_back( h );
                }
            }

            auto chunk = std::make_unique<voxel::chunk>();
            chunk->indices.resize( chunk_size * chunk_size * chunk_size, 0 );

            for ( int z = 0; z < chunk_size; z++ )
            {
                for ( int y = 0; y < chunk_size; y++ )
                {
                    for ( int x = 0; x < chunk_size; x++ )
                    {
                        brick brick {};
                        bool empty = true;

                        uint32_t lod_2x2x2 = 0;
                        for ( int cell_x = 0; cell_x < brick_size; cell_x++ )
                        {
                            for ( int cell_y = 0; cell_y < brick_size; cell_y++ )
                            {
                                float height = heights[cell_x + x * brick_size + ( cell_y + y * brick_size ) * brick_size * chunk_size];
                                for ( int cell_z = 0; cell_z < brick_size; cell_z++ )
                                {
                                    if ( ( start_z * chunk_size + z ) * brick_size + cell_z < height )
                                    {
                                        uint32_t sub_data = ( cell_x + cell_y * brick_size + cell_z * brick_size * brick_size ) / ( sizeof( uint32_t ) * 8 );
                                        uint32_t bit_position = ( cell_x + cell_y * brick_size + cell_z * brick_size * brick_size ) % ( sizeof( uint32_t ) * 8 );
                                        brick.data[sub_data] |= ( 1 << bit_position );
                                        empty = false;
                                        lod_2x2x2 |= 1 << ( ( ( cell_x & 0b100 ) >> 2 ) + ( ( cell_y & 0b100 ) >> 1 ) + ( cell_z & 0b100 ) );
                                    }
                                }
                            }
                        }

                        if ( !empty )
                        {
                            chunk->bricks.push_back( brick );
                            chunk->indices[x + y * chunk_size + z * chunk_size * chunk_size] = ( chunk->bricks.size() - 1 ) | brick_loaded_bit | ( lod_2x2x2 << 12 );
                        }
                    }
                }
            }

            uint32_t chunk_index = start_x + start_y * supergrid_xy + start_z * supergrid_xy * supergrid_xy;
            chunk->supergrid_index = chunk_index;
            chunklist[chunk_index] = std::move( chunk );
        }

        void world::generate()
        {
            auto begin = std::chrono::steady_clock::now();

            spdlog::info( "Generating {}x{}x{} world...", supergrid_xy, supergrid_xy, supergrid_z );

            chunklist.resize( supergrid_z * supergrid_xy * supergrid_xy );

            const int thread_count = std::thread::hardware_concurrency();
            std::vector<std::thread> threads( thread_count );

            for ( int i = 0; i < thread_count; i++ )
            {
                threads[i] = std::thread( [i, thread_count, this] ()
                    {
                        for ( int x = 0; x < supergrid_xy; x++ )
                        {
                            for ( int y = 0; y < supergrid_xy; y++ )
                            {
                                for ( int z = 0; z < supergrid_z; z++ )
                                {
                                    generate_chunk( x, y, z );
                                }
                            }
                        }
                    } );
            }

            for ( auto& i : threads )
            {
                i.join();
            }

            spdlog::info( "World generation complete [{} ms]", ( std::chrono::steady_clock::now() - begin ).count() / 1'000'000 );

            upload();
        }

        void world::upload()
        {
            auto begin = std::chrono::steady_clock::now();
            auto worker = vulkan::worker::create( device_ctx );

            std::vector<uint32_t> temp_indices( chunk_size * chunk_size * chunk_size );

            // For each chunk, create two buffers. One to contain all indexes for the gpu and one to contain all bricks that are currently loaded on the gpu.

            for ( int i = 0; i < chunklist.size(); i++ )
            {
                auto& chunk = chunklist[i];

                for ( int j = 0; j < chunk->indices.size(); j++ )
                {
                    if ( chunk->indices[j] & brick_loaded_bit )
                    {
                        // For each brick that has been created, mark it as unloaded (on the gpu) and save the lod bits.
                        temp_indices[j] = brick_unloaded_bit | ( chunk->indices[j] & brick_lod_bits );
                    }
                    else
                    {
                        temp_indices[j] = 0;
                    }
                }

                // Upload the chunk's brick indices to the GPU and note the device address of the index buffer.

                vulkan::buffer<uint32_t> index_staging;
                index_staging.allocate( chunk->indices.size() * sizeof( uint32_t ), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY );
                index_staging.upload_to_buffer( temp_indices.data(), temp_indices.size() * sizeof( uint32_t ) );

                chunk->gpu_indices.allocate( chunk->indices.size() * sizeof( uint32_t ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
                chunk->gpu_index_address = vulkan::get_buffer_device_address( chunk->gpu_indices.buf );
                gpu_supergrid_metadata.index_buf_pointers[i] = chunk->gpu_index_address;

                worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                    {
                        vk::BufferCopy copy {};
                        copy.size = index_staging.size;
                        cmd.copyBuffer( index_staging.buf, chunk->gpu_indices.buf, 1, &copy );
                    } );

                index_staging.free();

                // Each time the capacity of gpu_bricks would be exceeded, we will reallocate it at double size in process_load_queue.
                // Pick a good starting size for your application.
                chunk->gpu_bricks.allocate( supergrid_starting_size * sizeof( brick ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
                chunk->gpu_brick_address = vulkan::get_buffer_device_address( chunk->gpu_bricks.buf );
                gpu_supergrid_metadata.brick_buf_pointers[i] = chunk->gpu_brick_address;
            }

            vulkan::buffer<gpu_supergrid> gpu_supergrid_staging;
            gpu_supergrid_staging.allocate( sizeof( gpu_supergrid ), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY );
            gpu_supergrid_metadata_buffer.allocate( sizeof( gpu_supergrid ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );

            gpu_supergrid_staging.upload_to_buffer( &gpu_supergrid_metadata, sizeof( gpu_supergrid ) );

            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::BufferCopy copy {};
                    copy.size = gpu_supergrid_staging.size;
                    cmd.copyBuffer( gpu_supergrid_staging.buf, gpu_supergrid_metadata_buffer.buf, 1, &copy );
                } );

            gpu_supergrid_staging.free();

            spdlog::info( "Allocation took {} ms", ( std::chrono::steady_clock::now() - begin ).count() / 1'000'000 );

            // descriptor set
            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, bricks_to_gpu_staging.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, indices_to_gpu_staging.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 2, gpu_load_queue.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 3, gpu_supergrid_metadata_buffer.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( upload_set );
        }

        int get_chunk_index( const glm::ivec3& pos )
        {
            return pos.x / chunk_size
                + ( pos.y / chunk_size ) * supergrid_xy
                + ( pos.z / chunk_size ) * supergrid_xy * supergrid_xy;
        }

        void world::process_load_queue()
        {
            // The load queue is populated during world intersection tests.
            // If the load queue is not full and a brick is intersected, but not loaded, 
            // the load queue count will be incremented and the brick's world position placed in bricks_to_load.

            // Fetch the load queue from the gpu.
            // Note: It might be more performant to leave gpu_load_queue buf mapped to host memory and
            // allow the driver to handle writebacks. CharlesG@LunarG says "profile"
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::BufferCopy copy {};
                    copy.size = gpu_load_queue.size;
                    cmd.copyBuffer( gpu_load_queue.buf, gpu_load_queue_host.buf, 1, &copy );
                } );

            stat_brick_loads = 0;

            gpu_brick_load_queue* brick_load_queue = gpu_load_queue_host.mapped_data();
            uint32_t brick_to_load_count = std::min( static_cast<uint32_t>( brick_load_queue_size ), brick_load_queue->load_queue_count );
            if ( brick_to_load_count == 0 )
            {
                return;
            }

            stat_brick_loads = brick_to_load_count;

            int stage_index = 0;

            // Copy bricks and new indices to staging buffers.
            for ( int i = 0; i < brick_to_load_count; i++ )
            {
                const glm::ivec3& pos = brick_load_queue->bricks_to_load[i];

                // Determine the chunk this brick resides in.
                const auto chunk_index = get_chunk_index( pos );
                const auto& chunk = chunklist[chunk_index];
                const glm::ivec3 brick_pos = pos % chunk_size;

                // Look up the index for the brick within the chunk.
                const uint32_t index_of_index = brick_pos.x + brick_pos.y * chunk_size + brick_pos.z * chunk_size * chunk_size;
                const uint32_t index = chunk->indices[index_of_index];

                // Fetch the brick and calculate a new index.
                auto brick = chunk->bricks[index & brick_index_bits];
                auto new_index = ( chunk->gpu_index_highest | brick_loaded_bit | ( index & brick_lod_bits ) );

                // Place data for GPU upload.
                bricks_to_gpu_staging.mapped_data()[stage_index] = brick;
                indices_to_gpu_staging.mapped_data()[stage_index] = new_index;

                chunk->gpu_index_highest++;

                stage_index++;
            }

            // Sync chunks with the GPU.
            bool had_reallocations {};
            for ( auto& chunk : chunklist )
            {
                // Reallocate any chunk brick buffers that are now too small.
                if ( chunk->gpu_index_highest >= ( chunk->gpu_bricks.size / sizeof( brick ) ) )
                {
                    const int new_size = std::pow( 2.0, std::ceil( std::log2( chunk->gpu_index_highest + 1 ) ) );

                    vulkan::buffer<brick> new_bricks;
                    new_bricks.allocate( new_size * sizeof( brick ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY );

                    // Copy the contents of the previous brick buffer into the new brick buffer.
                    worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                        {
                            vk::BufferCopy copy {};
                            copy.size = chunk->gpu_bricks.size;
                            cmd.copyBuffer( chunk->gpu_bricks.buf, new_bricks.buf, 1, &copy );
                        } );

                    // Deallocate the old brick buffer.
                    chunk->gpu_bricks.free();

                    // Apply the new buffer and note the address. We will also need to re-upload world pointers.
                    chunk->gpu_bricks = new_bricks;
                    chunk->gpu_brick_address = vulkan::get_buffer_device_address( chunk->gpu_bricks.buf );
                    gpu_supergrid_metadata.brick_buf_pointers[chunk->supergrid_index] = chunk->gpu_brick_address;

                    had_reallocations = true;
                }
            }

            if ( had_reallocations )
            {
                // Upload new world pointers on the GPU.
                vulkan::buffer<gpu_supergrid> gpu_supergrid_staging;
                gpu_supergrid_staging.allocate( sizeof( gpu_supergrid ), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY );
                //gpu_supergrid_metadata_buffer.allocate( sizeof( gpu_supergrid ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );

                gpu_supergrid_staging.upload_to_buffer( &gpu_supergrid_metadata, sizeof( gpu_supergrid ) );

                worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                    {
                        vk::BufferCopy copy {};
                        copy.size = gpu_supergrid_staging.size;
                        cmd.copyBuffer( gpu_supergrid_staging.buf, gpu_supergrid_metadata_buffer.buf, 1, &copy );
                    } );

                gpu_supergrid_staging.free();
            }

            // Upload the bricks.
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    cmd.bindPipeline( vk::PipelineBindPoint::eCompute, upload_bricks_pipeline );
                    cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, upload_bricks_layout, 0, 1, &upload_set, 0, nullptr );
                    cmd.dispatch( stage_index, 1, 1 );
                } );

            // Write back load queue.
            brick_load_queue->load_queue_count = 0;
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    vk::BufferCopy copy {};
                    copy.size = gpu_load_queue_host.size;
                    cmd.copyBuffer( gpu_load_queue_host.buf, gpu_load_queue.buf, 1, &copy );
                } );
        }

        uint32_t world::get_brick_load_count()
        {
            return stat_brick_loads;
        }
    }
}