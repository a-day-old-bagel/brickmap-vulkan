#pragma once

namespace rebel_road
{
    namespace vulkan
    {
        class device_context;

        class worker        
        {
        public:
            static std::shared_ptr<worker> create( device_context* in_ctx );

            ~worker();
            worker() = delete;
            worker( device_context* in_ctx );

            void immediate_submit( std::function<void( vk::CommandBuffer cmd )>&& func );

        private:
            vk::Fence fence;
            vk::CommandPool command_pool;

            device_context* device_ctx {};
        };
    }
}