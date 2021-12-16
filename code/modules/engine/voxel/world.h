#pragma once

#include "vulkan/buffer.h"
#include "vulkan/render_context.h"
#include "vulkan/worker.h"
#include "containers/deletion_queue.h"

constexpr static int grid_size = 4096;
constexpr static int grid_height = 256;
constexpr static int chunk_size = 16;
constexpr static int brick_size = 8;

constexpr static glm::vec3 world_size = { grid_size / chunk_size / brick_size, grid_size / chunk_size / brick_size, grid_height / chunk_size / brick_size };
constexpr static int chunk_count = world_size.x * world_size.y * world_size.z;

constexpr static int chunk_brick_buffer_starting_size = 16;

constexpr static int cells = grid_size / brick_size;
constexpr static int cells_height = grid_height / brick_size;

// The amount of uint32_t members holding voxel bit data.
constexpr static int cell_members = brick_size * brick_size * brick_size / 32;

constexpr static uint64_t voxel_count = grid_size * grid_size * grid_height;

// LOD distance for blocksize 1x1x1 representing 8x8x8.
constexpr static int lod_distance_8x8x8 = 600'000;
// LOD distance for blocksize 2x2x2 representing 8x8x8.
constexpr static int lod_distance_2x2x2 = 100'000;

constexpr static uint32_t brick_index_bits = 0xFFFu;
constexpr static uint32_t brick_lod_bits = 0xFF000u;
constexpr static uint32_t brick_loaded_bit = 0x80000000u;
constexpr static uint32_t brick_unloaded_bit = 0x40000000u;
constexpr static uint32_t brick_requested_bit = 0x20000000u;

constexpr static int brick_load_queue_size = 1024;

// index format, 32 bits:
// 123xxxxxxxxx11111111000000000000
// 1 = loaded
// 2 = unloaded
// 3 = requested
// x = unused??
// l = brick lod
// b = brick index

namespace rebel_road
{
    namespace voxel
    {

        struct brick
        {
            // 8^3 brick of voxels
            // 1 bit per voxel
            // array index: ( cell_x + cell_y * brick_size + cell_z * brick_size * brick_size ) / ( sizeof( uint32_t ) * 8 )
            // bit position: ( cell_x + cell_y * brick_size + cell_z * brick_size * brick_size ) % ( sizeof( uint32_t ) * 8 )
            // index within chunk: x + y * chunk_size + z * chunk_size * chunk_size
            // index is 12 bits 0xFFF
            uint32_t data[cell_members];
        };

        struct gpu_index_pointers
        {
            uint64_t index_buf_pointers[chunk_count];
        };

        struct gpu_brick_pointers
        {
            uint64_t brick_buf_pointers[chunk_count];
        };

        struct gpu_world_config
        {
            int grid_size{ ::grid_size };
            int grid_height{ ::grid_height };
            int chunk_size{ ::chunk_size };
            int chunk_count{ ::chunk_count };
            glm::ivec4 world_size{ ::world_size.x, ::world_size.y, ::world_size.z, 1 };
            int cells{ ::cells };
            int cells_height{ ::cells_height };
            int lod_distance_8x8x8{ ::lod_distance_8x8x8 };
            int lod_distance_2x2x2{ ::lod_distance_2x2x2 };
        };

        struct gpu_brick_load_queue
        {
            uint32_t load_queue_count { 0 };
            uint32_t pad1 { 0 };
            uint32_t pad2 { 0 };
            uint32_t pad3 { 0 };
            glm::ivec4 bricks_to_load[brick_load_queue_size];
        };

        struct chunk
        {
            // 16^3 (4096) bricks, 12 bit index to each brick
            // index within grid: x + y * world_size.x + z * world_size.x * world_size.y

            std::vector<uint32_t> indices;          // CPU indices
            vulkan::buffer<uint32_t> gpu_indices;   // GPU indices
            vk::DeviceAddress gpu_index_address;    // Device address of GPU indices

            std::vector<brick> bricks;              // CPU bricks
            vulkan::buffer<brick> gpu_bricks;       // GPU bricks
            vk::DeviceAddress gpu_brick_address;    // Device address of GPU bricks

