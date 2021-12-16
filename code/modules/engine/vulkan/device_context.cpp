#include "device_context.h"
#include "render_context.h"
#include "shader.h"
#include "image.h"
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include <imgui.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           message_severity,
    VkDebugUtilsMessageTypeFlagsEXT                  message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data )
{
    // todo: crack message_type


    switch ( message_severity )
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            spdlog::debug( callback_data->pMessage );
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            spdlog::info( callback_data->pMessage );
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn( callback_data->pMessage );
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error( callback_data->pMessage );
            break;
    }

    return VK_FALSE;
}

VkResult vmaCreateImage( VmaAllocator allocator, const vk::ImageCreateInfo* pImageCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo,
    vk::Image* pImage, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo )
{
    return vmaCreateImage( allocator, reinterpret_cast<const VkImageCreateInfo*>( pImageCreateInfo ), pAllocationCreateInfo, reinterpret_cast<VkImage*>( pImage ), pAllocation, pAllocationInfo );
}

namespace rebel_road
{
    vulkan::device_context* vulkan::device_context_locator::service {};

    namespace vulkan
    {

        std::unique_ptr<device_context> device_context::create( const device_context::create_info& ci )
        {
            assert( device_context_locator::get() == nullptr );

            auto context = std::make_unique<device_context>();
            context->init_device( ci );
            context->init_swapchain();
            context->init_queues();
            device_context_locator::provide( context.get() );
            return context;
        }

        device_context::~device_context()
        {}

        void device_context::shutdown()
        {
            device.waitIdle();

            deletion_queue.flush();
        }

        void device_context::init_device( const device_context::create_info& ci )
        {
            vk::DynamicLoader dl;
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
            VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

            vkb::InstanceBuilder builder;
            auto inst_ret = builder.set_app_name( ci.app_name.c_str() )
                .request_validation_layers( ci.use_validation_layers )
                .require_api_version( 1, 2, 0 )
                .set_debug_callback( &debug_callback )
                .build();

            if ( !inst_ret.has_value() )
            {
                spdlog::critical( "Failed to create Vulkan instance with error code {} ({}).", inst_ret.error().value(), inst_ret.error().message() );
            }

            vkb_instance = inst_ret.value();

            VULKAN_HPP_DEFAULT_DISPATCHER.init( vkb_instance.instance );

            VkSurfaceKHR glfw_surface;
            glfwCreateWindowSurface( vkb_instance.instance, ci.window, nullptr, &glfw_surface );
            surface = glfw_surface;

            vkb::PhysicalDeviceSelector selector { vkb_instance };
            selector.set_minimum_version( 1, 2 ).set_surface( surface );

            selector.set_required_features( ci.required_features );
            selector.set_required_features_12( ci.required_features_12 );

            for ( auto ext : ci.required_extensions )
            {
                selector.add_required_extension( ext );
            }

            #ifdef DEBUG_MARKER_ENABLE
            selector.add_required_extension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
            #endif

            auto physical_device_result = selector.select();
            if ( !physical_device_result )
            {
                spdlog::critical( "Failed to select physical device: {}", physical_device_result.error().message() );
            }
            vkb::PhysicalDevice physical_device = physical_device_result.value();

            vkb::DeviceBuilder device_builder { physical_device };
            auto device_builder_result = device_builder.build();
            if ( !device_builder_result )
            {
                spdlog::critical( "Failed to build physical device context: {}", device_builder_result.error().message() );
            }
            vkb_device = device_builder_result.value();

            device = vkb_device.device;
            defaultGPU = physical_device.physical_device;

            VULKAN_HPP_DEFAULT_DISPATCHER.init( device );

            gpu_props = defaultGPU.getProperties();
            spdlog::info( "GPU Min Buffer Alignment: {}", gpu_props.limits.minUniformBufferOffsetAlignment );

            vk::PhysicalDeviceProperties2 device_props {};
            device_props.pNext = &raytracing_props;
            raytracing_props.pNext = &conservative_raster_props;
            defaultGPU.getProperties2( &device_props );

            const VmaAllocatorCreateInfo allocator_info
            {
                .flags = ci.alloc_flags,
                .physicalDevice = defaultGPU,
                .device = device,
                .instance = vkb_instance.instance,
                .vulkanApiVersion = VK_API_VERSION_1_2
            };
            VMA_CHECK( vmaCreateAllocator( &allocator_info, &allocator ) );

            deletion_queue.push_function( [=] ()
                {
                    vmaDestroyAllocator( allocator );
                    device.destroy();
                    vkb::destroy_surface( vkb_instance, surface );
                    vkb::destroy_instance( vkb_instance );
                } );
        }

