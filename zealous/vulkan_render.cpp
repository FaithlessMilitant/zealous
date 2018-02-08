#include "vulkan_render.hpp"

#include <array>
#include <cmath>
#include <SDL_timer.h>
#include <vulkan/vulkan.hpp>

namespace zealous {
    //--------------------------------------------------------------------------
    void Renderer::InitRender( std::shared_ptr<VulkanContext> context ) {
        this->context = context;

        const vk::Device& device = context->Device();

        // create the buffer with the vertex data
        struct Vertex_Pos2f_Color4f {
            std::array<float, 2> pos;
            std::array<float, 4> color;
        };
        static_assert( sizeof( Vertex_Pos2f_Color4f ) == 6 * sizeof( float ) );

        std::array<Vertex_Pos2f_Color4f, 3> vertices;
        const size_t nVertices = vertices.size();
        for ( int i = 0; i < vertices.size(); ++i ) {
            Vertex_Pos2f_Color4f& vertex = vertices[i];

            const float angle = ( 2.f * float( M_PI ) * i / nVertices );
            vertex.pos[0] = std::sin( angle );
            vertex.pos[1] = std::cos( angle );

            constexpr std::array<float, 4> colorUsed = {1.f, 0.f, 0.f, 1.f};
            vertex.color = colorUsed;
        }

        constexpr size_t bufferSize = sizeof( vertices );

        const uint32_t familyIndex = context->GraphicsQueueFamilyIndex();
        vk::BufferCreateInfo createInfo = vk::BufferCreateInfo()
                                          .setQueueFamilyIndexCount( 1 )
                                          .setPQueueFamilyIndices( &familyIndex )
                                          .setSharingMode( vk::SharingMode::eConcurrent )
                                          .setSize( bufferSize )
                                          .setUsage( vk::BufferUsageFlagBits::eUniformBuffer );
        buffer = device.createBuffer( createInfo );

        const vk::MemoryRequirements reqs = device.getBufferMemoryRequirements( buffer );
        vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
                                           .setAllocationSize( reqs.size );

        // determine the memory type index
        const vk::MemoryPropertyFlags desiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        const vk::PhysicalDeviceMemoryProperties& memoryProperties = context->PhysicalDeviceMemoryProperties();
        for ( uint32_t i = 0, typeBits = reqs.memoryTypeBits; i < memoryProperties.memoryTypeCount; i++ ) {
            if ( ( typeBits & 1 ) == 1 ) {
                // Type is available, does it match user properties?
                if ( ( memoryProperties.memoryTypes[i].propertyFlags & desiredFlags ) == desiredFlags ) {
                    allocInfo.setMemoryTypeIndex( i );
                    break;
                }
            }
            typeBits >>= 1;
        }

        deviceMemory = device.allocateMemory( allocInfo );
        void* memory = device.mapMemory( deviceMemory, 0, bufferSize );
        memcpy( memory, &vertices, bufferSize );
        device.unmapMemory( deviceMemory );

        device.bindBufferMemory( buffer, deviceMemory, 0 );
    }

    //--------------------------------------------------------------------------
    void Renderer::RenderOnce() {
        const vk::Device& device = context->Device();
        auto valueResult = device.acquireNextImageKHR( context->Swapchain(),
                           std::numeric_limits<uint64_t>::max(),
                           context->ImageAvailableSemaphore(),
                           vk::Fence() );

        assert( valueResult.result == vk::Result::eSuccess );
        const uint32_t value = valueResult.value;

        const vk::Fence& fence = context->Fences()[value];
        vk::ArrayProxy<const vk::Fence> proxy{ fence };
        vk::Result result = device.waitForFences( proxy,
                            true, std::numeric_limits<uint64_t>::max() );
        assert( result == vk::Result::eSuccess );

        device.resetFences( proxy );

        const vk::CommandBuffer& commandBuffer = context->CommandBuffers()[value];
        commandBuffer.reset( vk::CommandBufferResetFlags() );
        vk::CommandBufferBeginInfo info = vk::CommandBufferBeginInfo()
                                          .setFlags( vk::CommandBufferUsageFlagBits::eSimultaneousUse );
        commandBuffer.begin( info );
        {
            // save the image barrier
            vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
                    .setAspectMask( vk::ImageAspectFlagBits::eColor )
                    .setBaseMipLevel( 0 )
                    .setLevelCount( 1 )
                    .setBaseArrayLayer( 0 )
                    .setLayerCount( 1 );

            const vk::Image& image = context->SwapchainImages()[value];
            vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
                                             .setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
                                             .setDstAccessMask( vk::AccessFlagBits::eMemoryRead )
                                             .setOldLayout( vk::ImageLayout::eUndefined )
                                             .setNewLayout( vk::ImageLayout::eTransferDstOptimal )
                                             .setSrcQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
                                             .setDstQueueFamilyIndex( VK_QUEUE_FAMILY_IGNORED )
                                             .setImage( image )
                                             .setSubresourceRange( subresourceRange );
            commandBuffer.pipelineBarrier( vk::PipelineStageFlagBits::eTransfer,
                                           vk::PipelineStageFlagBits::eTransfer,
                                           vk::DependencyFlags(),
                                           nullptr,
                                           nullptr,
                                           barrier );

            std::array<float, 4> color;
            double currentTime = ( double )SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
            color[0] = ( float )( 0.5 + 0.5 * SDL_sin( currentTime ) );
            color[1] = ( float )( 0.5 + 0.5 * SDL_sin( currentTime + M_PI * 2 / 3 ) );
            color[2] = ( float )( 0.5 + 0.5 * SDL_sin( currentTime + M_PI * 4 / 3 ) );
            color[3] = 1;

            vk::ClearColorValue clearValue = vk::ClearColorValue()
                                             .setFloat32( color );
            commandBuffer.clearColorImage( image, vk::ImageLayout::eTransferDstOptimal, clearValue, subresourceRange );
        }
        commandBuffer.end();

        vk::PipelineStageFlags waitDestStageMask = vk::PipelineStageFlagBits::eTransfer;
        vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setWaitSemaphoreCount( 1 )
                                    .setPWaitSemaphores( &context->ImageAvailableSemaphore() )
                                    .setPWaitDstStageMask( &waitDestStageMask )
                                    .setCommandBufferCount( 1 )
                                    .setPCommandBuffers( &commandBuffer )
                                    .setSignalSemaphoreCount( 1 )
                                    .setPSignalSemaphores( &context->DoneRenderingSemaphore() );
        context->GraphicsQueue().submit( submitInfo, fence );

        vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
                                         .setWaitSemaphoreCount( 1 )
                                         .setPWaitSemaphores( &context->DoneRenderingSemaphore() )
                                         .setSwapchainCount( 1 )
                                         .setPSwapchains( &context->Swapchain() )
                                         .setPImageIndices( &value );
        context->PresentQueue().presentKHR( presentInfo );
    }

    //--------------------------------------------------------------------------
    void Renderer::DeInitRender() {
        context.reset();
    }
}
