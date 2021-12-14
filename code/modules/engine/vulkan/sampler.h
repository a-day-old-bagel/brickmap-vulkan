#pragma once

namespace rebel_road
{
    namespace vulkan
    {
        class sampler
        {
        public:
            sampler() = default;
            sampler( vk::Filter, vk::SamplerAddressMode );
            sampler( vk::Filter, vk::SamplerAddressMode, bool in_enable_anisotropy, float in_max_anisotropy );

            bool operator==( const sampler& other ) const;
            size_t hash() const;

            vk::Filter min_filter;
            vk::Filter mag_filter;
            vk::SamplerAddressMode address_mode_u;
            vk::SamplerAddressMode address_mode_v;
            vk::SamplerAddressMode address_mode_w;
            bool enable_anisotropy {};
            float max_anisotropy { 0.f };
        };



    } // namespace vulkan

} // namespace rebel_road

namespace std
{

    template<>
    struct hash<rebel_road::vulkan::sampler>
    {
        inline size_t operator()( const rebel_road::vulkan::sampler& x ) const
        {
            return x.hash();
        }
    };

}