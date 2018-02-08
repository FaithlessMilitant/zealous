#include "vulkan_helpers.hpp"

#include "container_helpers.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <SDL_vulkan.h>

namespace zealous {
    //--------------------------------------------------------------------------
    static bool sDisplayCallbacks = false;

    //--------------------------------------------------------------------------
    VkBool32 VulkanDebugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       pUserData
    ) {
        if ( sDisplayCallbacks ) {
            std::cerr << "Layer prefix : " << pLayerPrefix   << std::endl
                      << "Message :      " << pMessage       << std::endl
                      << std::endl;
        }

        return VK_FALSE;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void InitVulkanInstance( VulkanContext& context ) {
        // SDL offers a helper function to determine all necessary extensions, use that
        unsigned int extCount;
        SDL_Vulkan_GetInstanceExtensions( context.SDLWindow(), &extCount, nullptr );
        std::vector<const char*> desiredExts( extCount, nullptr );
        SDL_Vulkan_GetInstanceExtensions( context.SDLWindow(), &extCount, desiredExts.data() );
        if ( !Contains( desiredExts, "VK_EXT_debug_report" ) )
            desiredExts.push_back( "VK_EXT_debug_report" );

        const std::vector<const char*> validationLayers = {
            "VK_LAYER_LUNARG_standard_validation"
        };

        const vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo()
                .setEnabledExtensionCount( ( uint32_t )desiredExts.size() )
                .setPpEnabledExtensionNames( desiredExts.data() )
                .setEnabledLayerCount( ( uint32_t )validationLayers.size() )
                .setPpEnabledLayerNames( validationLayers.data() );

        context.SetInstance( vk::createInstance( instanceCreateInfo ) );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanInstance( VulkanContext& context ) {
        const vk::Instance& instance = context.Instance();
        instance.destroy();
        context.SetInstance( vk::Instance() );
    }

    //--------------------------------------------------------------------------
    void InitVulkanDebugLayer( VulkanContext& context ) {
        const vk::Instance& instance = context.Instance();

        vk::DebugReportCallbackCreateInfoEXT createInfo = vk::DebugReportCallbackCreateInfoEXT()
                .setFlags( vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning |
                           vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eError )
                .setPfnCallback( &VulkanDebugReportCallback );

        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
            ( PFN_vkCreateDebugReportCallbackEXT ) instance.getProcAddr( "vkCreateDebugReportCallbackEXT" );
        VkDebugReportCallbackEXT debugReportCallback_;
        VkDebugReportCallbackCreateInfoEXT createInfo_( createInfo );
        vkCreateDebugReportCallbackEXT( instance, &createInfo_, nullptr, &debugReportCallback_ );
        vk::DebugReportCallbackEXT debugReportCallback( debugReportCallback_ );

        context.SetDebugReportCallback( debugReportCallback );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanDebugLayer( VulkanContext& context ) {
        const vk::Instance& instance = context.Instance();

        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
            ( PFN_vkDestroyDebugReportCallbackEXT )instance.getProcAddr( "vkDestroyDebugReportCallbackEXT" );
        vkDestroyDebugReportCallbackEXT( instance, context.DebugReportCallback(), nullptr );
    }

    //--------------------------------------------------------------------------
    void InitVulkanSurface( VulkanContext& context ) {
        VkSurfaceKHR vkSurface;
        SDL_Vulkan_CreateSurface( context.SDLWindow(), context.Instance(), &vkSurface );
        vk::SurfaceKHR surface( vkSurface );
        context.SetWindowSurface( surface );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanSurface( VulkanContext& context ) {
        const vk::Instance& instance = context.Instance();
        instance.destroySurfaceKHR( context.WindowSurface() );
        context.SetWindowSurface( vk::SurfaceKHR() );
    }

    //--------------------------------------------------------------------------
    std::vector<const char*> DesiredDeviceExtensions() {
        return std::vector<const char*> { "VK_KHR_swapchain" };
    }

    //--------------------------------------------------------------------------
    void InitVulkanPhysicalDevice( VulkanContext& context ) {
        const vk::Instance& instance = context.Instance();
        // choosing a physical device among all
        std::vector<vk::PhysicalDevice> physDevs = instance.enumeratePhysicalDevices();

        // we are activating a subset of extensions and looking for a device
        // with all those extensions available
        const auto desiredExts = DesiredDeviceExtensions();

        uint32_t presentQueueFamilyIndex, graphicsQueueFamilyIndex;
        auto physDevIt = physDevs.begin();

        for ( physDevIt; physDevIt != physDevs.end(); ++physDevIt ) {
            // during the search, get the family indexes in charge of present and rendering
            presentQueueFamilyIndex = -1;
            graphicsQueueFamilyIndex = -1;

            const vk::PhysicalDevice& physicalDevice = *physDevIt;

            const std::vector<vk::QueueFamilyProperties> familyProps = physicalDevice.getQueueFamilyProperties();
            for ( uint32_t i = 0, end = ( uint32_t )familyProps.size(); i < end; ++i ) {
                const bool presentSupported = physicalDevice.getSurfaceSupportKHR( i, context.WindowSurface() );
                const bool graphicsSupported = ( bool )( familyProps[i].queueFlags & vk::QueueFlagBits::eGraphics );

                if ( presentSupported and graphicsSupported ) {
                    presentQueueFamilyIndex = i;
                    graphicsQueueFamilyIndex = i;
                    break;
                } else if ( graphicsSupported and graphicsQueueFamilyIndex == -1 ) {
                    graphicsQueueFamilyIndex = i;
                } else if ( presentSupported and presentQueueFamilyIndex == -1 ) {
                    presentQueueFamilyIndex = i;
                }
            }

            if ( presentQueueFamilyIndex != -1 and graphicsQueueFamilyIndex != -1 ) {
                std::vector<vk::ExtensionProperties> extProps = physicalDevice.enumerateDeviceExtensionProperties();

                auto func = [&extProps]( bool validSoFar, std::string extString ) {
                    if ( not validSoFar )
                        return false;
                    auto predicate = [extString]( const vk::ExtensionProperties & extension ) {
                        return extString == extension.extensionName;
                    };
                    return ContainsIf( extProps, predicate );
                };
                const bool acceptsAllExtensions = std::all_of( desiredExts.begin(), desiredExts.end(), [&extProps]( const std::string & desiredExtName ) {
                    return ContainsIf( extProps, [&desiredExtName]( const vk::ExtensionProperties & extProps ) {
                        return desiredExtName == extProps.extensionName;
                    } );
                } );

                if ( acceptsAllExtensions ) {
                    // success ! let's set our state
                    context.SetPhysicalDevice( physicalDevice );
                    context.SetGraphicsQueueFamilyIndex( graphicsQueueFamilyIndex );
                    context.SetPresentQueueFamilyIndex( graphicsQueueFamilyIndex );

                    // get the memory properties while we're at it
                    const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
                    context.SetPhysicalDeviceMemoryProperties( memoryProperties );
                }
            }
        }
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanPhysicalDevice( VulkanContext& context ) {
        // nothing special to do
        context.SetPhysicalDevice( vk::PhysicalDevice() );
    }

    //--------------------------------------------------------------------------
    void InitVulkanDevice( VulkanContext& context ) {
        const vk::Instance& instance = context.Instance();
        const vk::PhysicalDevice& physicalDevice = context.PhysicalDevice();

        const auto desiredExts = DesiredDeviceExtensions();

        const vk::DeviceQueueCreateInfo queueCreateInfo[1] = {
            vk::DeviceQueueCreateInfo()
            .setQueueCount( 1 )
            .setQueueFamilyIndex( context.GraphicsQueueFamilyIndex() )
        };

        vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
                                                .setEnabledExtensionCount( ( uint32_t )desiredExts.size() )
                                                .setPpEnabledExtensionNames( desiredExts.data() )
                                                .setQueueCreateInfoCount( 1 )
                                                .setPQueueCreateInfos( queueCreateInfo );
        const vk::Device& device = physicalDevice.createDevice( deviceCreateInfo );
        context.SetDevice( device );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanDevice( VulkanContext& context ) {
        context.Device().destroy();
        context.SetDevice( vk::Device() );
    }

    //--------------------------------------------------------------------------
    void InitVulkanQueues( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        vk::Queue graphicsQueue = device.getQueue( context.GraphicsQueueFamilyIndex(), 0 );
        context.SetGraphicsQueue( graphicsQueue );

        vk::Queue presentQueue = device.getQueue( context.PresentQueueFamilyIndex(), 0 );
        context.SetPresentQueue( graphicsQueue );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanQueues( VulkanContext& context ) {
        // nothing here
    }

    //--------------------------------------------------------------------------
    void InitVulkanSemaphores( VulkanContext& context ) {
        const vk::Device& device = context.Device();

        const vk::SemaphoreCreateInfo createInfo = vk::SemaphoreCreateInfo();
        context.SetImageAvailableSemaphore( device.createSemaphore( createInfo ) );
        context.SetDoneRenderingSemaphore( device.createSemaphore( createInfo ) );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanSemaphores( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        device.destroySemaphore( context.ImageAvailableSemaphore() );
        device.destroySemaphore( context.DoneRenderingSemaphore() );
    }

    //--------------------------------------------------------------------------
    void InitVulkanSwapchain( VulkanContext& context ) {
        const vk::SurfaceKHR& windowSurface = context.WindowSurface();

        // check whether the surface has the capabilities needed
        const vk::SurfaceCapabilitiesKHR caps = context.SurfaceCapabilities( windowSurface );
        assert( caps.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst );

        // image count ?
        const uint32_t imageCount = std::min( caps.maxImageCount, caps.minImageCount + 1 );

        // choose the most appropriate surface format
        std::vector<vk::SurfaceFormatKHR> formats = context.SurfaceFormats( windowSurface );
        assert( not formats.empty() );
        auto formatIt = std::find_if( formats.begin(), formats.end(), [] ( const vk::SurfaceFormatKHR & format ) {
            return format.format == vk::Format::eR8G8B8A8Unorm;
        } );
        vk::SurfaceFormatKHR format = ( formatIt != formats.end() ) ? *formatIt : formats[0];
        if ( format.format == vk::Format::eUndefined ) {
            format.format = vk::Format::eR8G8B8A8Unorm;
            format.colorSpace = vk::ColorSpaceKHR::eExtendedSrgbNonlinearEXT;
        }

        // get the extent of the rendering
        int w, h;
        SDL_Vulkan_GetDrawableSize( context.SDLWindow(), &w, &h );
        context.SetWindowWidth( w );
        context.SetWindowHeight( h );

        // create the swapchain
        const vk::Device& device = context.Device();
        const vk::SwapchainKHR oldSwapchain = context.Swapchain();

        const vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
                .setClipped( true )
                .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
                .setImageArrayLayers( 1 )
                .setImageColorSpace( format.colorSpace )
                .setImageExtent( vk::Extent2D( w, h ) )
                .setImageFormat( format.format )
                .setImageSharingMode( vk::SharingMode::eExclusive )
                .setImageUsage( vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment )
                .setMinImageCount( imageCount )
                .setPresentMode( vk::PresentModeKHR::eFifo )
                .setSurface( windowSurface )
                .setPreTransform( caps.currentTransform )
                .setOldSwapchain( oldSwapchain );
        const vk::SwapchainKHR& swapchain = device.createSwapchainKHR( createInfo );
        context.SetSwapchain( swapchain );

        if ( !!oldSwapchain )
            device.destroySwapchainKHR( oldSwapchain );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanSwapchain( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        const vk::SwapchainKHR& swapchain = context.Swapchain();
        device.destroySwapchainKHR( swapchain );
        context.SetSwapchain( vk::SwapchainKHR() );
    }

    //--------------------------------------------------------------------------
    void InitVulkanSwapchainImages( VulkanContext& context ) {
        const vk::SwapchainKHR& swapchain = context.Swapchain();
        const vk::Device& device = context.Device();

        std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR( swapchain );
        context.SetSwapchainImages( std::move( swapchainImages ) );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanSwapchainImages( VulkanContext& context ) {
        context.SetSwapchainImages( std::vector<vk::Image> {} );
    }

    //--------------------------------------------------------------------------
    void InitVulkanCommandPool( VulkanContext& context ) {
        const vk::Device& device = context.Device();

        vk::CommandPoolCreateInfo createInfo = vk::CommandPoolCreateInfo()
                                               .setQueueFamilyIndex( context.GraphicsQueueFamilyIndex() )
                                               .setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer );
        vk::CommandPool pool = device.createCommandPool( createInfo );

        context.SetCommandPool( pool );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanCommandPool( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        device.destroyCommandPool( context.CommandPool() );
    }

    //--------------------------------------------------------------------------
    void InitVulkanCommandBuffers( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        const vk::CommandPool& commandPool = context.CommandPool();

        vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
                .setCommandPool( commandPool )
                .setLevel( vk::CommandBufferLevel::ePrimary )
                .setCommandBufferCount( ( uint32_t )context.SwapchainImages().size() );
        std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers( allocInfo );

        context.SetCommandBuffers( std::move( commandBuffers ) );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanCommandBuffers( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        device.freeCommandBuffers( context.CommandPool(), vk::ArrayProxy<const vk::CommandBuffer>( context.CommandBuffers() ) );
        context.SetCommandBuffers( std::vector<vk::CommandBuffer> {} );
    }

    //--------------------------------------------------------------------------
    void InitVulkanFences( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        vk::FenceCreateInfo createInfo = vk::FenceCreateInfo()
                                         .setFlags( vk::FenceCreateFlagBits::eSignaled );
        const size_t fenceCount = context.SwapchainImages().size();
        std::vector<vk::Fence> fences{ fenceCount };
        for ( size_t i = 0 ; i < fenceCount ; ++i )
            fences[i] = device.createFence( createInfo );

        context.SetFences( std::move( fences ) );
    }

    //--------------------------------------------------------------------------
    void DeInitVulkanFences( VulkanContext& context ) {
        const vk::Device& device = context.Device();
        std::vector<vk::Fence> fences = context.Fences();
        context.SetFences( std::vector<vk::Fence> {} );

        device.waitForFences( vk::ArrayProxy<const vk::Fence>( fences ), true, std::numeric_limits<uint64_t>::max() );
        for ( auto fence : fences ) {
            device.destroyFence( fence );
        }
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void InitVulkan( VulkanContext& context ) {
        sDisplayCallbacks = true;
        InitVulkanInstance( context );
        InitVulkanDebugLayer( context );
        InitVulkanSurface( context );
        InitVulkanPhysicalDevice( context );
        InitVulkanDevice( context );
        InitVulkanQueues( context );
        InitVulkanSemaphores( context );
        InitVulkanSwapchain( context );
        InitVulkanSwapchainImages( context );
        InitVulkanCommandPool( context );
        InitVulkanCommandBuffers( context );
        InitVulkanFences( context );
        sDisplayCallbacks = false;
    }

    //--------------------------------------------------------------------------
    void DeInitVulkan( VulkanContext& context ) {
        sDisplayCallbacks = true;
        DeInitVulkanFences( context );
        DeInitVulkanCommandBuffers( context );
        DeInitVulkanCommandPool( context );
        DeInitVulkanSwapchainImages( context );
        DeInitVulkanSwapchain( context );
        DeInitVulkanSemaphores( context );
        DeInitVulkanQueues( context );
        DeInitVulkanDevice( context );
        DeInitVulkanPhysicalDevice( context );
        DeInitVulkanSurface( context );
        DeInitVulkanDebugLayer( context );
        DeInitVulkanInstance( context );
        sDisplayCallbacks = false;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool MustUpdateVulkan( VulkanContext& context ) {
        int w, h;
        SDL_Vulkan_GetDrawableSize( context.SDLWindow(), &w, &h );
        return ( context.WindowWidth() != w or context.WindowHeight() != h );
    }

    //--------------------------------------------------------------------------
    void UpdateVulkan( VulkanContext& context ) {
        sDisplayCallbacks = true;
        DeInitVulkanFences( context );
        DeInitVulkanCommandBuffers( context );
        DeInitVulkanCommandPool( context );
        DeInitVulkanSwapchainImages( context );
        //DeInitVulkanSwapchain( context );

        InitVulkanSwapchain( context );
        InitVulkanSwapchainImages( context );
        InitVulkanCommandPool( context );
        InitVulkanCommandBuffers( context );
        InitVulkanFences( context );
        sDisplayCallbacks = false;
    }
}
