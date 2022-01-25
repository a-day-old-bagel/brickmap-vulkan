#include "world.h"
#include "SimplexNoise.h"
#include "vulkan/worker.h"
#include "vulkan/shader.h"

namespace rebel_road
{
    namespace voxel
    {

        int get_chunk_index( const glm::ivec3& pos )
        {
            return pos.x / chunk_size
                + ( pos.y / chunk_size ) * world_size.x
                + ( pos.z / chunk_size ) * world_size.x * world_size.y;
        }

        std::shared_ptr<world> world::create( vulkan::render_context* in_render_ctx )
        {
            return std::make_shared<world>( in_render_ctx );
        }

        world::world( vulkan::render_context* in_render_ctx )
            : device_ctx( in_render_ctx->get_device_context() ), render_ctx( in_render_ctx )
        {
            gpu_world_conf.allocate( sizeof( gpu_world_config ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
            gpu_world_index_ptrs.allocate( sizeof( gpu_index_pointers ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
            gpu_world_brick_ptrs.allocate( sizeof( gpu_brick_pointers ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );

            bricks_requested_by_gpu.allocate( sizeof( gpu_brick_load_queue ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_TO_CPU );
            gpu_bricks_to_load.allocate( brick_load_queue_size * sizeof( brick ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY );
            gpu_indices_to_load.allocate( brick_load_queue_size * sizeof( uint32_t ), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY );

            worker = vulkan::worker::create( device_ctx );

            // shader
            auto [pipe, layout] = vulkan::load_compute_shader( "world_upload_bricks.comp.spv", device_ctx );
            upload_bricks_pipeline = pipe; upload_bricks_layout = layout;

            brick_loader_command_pool = device_ctx->create_command_pool( device_ctx->transfer_queue_family, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
            auto cmd_alloc_info = vulkan::command_buffer_allocate_info( brick_loader_command_pool );
            brick_loader_cmd = device_ctx->device.allocateCommandBuffers( cmd_alloc_info )[0];

            brick_processor_command_pool = device_ctx->create_command_pool( device_ctx->graphics_queue_family, vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
            cmd_alloc_info = vulkan::command_buffer_allocate_info( brick_processor_command_pool );
            brick_processor_cmd = device_ctx->device.allocateCommandBuffers( cmd_alloc_info )[0];

            vk::SemaphoreTypeCreateInfo type_info {};
            type_info.semaphoreType = vk::SemaphoreType::eTimeline;
            type_info.initialValue = 0;
            vk::SemaphoreCreateInfo semaphore_info = vulkan::semaphore_create_info();
            semaphore_info.pNext = &type_info;
            brick_load_semaphore = device_ctx->create_semaphore( semaphore_info );
            brick_proc_semaphore = device_ctx->create_semaphore( semaphore_info );
            brick_halt_semaphore = device_ctx->create_semaphore( semaphore_info );
        }

        world::~world()
        {
            for ( auto& chunk : chunklist )
            {
                chunk->gpu_bricks.free();
                chunk->gpu_indices.free();
            }

            gpu_world_conf.free();
            gpu_world_index_ptrs.free();
            gpu_world_brick_ptrs.free();

            bricks_requested_by_gpu.free();
            gpu_bricks_to_load.free();
            gpu_indices_to_load.free();

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

            uint64_t filled_count {};

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
                                        filled_count++;
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

            uint32_t chunk_index = start_x + start_y * world_size.x + start_z * world_size.x * world_size.y;
            chunk->world_ptr_index = chunk_index;
            chunklist[chunk_index] = std::move( chunk );
            filled_voxel_counts[chunk_index] = filled_count;
        }

        void world::generate()
        {
            auto begin = std::chrono::steady_clock::now();

            spdlog::info( "Generating {}x{}x{} world of {} chunks...", world_size.x, world_size.y, world_size.z, chunk_count );

            chunklist.resize( chunk_count );
            filled_voxel_counts.resize( chunk_count );

            const int thread_count = std::thread::hardware_concurrency();
            std::vector<std::thread> threads( thread_count );

            for ( int i = 0; i < thread_count; i++ )
            {
                threads[i] = std::thread( [i, thread_count, this] ()
                    {
                        for ( int x = i * ( world_size.x / float( thread_count ) ); x < ( i + 1 ) * ( world_size.x / float( thread_count ) ); x++ )
                        {
                            for ( int y = 0; y < world_size.y; y++ )
                            {
                                for ( int z = 0; z < world_size.z; z++ )
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

            begin = std::chrono::steady_clock::now();
            auto worker = vulkan::worker::create( device_ctx );

            std::vector<uint32_t> temp_indices( chunk_size * chunk_size * chunk_size );

            // For each chunk, create two buffers. One to contain all indexes for the gpu and one to contain all bricks that are currently loaded on the gpu.

            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {

                    for ( int i = 0; i < chunklist.size(); i++ )
                    {
                        //spdlog::info( "Upload chunk {}", i );
                        auto& chunk = chunklist[i];

                        filled_voxels += filled_voxel_counts[i];

                        // Optimize me: this is very slow.
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
                        world_index_ptrs.index_buf_pointers[i] = chunk->gpu_index_address;

                        vk::BufferCopy copy {};
                        copy.size = index_staging.size;
                        cmd.copyBuffer( index_staging.buf, chunk->gpu_indices.buf, 1, &copy );

                        brick_loader_deletion_queue.push_function( [index_staging] () mutable { index_staging.free(); } );

                        // Each time the capacity of gpu_bricks would be exceeded, we will reallocate it at double size in process_load_queue.
                        // We don't start with any bricks loaded on the GPU. When rays hit an unloaded brick they will request a load.

                        chunk->gpu_bricks.allocate( chunk_brick_buffer_starting_size * sizeof( brick ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY );
                        chunk->gpu_brick_address = vulkan::get_buffer_device_address( chunk->gpu_bricks.buf );
                        world_brick_ptrs.brick_buf_pointers[i] = chunk->gpu_brick_address;
                    }
                } );

            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    // Upload world data, indices, and bricks to the GPU.
                    auto staging1 = gpu_world_conf.upload_to_buffer( cmd, &world_conf, sizeof( world_conf ) );
                    brick_loader_deletion_queue.push_function( [staging1] () mutable { staging1.free(); } );

                    auto staging2 = gpu_world_index_ptrs.upload_to_buffer( cmd, &world_index_ptrs, sizeof( world_index_ptrs ) );
                    brick_loader_deletion_queue.push_function( [staging2] () mutable { staging2.free(); } );

                    auto staging3 = gpu_world_brick_ptrs.upload_to_buffer( cmd, &world_brick_ptrs, sizeof( world_brick_ptrs ) );
                    brick_loader_deletion_queue.push_function( [staging3] () mutable { staging3.free(); } );
                } );

            // Upload pointer data to GPU.

            // Create a descriptor set for copying uploaded brick data into position on the GPU.
            vulkan::descriptor_builder::begin( render_ctx->get_descriptor_layout_cache(), render_ctx->get_descriptor_allocator() )
                .bind_buffer( 0, gpu_bricks_to_load.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 1, gpu_indices_to_load.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 2, bricks_requested_by_gpu.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 3, gpu_world_index_ptrs.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 4, gpu_world_brick_ptrs.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .bind_buffer( 5, gpu_world_conf.get_info(), vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute )
                .build( upload_set );

            spdlog::info( "Allocation took {} ms", ( std::chrono::steady_clock::now() - begin ).count() / 1'000'000 );
        }

        void world::tick( float delta_time )
        {
            ZoneScopedN( "world - tick" );

            load_requested_bricks();
            process_load_queue();
            world_frame++;
        }

        void world::load_requested_bricks()
        {
            ZoneScopedN( "world - load bricks" );

            const uint64_t no_wait = 0;

            // Wait for main thread's device work to complete.
            std::vector<vk::Semaphore> wait_semaphores = { brick_halt_semaphore };
            std::vector<uint64_t> wait_values = { world_frame };

            vk::SemaphoreWaitInfo wait_info;
            wait_info.semaphoreCount = wait_semaphores.size();
            wait_info.pSemaphores = wait_semaphores.data();
            wait_info.pValues = wait_values.data();
            VK_CHECK( device_ctx->device.waitSemaphores( &wait_info, UINT64_MAX ) );

            loader_frames++;
            uint64_t signal_value = loader_frames;

            stat_brick_loads = 0;

            // Check to see if any bricks have been requested.
            gpu_brick_load_queue* requested_bricks = bricks_requested_by_gpu.mapped_data();
            uint32_t brick_to_load_count = std::min( static_cast<uint32_t>( brick_load_queue_size ), requested_bricks->load_queue_count );
            if ( brick_to_load_count > 0 )
            {
                stat_brick_loads = brick_to_load_count;

                oubound_bricks = 0;

                brick_loader_cmd.reset( {} );
                vk::CommandBufferBeginInfo begin_info {};
                brick_loader_cmd.begin( begin_info );

                // Write the requested bricks to the host->GPU buffers.
                std::vector<brick> bricks_to_load;
                std::vector<uint32_t> indices_to_load;
                for ( int i = 0; i < brick_to_load_count; i++ )
                {
                    const glm::ivec3& pos = requested_bricks->bricks_to_load[i];

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
                    bricks_to_load.push_back( brick );
                    indices_to_load.push_back( new_index );

                    chunk->gpu_index_highest++;

                    oubound_bricks++;
                }

                auto staging_bricks = gpu_bricks_to_load.upload_to_buffer( brick_loader_cmd, bricks_to_load.data(), bricks_to_load.size() * sizeof( brick ) );
                brick_loader_deletion_queue.push_function( [staging_bricks] () mutable { staging_bricks.free(); } );

                auto staging_indices = gpu_indices_to_load.upload_to_buffer( brick_loader_cmd, indices_to_load.data(), indices_to_load.size() * sizeof( uint32_t ) );
                brick_loader_deletion_queue.push_function( [staging_indices] () mutable { staging_indices.free(); } );

                brick_loader_cmd.end();

                vk::TimelineSemaphoreSubmitInfo timeline_info;
                timeline_info.waitSemaphoreValueCount = 1;
                timeline_info.pWaitSemaphoreValues = &no_wait;
                timeline_info.signalSemaphoreValueCount = 1;
                timeline_info.pSignalSemaphoreValues = &signal_value;
                auto submit_info = vulkan::submit_info( &brick_loader_cmd );

                vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eTransfer;

                submit_info.pNext = &timeline_info;
                submit_info.pWaitDstStageMask = &wait_stage;
                submit_info.signalSemaphoreCount = 1;
                submit_info.pSignalSemaphores = &brick_load_semaphore;

                VK_CHECK( render_ctx->get_transfer_queue().submit( 1, &submit_info, nullptr ) );
            }
            else
            {
                vk::SemaphoreSignalInfo signal_info {};
                signal_info.semaphore = brick_load_semaphore;
                signal_info.value = signal_value;

                VK_CHECK( device_ctx->device.signalSemaphore( &signal_info ) );
            }
        }

        void world::process_load_queue()
        {
            ZoneScopedN( "world - process load queue" );

            std::vector<vk::Semaphore> wait_semaphores = { brick_load_semaphore };
            std::vector<uint64_t> wait_values = { loader_frames };

            vk::SemaphoreWaitInfo wait_info;
            wait_info.semaphoreCount = wait_semaphores.size();
            wait_info.pSemaphores = wait_semaphores.data();
            wait_info.pValues = wait_values.data();
            VK_CHECK( device_ctx->device.waitSemaphores( &wait_info, UINT64_MAX ) );

            brick_loader_deletion_queue.flush();

            proc_frames++;
            uint64_t signal_value = proc_frames;

            if ( oubound_bricks > 0 )
            {
                auto cmd = brick_processor_cmd;
                cmd.reset( {} );
                vk::CommandBufferBeginInfo begin_info {};
                cmd.begin( begin_info );

                // The load queue is populated during world intersection tests.
                // If the load queue is not full and a brick is intersected, but not loaded, 
                // the load queue count will be incremented and the brick's world position placed in bricks_to_load.

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
                        vk::BufferCopy copy {};
                        copy.size = chunk->gpu_bricks.size;
                        cmd.copyBuffer( chunk->gpu_bricks.buf, new_bricks.buf, 1, &copy );

                        // Schedule the old brick buffer for deallocation.
                        vulkan::buffer<brick> old_bricks = chunk->gpu_bricks;
                        brick_loader_deletion_queue.push_function( [old_bricks] () mutable { old_bricks.free(); } );

                        // Apply the new buffer and note the address. We will also need to re-upload world pointers.
                        chunk->gpu_bricks = new_bricks;
                        chunk->gpu_brick_address = vulkan::get_buffer_device_address( chunk->gpu_bricks.buf );
                        world_brick_ptrs.brick_buf_pointers[chunk->world_ptr_index] = chunk->gpu_brick_address;

                        had_reallocations = true;
                    }
                }

                if ( had_reallocations )
                {
                    // Upload new world pointers on the GPU.
                    auto staging = gpu_world_brick_ptrs.upload_to_buffer( cmd, &world_brick_ptrs, sizeof( gpu_brick_pointers ) );
                    brick_loader_deletion_queue.push_function( [staging] () mutable { staging.free(); } );
                }

                // Barrier to ensure that all CPU writes are finished before shader access.
                vk::BufferMemoryBarrier cpu_writes_complete = bricks_requested_by_gpu.get_memory_barrier( vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eShaderRead );
                cmd.pipelineBarrier( vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 1, &cpu_writes_complete, 0, nullptr );

                // Copy the bricks into place on the GPU.
                cmd.bindPipeline( vk::PipelineBindPoint::eCompute, upload_bricks_pipeline );
                cmd.bindDescriptorSets( vk::PipelineBindPoint::eCompute, upload_bricks_layout, 0, 1, &upload_set, 0, nullptr );
                cmd.dispatch( oubound_bricks, 1, 1 );
                cmd.pipelineBarrier( vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 0, nullptr );

                cmd.end();

                const uint64_t no_wait = 0;

                vk::TimelineSemaphoreSubmitInfo timeline_info;
                timeline_info.waitSemaphoreValueCount = 1;
                timeline_info.pWaitSemaphoreValues = &no_wait;
                timeline_info.signalSemaphoreValueCount = 1;
                timeline_info.pSignalSemaphoreValues = &signal_value;

                auto submit_info = vulkan::submit_info( &cmd );

                vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eComputeShader;

                submit_info.pNext = &timeline_info;
                submit_info.pWaitDstStageMask = &wait_stage;
                submit_info.signalSemaphoreCount = 1;
                submit_info.pSignalSemaphores = &brick_proc_semaphore;

                VK_CHECK( render_ctx->get_graphics_queue().submit( 1, &submit_info, nullptr ) );

                oubound_bricks = 0;
            }
            else
            {

                vk::SemaphoreSignalInfo signal_info {};
                signal_info.semaphore = brick_proc_semaphore;
                signal_info.value = signal_value;
                VK_CHECK( device_ctx->device.signalSemaphore( &signal_info ) );
            }
        }

        uint32_t world::get_brick_load_count()
        {
            return stat_brick_loads;
        }
    }
}