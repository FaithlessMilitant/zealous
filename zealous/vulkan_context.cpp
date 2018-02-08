#include "vulkan_context.hpp"

#include <SDL_video.h>

namespace zealous {
    //--------------------------------------------------------------------------
    VulkanContext::VulkanContext()
        : presentQueueFamilyIndex( -1 )
        , graphicsQueueFamilyIndex( -1 )
        , width ( 0 )
        , height( 0 ) {
    }

    //--------------------------------------------------------------------------
    VulkanContext::~VulkanContext() {
    }

    //--------------------------------------------------------------------------
    vk::SurfaceCapabilitiesKHR VulkanContext::SurfaceCapabilities( const vk::SurfaceKHR& surface ) const {
        return physicalDevice.getSurfaceCapabilitiesKHR( surface );
    }

    //--------------------------------------------------------------------------
    std::vector<vk::SurfaceFormatKHR> VulkanContext::SurfaceFormats( const vk::SurfaceKHR& surface ) const {
        return physicalDevice.getSurfaceFormatsKHR( surface );
    }
}
