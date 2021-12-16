const int brick_size = 8;
const int cell_members = brick_size * brick_size * brick_size / 32;
const int brick_load_queue_size = 1024;
const uint brick_index_bits = 0xFFFu;
const uint brick_lod_bits = 0xFF000u;
const uint brick_loaded_bit = 0x80000000u;
const uint brick_unloaded_bit = 0x40000000u;
const uint brick_requested_bit = 0x20000000u;

struct brick
{
    uint data[cell_members];
};

const int MAX_BOUNCES = 3;

/*
const int grid_size = 4096;//128;
const int grid_height = 128;//128;

const int chunk_size = 16;
const int supergrid_xy = grid_size / brick_size / chunk_size;
const int supergrid_z = grid_height / brick_size / chunk_size;

const int supergrid_starting_size = 16;

const int cells = grid_size / brick_size;
const int cells_height = grid_height / brick_size;
// The amount of uint32_t members holding voxel bit data
const int world_size = supergrid_xy * supergrid_xy * supergrid_z;

// LoD distance for blocksize 1x1x1 representing 8x8x8
const int lod_distance_8x8x8 = 600000;
// LoD distance for blocksize 2x2x2 representing 8x8x8
const int lod_distance_2x2x2 = 100000;


*/

