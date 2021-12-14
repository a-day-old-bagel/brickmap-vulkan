#include "imgui_context.h"
#include "vulkan/worker.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace rebel_road
{

    namespace imgui
    {

        std::unique_ptr<imgui_context> imgui_context::create( vulkan::device_context* in_context, GLFWwindow* in_window, vk::RenderPass in_render_pass )
        {
            return std::make_unique<imgui_context>( in_context, in_window, in_render_pass );
        }

        imgui_context::~imgui_context()
        {

        }

        imgui_context::imgui_context( vulkan::device_context* in_context, GLFWwindow* in_window, vk::RenderPass in_render_pass )
            : device_ctx( in_context ), window( in_window ), render_pass( in_render_pass )
        {
            init_imgui();
        }

        void imgui_context::shutdown()
        {
            if ( device_ctx )
            {
                device_ctx->device.waitIdle();
            }
            deletion_queue.flush();
        }

        void imgui_context::init_imgui()
        {
            vk::DescriptorPoolSize pool_sizes[] =
            {
                { vk::DescriptorType::eSampler, 1000 },
                { vk::DescriptorType::eCombinedImageSampler, 1000 },
                { vk::DescriptorType::eSampledImage, 1000 },
                { vk::DescriptorType::eStorageImage, 1000 },
                { vk::DescriptorType::eUniformTexelBuffer, 1000 },
                { vk::DescriptorType::eStorageTexelBuffer, 1000 },
                { vk::DescriptorType::eUniformBuffer, 1000 },
                { vk::DescriptorType::eStorageBuffer, 1000 },
                { vk::DescriptorType::eUniformBufferDynamic, 1000 },
                { vk::DescriptorType::eStorageBufferDynamic, 1000 },
                { vk::DescriptorType::eInputAttachment, 1000 }
            };

            vk::DescriptorPoolCreateInfo pool_info {};
            pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
            pool_info.maxSets = 1000;
            pool_info.poolSizeCount = static_cast<uint32_t>( std::size( pool_sizes ) );//11;
            pool_info.pPoolSizes = pool_sizes;

            vk::DescriptorPool imgui_pool;
            imgui_pool = device_ctx->device.createDescriptorPool( pool_info );
            deletion_queue.push_function( [=] () { device_ctx->device.destroyDescriptorPool( imgui_pool ); } );

            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            ImGui::GetIO().IniFilename = NULL;

            ImGui_ImplGlfw_InitForVulkan( window, true );

            ImGui_ImplVulkan_InitInfo init_info
            {
                .Instance = device_ctx->vkb_instance.instance,
                .PhysicalDevice = device_ctx->defaultGPU,
                .Device = device_ctx->device,
                .QueueFamily = device_ctx->graphics_queue_family,
                .Queue = device_ctx->graphics_queue,
                .DescriptorPool = imgui_pool,
                .MinImageCount = static_cast<uint32_t>( device_ctx->swapchain_images.size() ),
                .ImageCount = static_cast<uint32_t>( device_ctx->swapchain_images.size() )
            };
            ImGui_ImplVulkan_Init( &init_info, render_pass );

            // Upload imgui fonts textures.
            auto worker = vulkan::worker::create( device_ctx );
            worker->immediate_submit( [&] ( vk::CommandBuffer cmd )
                {
                    ImGui_ImplVulkan_CreateFontsTexture( cmd );
                } );

            // Clear font textures from cpu data.
            ImGui_ImplVulkan_DestroyFontUploadObjects();

            deletion_queue.push_function( [=] () { ImGui_ImplVulkan_Shutdown(); } );
        }

        void imgui_context::begin_frame()
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        void imgui_context::render()
        {
            ImGui::Render();
        }

        void imgui_context::draw( vk::CommandBuffer cmd )
        {
            ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), cmd );
        }

    } // namespace imgui

} // namespace rebel_road