            int gpu_index_highest {};               // Highest utilized brick index of GPU loaded bricks.
            uint32_t world_ptr_index {};            // Location of our bricks & indices within gpu_*_pointers.
        };

        class world
        {
        public:

            static std::shared_ptr<world> create( vulkan::render_context* in_render_ctx );

            ~world();
            world() = delete;
            world( vulkan::render_context* in_render_ctx );

            void generate();
            void tick( float delta_time );

            void process_load_queue();

            vk::DescriptorBufferInfo get_world_buffer_info() { return gpu_world_conf.get_info(); }
            vk::DescriptorBufferInfo get_index_buffer_info() { return gpu_world_index_ptrs.get_info(); }
            vk::DescriptorBufferInfo get_brick_buffer_info() { return gpu_world_brick_ptrs.get_info(); }
            vk::DescriptorBufferInfo get_load_queue_info() { return bricks_requested_by_gpu.get_info(); }

            uint32_t get_brick_load_count();
            uint64_t get_filled_voxel_count() { return filled_voxels; }

            // Semaphore World -> Ray Tracer 
            uint64_t get_ray_tracer_wait_value() { return proc_frames; }
            uint64_t get_ray_tracer_signal_value() { return world_frame; }
            vk::Semaphore get_ray_tracer_wait_semaphore() { return brick_proc_semaphore; }
            vk::Semaphore get_ray_tracer_signal_semaphore() { return brick_halt_semaphore; }

        private:
            void generate_chunk( int start_x, int start_y, int start_z );

            void load_requested_bricks();

            std::vector<std::unique_ptr<chunk>> chunklist;
            std::vector<uint64_t> filled_voxel_counts;

            vulkan::buffer<gpu_world_config> gpu_world_conf;
            gpu_world_config world_conf{};

            vulkan::buffer<gpu_index_pointers> gpu_world_index_ptrs;
            gpu_index_pointers world_index_ptrs{};

            vulkan::buffer<gpu_brick_pointers> gpu_world_brick_ptrs;
            gpu_brick_pointers world_brick_ptrs{};

            vulkan::buffer<gpu_brick_load_queue> bricks_requested_by_gpu;
            vulkan::buffer<brick> gpu_bricks_to_load;
            vulkan::buffer<uint32_t> gpu_indices_to_load;
            uint32_t oubound_bricks{};

            vk::Pipeline upload_bricks_pipeline;
            vk::PipelineLayout upload_bricks_layout;
            vk::DescriptorSet upload_set;

            uint32_t stat_brick_loads {};
            bool load_queue_initialized {};

            // Synchronization objects for loading bricks onto the GPU.
            // I don't completely understand timeline semaphores yet and it is probably possible to do this with fewer semaphores or an entirely different sync design.
            // After the ray tracer completes a pass, bricks_requested_by_gpu is valid to be read once transfers complete. This must be synchronized.
            // We then do any necessary reallocations and send the bricks to the GPU to be processed, which also needs to be synchronzied.
            // When that's done, we allow the ray tracer to continue.
            vk::CommandBuffer brick_loader_cmd;
            vk::CommandPool brick_loader_command_pool;
            vk::CommandBuffer brick_processor_cmd;
            vk::CommandPool brick_processor_command_pool;
            vk::Semaphore brick_load_semaphore;                     // ... indicates we are busy writing / transferring gpu_bricks_to_load / gpu_indices_to_load (CPU to GPU buffers)
            vk::Semaphore brick_proc_semaphore;                     // ... indicates we are reallocating brick buffers or executing the compute operation to place the uploaded bricks.
            vk::Semaphore brick_halt_semaphore;                     // ... indicates the ray tracer is busy ... prevents the world from checking the requested bricks count which will not be stable until after tracing & transfer.
            util::deletion_queue brick_loader_deletion_queue;
            uint64_t loader_frames{};
            uint64_t proc_frames{};
            uint64_t world_frame{};

            vulkan::render_context* render_ctx {};
            vulkan::device_context* device_ctx {};
            std::shared_ptr<vulkan::worker> worker;

            uint64_t filled_voxels{};
        };

    }
}
