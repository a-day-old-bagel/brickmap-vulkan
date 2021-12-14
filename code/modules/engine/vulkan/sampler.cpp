#include "sampler.h"
#include "hash/hash_utils.h"

namespace rebel_road
{
    namespace vulkan
    {

        sampler::sampler( vk::Filter in_filter, vk::SamplerAddressMode in_addr_mode )
        : min_filter( in_filter ), mag_filter( in_filter ), address_mode_u( in_addr_mode ), address_mode_v( in_addr_mode ), address_mode_w( in_addr_mode )
        {
        }

        sampler::sampler( vk::Filter in_filter, vk::SamplerAddressMode in_addr_mode, bool in_enable_anisotropy, float in_max_anisotropy )
        : min_filter( in_filter ), mag_filter( in_filter ), address_mode_u( in_addr_mode ), address_mode_v( in_addr_mode ), address_mode_w( in_addr_mode ),
        enable_anisotropy( in_enable_anisotropy ), max_anisotropy( in_max_anisotropy )
        {

        }

        bool sampler::operator==( const sampler& other ) const
        {
            return ( min_filter == other.min_filter && 
                     mag_filter == other.mag_filter &&
                     address_mode_u == other.address_mode_u && 
                     address_mode_v == other.address_mode_v && 
                     address_mode_w == other.address_mode_w &&
                     enable_anisotropy == other.enable_anisotropy &&
                     max_anisotropy == other.max_anisotropy );
        }
            
        size_t sampler::hash() const
        {
            std::size_t seed{ 0 };
            hash_combine( seed, min_filter );
            hash_combine( seed, mag_filter );
            hash_combine( seed, address_mode_u );
            hash_combine( seed, address_mode_v );
            hash_combine( seed, address_mode_w );
            hash_combine( seed, enable_anisotropy );
            hash_combine( seed, max_anisotropy );
            return seed;
        }

    } // namespace vulkan

} // namespace rebel_road

