#pragma once

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace zealous {
    //--------------------------------------------------------------------------
    class VulkanContext {
      public:
        VulkanContext();
        ~VulkanContext();

        SDL_Window* SDLWindow() { return window; }
        const SDL_Window* SDLWindow() const { return window; }
        int WindowWidth() const { return width; }
        int WindowHeight() const { return height; }

        const vk::Instance& Instance() const { return instance; }
        const vk::SurfaceKHR& WindowSurface() const { return windowSurface; }
        const vk::PhysicalDevice& PhysicalDevice() const { return physicalDevice; }
        const vk::PhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const { return physicalDeviceMemoryProperties; }
        const vk::Device& Device() const { return device; }
        const vk::Queue& PresentQueue() const { return presentQueue; }
        const vk::Queue& GraphicsQueue() const { return graphicsQueue; }
        const vk::Semaphore& ImageAvailableSemaphore() const { return imageAvailableSemaphore; }
        const vk::Semaphore& DoneRenderingSemaphore() const { return doneRenderingSemaphore; }
        const vk::SwapchainKHR& Swapchain() const { return swapchain; }
        const std::vector<vk::Image>& SwapchainImages() const { return swapchainImages; }
        const vk::CommandPool& CommandPool() const { return commandPool; }
        const std::vector<vk::CommandBuffer>& CommandBuffers() const { return commandBuffers; }
        const std::vector<vk::Fence>& Fences() const { return fences; }

        vk::SurfaceCapabilitiesKHR SurfaceCapabilities( const vk::SurfaceKHR& surface ) const;
        std::vector<vk::SurfaceFormatKHR> SurfaceFormats( const vk::SurfaceKHR& surface ) const;

        uint32_t PresentQueueFamilyIndex() const { return presentQueueFamilyIndex; }
        uint32_t GraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }

        const vk::DebugReportCallbackEXT& DebugReportCallback() const { return debugReportCallback; }

        void SetSDLWindow( SDL_Window* sdlWindow ) { window = sdlWindow; }
        void SetWindowWidth( int width ) { this->width = width; }
        void SetWindowHeight( int height ) { this->height = height; }

        void SetInstance( const vk::Instance& instance ) { this->instance = instance; }
        void SetWindowSurface( const vk::SurfaceKHR& windowSurface ) { this->windowSurface = windowSurface; }
        void SetPresentQueueFamilyIndex( uint32_t familyIndex ) { this->presentQueueFamilyIndex = familyIndex; }
        void SetGraphicsQueueFamilyIndex( uint32_t familyIndex ) { this->graphicsQueueFamilyIndex = familyIndex; }
        void SetPhysicalDevice( const vk::PhysicalDevice& physicalDevice ) { this->physicalDevice = physicalDevice; }
        void SetPhysicalDeviceMemoryProperties( const vk::PhysicalDeviceMemoryProperties& memoryProperties ) { this->physicalDeviceMemoryProperties = memoryProperties; }
        void SetDevice( const vk::Device& device ) { this->device = device; }
        void SetPresentQueue( const vk::Queue& queue ) { this->presentQueue = queue; }
        void SetGraphicsQueue( const vk::Queue& queue ) { this->graphicsQueue = queue; }
        void SetImageAvailableSemaphore( const vk::Semaphore& semaphore ) { this->imageAvailableSemaphore = semaphore; }
        void SetDoneRenderingSemaphore( const vk::Semaphore& semaphore ) { this->doneRenderingSemaphore = semaphore; }
        void SetSwapchain( const vk::SwapchainKHR& swapchain ) { this->swapchain = swapchain; }
        void SetSwapchainImages( std::vector<vk::Image>&& swapchainImages ) { this->swapchainImages = swapchainImages; }
        void SetCommandPool( const vk::CommandPool& pool ) { this->commandPool = pool; }
        void SetCommandBuffers( std::vector<vk::CommandBuffer>&& commandBuffers ) { this->commandBuffers = commandBuffers; }
        void SetFences( std::vector<vk::Fence>&& fences ) { this->fences = fences; }

        void SetDebugReportCallback( const vk::DebugReportCallbackEXT& callback ) { this->debugReportCallback = callback; }

      private:
        vk::Instance instance;
        vk::SurfaceKHR windowSurface;
        vk::PhysicalDevice physicalDevice;
        vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vk::Device device;
        vk::Semaphore imageAvailableSemaphore;
        vk::Semaphore doneRenderingSemaphore;
        uint32_t presentQueueFamilyIndex;
        uint32_t graphicsQueueFamilyIndex;
        vk::Queue presentQueue;
        vk::Queue graphicsQueue;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        vk::CommandPool commandPool;
        std::vector<vk::CommandBuffer> commandBuffers;
        std::vector<vk::Fence> fences;
        int width;
        int height;

        vk::DebugReportCallbackEXT debugReportCallback;

        SDL_Window* window;
    };
}
