#pragma once

#include "vulkan/buffer.h"
#include "vulkan/render_context.h"
#include "vulkan/worker.h"

constexpr static int grid_size = 256;//128;
constexpr static int grid_height = 128;//128;
constexpr static int brick_size = 8;

constexpr static int chunk_size = 16;
constexpr static int supergrid_xy = grid_size / chunk_size / brick_size;
constexpr static int supergrid_z = grid_height / chunk_size / brick_size;
constexpr static int world_size = supergrid_xy * supergrid_xy * supergrid_z;

constexpr static int supergrid_starting_size = 16;

constexpr static int cells = grid_size / brick_size;
constexpr static int cells_height = grid_height / brick_size;
// The amount of uint32_t members holding voxel bit data
constexpr static int cell_members = brick_size * brick_size * brick_size / 32;

constexpr static float epsilon = 0.001f;

// LoD distance for blocksize 1x1x1 representing 8x8x8
constexpr static int lod_distance_8x8x8 = 600'000;
// LoD distance for blocksize 2x2x2 representing 8x8x8
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
// x = unused
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

        struct gpu_supergrid
        {
            uint64_t index_buf_pointers[world_size];
            uint64_t brick_buf_pointers[world_size];
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
            // index within grid: x + y * supergrid_xy + z * supergrid_xy * supergrid_xy

            std::vector<uint32_t> indices;          // CPU indices
            vulkan::buffer<uint32_t> gpu_indices;   // GPU indices
            vk::DeviceAddress gpu_index_address;    // Device address of GPU indices

            std::vector<brick> bricks;              // CPU bricks
            vulkan::buffer<brick> gpu_bricks;       // GPU bricks
            vk::DeviceAddress gpu_brick_address;    // Device address of GPU bricks

            //int gpu_brick_count = supergrid_starting_size;// Current count of loaded bricks on the GPU.
            int gpu_index_highest {};                // Highest utilized brick index of GPU loaded bricks.

            uint32_t supergrid_index {};
        };

        class world
        {
        public:

            static std::shared_ptr<world> create( vulkan::render_context* in_render_ctx );

            ~world();
            world() = delete;
            world( vulkan::render_context* in_render_ctx );

            void generate();

            void process_load_queue();

            vk::DescriptorBufferInfo get_supergrid_info() { return gpu_supergrid_metadata_buffer.get_info(); }
            vk::DescriptorBufferInfo get_load_queue_info() { return gpu_load_queue.get_info(); }

            uint32_t get_brick_load_count();

        private:
            void generate_chunk( int start_x, int start_y, int start_z );
            void upload();

            std::vector<std::unique_ptr<chunk>> chunklist;

            vulkan::buffer<gpu_supergrid> gpu_supergrid_metadata_buffer;
            gpu_supergrid gpu_supergrid_metadata; // rename to world_pointers

            vulkan::buffer<gpu_brick_load_queue> gpu_load_queue;
            vulkan::buffer<gpu_brick_load_queue> gpu_load_queue_host;
            vulkan::buffer<brick> bricks_to_gpu_staging;
            vulkan::buffer<uint32_t> indices_to_gpu_staging;

            vulkan::render_context* render_ctx {};
            vulkan::device_context* device_ctx {};
            std::shared_ptr<vulkan::worker> worker;

            vk::Pipeline upload_bricks_pipeline;
            vk::PipelineLayout upload_bricks_layout;
            vk::DescriptorSet upload_set;

            uint32_t stat_brick_loads {};
        };

    }
}
