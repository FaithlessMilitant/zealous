#include "app.hpp"
#include "vulkan_helpers.hpp"
#include "vulkan_render.hpp"

#include <SDL.h>

namespace zealous {
    //--------------------------------------------------------------------------
    App::App()
        : running( false )
        , window ( nullptr ) {
    }

    //--------------------------------------------------------------------------
    App::~App() {
    }

    //--------------------------------------------------------------------------
    void App::Init() {
        // SDL
        SDL_Init( SDL_INIT_VIDEO );
        window = SDL_CreateWindow( "LOL", 50, 50, 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE );

        // Vulkan
        vulkanContext = std::make_unique<VulkanContext>();
        vulkanContext->SetSDLWindow( window );
        InitVulkan( *vulkanContext );

        // Rendering
        renderer.InitRender( vulkanContext );
    }

    //--------------------------------------------------------------------------
    void App::DeInit() {
        // Rendering
        renderer.DeInitRender();

        // Vulkan
        DeInitVulkan( *vulkanContext );

        SDL_Quit();
    }

    //--------------------------------------------------------------------------
    void App::Run() {
        running = true;
        Init();
        while ( OneTick(), running );
        assert( !running );
        DeInit();
    }

    //--------------------------------------------------------------------------
    void App::Render() {
        if ( MustUpdateVulkan( *vulkanContext ) )
            UpdateVulkan( *vulkanContext );
        renderer.RenderOnce();
    }

    //--------------------------------------------------------------------------
    void App::OneTick() {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) ) {
            if ( event.type == SDL_QUIT )
                running = false;
        }
        Render();
    }
}
