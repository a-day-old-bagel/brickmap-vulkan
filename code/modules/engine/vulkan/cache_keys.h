#pragma once

namespace rebel_road
{
    namespace vulkan
    {
        struct render_pass_key
        {
            vk::RenderPassCreateInfo info;

            bool operator==( const render_pass_key& other ) const;
            size_t hash() const;
        };

        struct render_pass_hash
        {
            std::size_t operator()( const render_pass_key& k ) const
            {
                return k.hash();
            }
        };
    }
}