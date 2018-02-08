#pragma once
#include "vulkan_context.hpp"

namespace zealous {
    class Renderer {
      public:
        void InitRender( std::shared_ptr<VulkanContext> context );
        void RenderOnce();
        void DeInitRender();

      private:
        std::shared_ptr<VulkanContext> context;
        vk::DeviceMemory deviceMemory;
        vk::Buffer buffer;
    };
}