        void device_context::init_swapchain()
        {
            vkb::SwapchainBuilder swapchain_builder( vkb_device );

            auto vkb_swapchain_result = swapchain_builder
                .set_old_swapchain( vkb_swapchain )
                .use_default_format_selection()
                .set_image_usage_flags( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT )
                .set_desired_present_mode( VK_PRESENT_MODE_FIFO_KHR )
                .build();

            if ( !vkb_swapchain_result )
            {
                spdlog::critical( "Failed to build swapchain: {}", vkb_swapchain_result.error().message() );
            }

            vkb::destroy_swapchain( vkb_swapchain );
            vkb_swapchain = vkb_swapchain_result.value();
            swapchain = vkb_swapchain.swapchain;

            auto images = vkb_swapchain.get_images().value();
            for ( auto image : images )
            {
                swapchain_images.push_back( image );
            }

            auto image_views = vkb_swapchain.get_image_views().value();
            for ( auto image_view : image_views )
            {
                swapchain_image_views.push_back( image_view );
            }

            deletion_queue.push_function( [=] ()
                {
                    vkb_swapchain.destroy_image_views( image_views );
                    vkb::destroy_swapchain( vkb_swapchain );

                    swapchain_images.clear();
                    swapchain_image_views.clear();
                } );
        }

        uint32_t device_context::get_swapchain_image_count()
        {
            return vkb_swapchain.image_count;
        }

        vk::ImageView device_context::get_swapchain_image_view( int idx )
        {
            return swapchain_image_views[idx];
        }

        vk::Format device_context::get_swapchain_image_format()
        {
            return vk::Format { vkb_swapchain.image_format };
        }

        void device_context::init_queues()
        {
            compute_queue = vkb_device.get_queue( vkb::QueueType::compute ).value();
            compute_queue_family = vkb_device.get_queue_index( vkb::QueueType::compute ).value();
            graphics_queue = vkb_device.get_queue( vkb::QueueType::graphics ).value();
            graphics_queue_family = vkb_device.get_queue_index( vkb::QueueType::graphics ).value();
            transfer_queue = vkb_device.get_queue( vkb::QueueType::transfer ).value();
            transfer_queue_family = vkb_device.get_queue_index( vkb::QueueType::transfer ).value();
        }

        std::shared_ptr<render_context> device_context::create_render_context()
        {
            return render_context::create( this );
        }

