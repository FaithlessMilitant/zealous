#pragma once

#include "vulkan_context.hpp"
#include "vulkan_render.hpp"

//--------------------------------------------------------------------------
// Forward declares
//--------------------------------------------------------------------------
struct SDL_Window;

namespace zealous {
    //--------------------------------------------------------------------------
    class App {
      public:
        App();
        ~App();

        void Init();
        void DeInit();

        void Run();
        void OneTick();
        void Render();

      private:
        bool running;
        std::shared_ptr<VulkanContext> vulkanContext;
        Renderer renderer;
        SDL_Window* window;
    };
}
