#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>

#include "../headers/core.hxx"
#include "../headers/pipeline.hxx"

int main(int argc, char *argv[])
{

    try
    {

        // Initialize SDL2
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << std::endl;
            return 1;
        }

        int window_width = 800;
        int window_height = 600;

        // Create SDL2 window
        SDL_Window *window = SDL_CreateWindow(
            "Enceladus-Vulkan||SDL2",
            100,
            100,
            window_width,
            window_height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

        if (!window)
        {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        std::cout << "Window created: " << window_width << "x" << window_height << std::endl;

        Core core(window);
        Pipeline pipeline(core.getDevice());

        // Main loop
        bool running = true;
        SDL_Event event;

        while (running)
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    std::cout << "Quit event received, exiting main loop." << std::endl;
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        running = false;
                    }
                    break;
                }
            }

            {
                // TODO: Render Vulkan frame here
            }
        }

        // Cleanup

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    catch (const std::exception &e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