        std::shared_ptr<image> device_context::create_render_target( vk::Format image_format, vk::ImageUsageFlags image_usage, vk::Extent3D image_extent, vk::ImageAspectFlagBits image_aspect, uint32_t mip_levels  )
        {
            auto new_image = std::make_shared<image>();

            const VmaAllocationCreateInfo alloc_info
            {
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VkMemoryPropertyFlags( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
            };

            vk::ImageCreateInfo image_info = image_create_info( image_format, image_usage, image_extent, mip_levels );
            auto [img, alloc] = create_image( image_info, alloc_info );
            new_image->vk_image = img; new_image->vk_allocation = alloc;

            vk::ImageViewCreateInfo image_view_info = image_view_create_info( image_format, new_image->vk_image, image_aspect, mip_levels );
            new_image->default_view = create_image_view( image_view_info );
            new_image->format = image_format;

            return new_image;
        }

        vk::RenderPass device_context::create_render_pass( vk::RenderPassCreateInfo render_pass_info )
        {
            render_pass_key key {};
            key.info = render_pass_info;

            auto it = render_pass_cache.find( key );
            if ( it != render_pass_cache.end() )
            {
                return ( *it ).second;
            }
            else
            {
                auto render_pass = device.createRenderPass( render_pass_info );
                deletion_queue.push_function( [=] () { device.destroyRenderPass( render_pass ); } );
                render_pass_cache[key] = render_pass;
                return render_pass;
            }
        }

        vk::Framebuffer device_context::create_framebuffer( vk::RenderPass render_pass, const std::vector<vk::ImageView>& attachments, vk::Extent2D extent, const std::string& key )
        {
            auto fb_info = vulkan::framebuffer_create_info( render_pass, extent );
            fb_info.pAttachments = attachments.data();
            fb_info.attachmentCount = attachments.size();

            if ( !key.empty() )
            {
                auto it = framebuffer_cache.find( key );
                if ( it != framebuffer_cache.end() )
                {
                    return ( *it ).second;
                }
            }

            auto framebuffer = device.createFramebuffer( fb_info );
            deletion_queue.push_function( [=] () { device.destroyFramebuffer( framebuffer ); } );

            if ( !key.empty() )
            {
                framebuffer_cache[key] = framebuffer;
            }

            return framebuffer;
        }

        shader_module* device_context::create_shader_module( const std::string& name )
        {
            auto it = shader_module_cache.find( name );
            if ( it != shader_module_cache.end() )
            {
                return ( *it ).second.get();
            }
            else
            {
                std::string base_prefix = "../shaders/";
                std::string full_path = base_prefix + name;

                std::ifstream file( full_path, std::ios::ate | std::ios::binary );
                if ( !file.is_open() )
                {
                    spdlog::critical( "Failed to open shader file {}", full_path );
                    return nullptr;
                }

                size_t file_size { (size_t) file.tellg() };
                std::vector<uint32_t> buffer( file_size / sizeof( uint32_t ) );

                file.seekg( 0 );
                file.read( (char*) buffer.data(), file_size );
                file.close();

                vk::ShaderModuleCreateInfo create_info {};
                create_info.codeSize = buffer.size() * sizeof( uint32_t );
                create_info.pCode = buffer.data();
                auto module = device.createShaderModule( create_info );
                deletion_queue.push_function( [=] () { device.destroyShaderModule( module ); } );

                auto new_shader_module = std::make_unique<shader_module>();
                new_shader_module->module = module;
                new_shader_module->code = std::move( buffer );
                shader_module_cache[name] = std::move( new_shader_module );

                return shader_module_cache[name].get();
            }
        }

        vk::DescriptorSetLayout device_context::create_descriptor_set_layout( const vk::DescriptorSetLayoutCreateInfo& create_info )
        {
            auto layout = device.createDescriptorSetLayout( create_info );
            deletion_queue.push_function( [=] () { device.destroyDescriptorSetLayout( layout ); } );
            return layout;
        }

        vk::PipelineLayout device_context::create_pipeline_layout( const vk::PipelineLayoutCreateInfo& create_info, const std::string& key )
        {
            if ( !key.empty() )
            {
                auto it = pipeline_layout_cache.find( key );
                if ( it != pipeline_layout_cache.end() )
                {
                    return ( *it ).second;
                }
            }

            auto layout = device.createPipelineLayout( create_info );
            deletion_queue.push_function( [=] () { device.destroyPipelineLayout( layout ); } );

            if ( !key.empty() )
            {
                pipeline_layout_cache[key] = layout;
            }

            return layout;
        }

        vk::PipelineLayout device_context::find_pipeline_layout( const std::string& key )
        {
            auto it = pipeline_layout_cache.find( key );
            if ( it != pipeline_layout_cache.end() )
            {
                return ( *it ).second;
            }
            else
            {
                return {};
            }
        }

        vk::Pipeline device_context::create_graphics_pipeline( const vk::GraphicsPipelineCreateInfo& create_info, const std::string& key )
        {
            if ( !key.empty() )
            {
                auto it = pipeline_cache.find( key );
                if ( it != pipeline_cache.end() )
                {
                    return ( *it ).second;
                }
            }

            auto pipeline = device.createGraphicsPipeline( nullptr, create_info );
            deletion_queue.push_function( [=] () { device.destroyPipeline( pipeline ); } );

            if ( !key.empty() )
            {
                pipeline_cache[key] = pipeline;
            }

            return pipeline;
        }

        vk::Pipeline device_context::find_graphics_pipeline( const std::string& key )
        {
            auto it = pipeline_cache.find( key );
            if ( it != pipeline_cache.end() )
            {
                return ( *it ).second;
            }
            else
            {
                return {};
            }
        }

        vk::Pipeline device_context::create_compute_pipeline( const vk::ComputePipelineCreateInfo& create_info )
        {
            auto pipeline = device.createComputePipeline( nullptr, create_info );
            deletion_queue.push_function( [=,this] () { device.destroyPipeline( pipeline ); } );
            return pipeline;
        }

        vk::ImageView device_context::create_image_view( const vk::ImageViewCreateInfo& create_info )
        {
            auto image_view = device.createImageView( create_info );
            deletion_queue.push_function( [=,this] () { device.destroyImageView( image_view ); } );
            return image_view;
        }

        std::pair<vk::Image, VmaAllocation> device_context::create_image( const vk::ImageCreateInfo& create_info, const VmaAllocationCreateInfo& alloc_info )
        {
            vk::Image vk_image;
            VmaAllocation vk_allocation;
            vmaCreateImage( allocator, &create_info, &alloc_info, &vk_image, &vk_allocation, nullptr );
            deletion_queue.push_function( [=,this] () { vmaDestroyImage( allocator, vk_image, vk_allocation ); } );
            return std::make_pair( vk_image, vk_allocation );
        }

        vk::Sampler device_context::create_sampler( const vk::SamplerCreateInfo& create_info )
        {
            auto sampler = device.createSampler( create_info );
            deletion_queue.push_function( [=,this] () { device.destroySampler( sampler ); } );
            return sampler;
        }

        std::vector<vk::Framebuffer> device_context::create_swap_chain_framebuffers( vk::RenderPass render_pass, vk::Extent2D render_extent, std::vector<vk::ImageView> attachments )
        {
            std::vector<vk::Framebuffer> framebuffers;
            for ( int i = 0; i < get_swapchain_image_count(); i++ )
            {
                std::vector<vk::ImageView> fb_attachments { get_swapchain_image_view( i ) };
                fb_attachments.insert( fb_attachments.end(), attachments.begin(), attachments.end() );
                framebuffers.push_back( create_framebuffer( render_pass, fb_attachments, render_extent ) );
            }
            return framebuffers;
        }

        vk::CommandPool device_context::create_command_pool( uint32_t family, vk::CommandPoolCreateFlags flags )
        {
            auto command_pool_info = vulkan::command_pool_create_info( family, flags );
            auto command_pool = device.createCommandPool( command_pool_info );
            deletion_queue.push_function( [=,this] () { device.destroyCommandPool( command_pool ); } );
            return command_pool;
        }

        vk::Semaphore device_context::create_semaphore( const vk::SemaphoreCreateInfo create_info )
        {
            auto semaphore = device.createSemaphore( create_info );
            deletion_queue.push_function( [=,this] () { device.destroySemaphore( semaphore ); } );
            return semaphore;
        }

        void device_context::render_stats()
        {
            ImGui::Text( "Cached Render Passes: %i", static_cast<uint32_t>( render_pass_cache.size() ) );
            ImGui::Text( "Cached Framebuffers: %i", static_cast<uint32_t>( framebuffer_cache.size() ) );
            ImGui::Text( "Cached Shader Modules: %i", static_cast<uint32_t>( shader_module_cache.size() ) );
            ImGui::Text( "Cached Pipeline Layouts: %i", static_cast<uint32_t>( pipeline_layout_cache.size() ) );
            ImGui::Text( "Cached Pipelines: %i", static_cast<uint32_t>( pipeline_cache.size() ) );
        }


    } // namespace vulkan

} // namespace rebel_road