#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>

#include "../headers/core.hxx"
#include "../headers/renderpass.hxx"
#include "../headers/renderer.hxx"
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

        std::unique_ptr<Core> core = std::make_unique<Core>(window);
        std::unique_ptr<RenderPass> renderPass = std::make_unique<RenderPass>(
            core->getDevice(),
            core->getSwapChainImageFormat());

        PipelineConfig pipelineConfig{
            .device = core->getDevice(),
            .renderPass = renderPass->getHandle(),
            .swapChainExtent = core->getSwapChainExtent(),
            .vertShader = new Shader(core->getDevice(), "build/shaders/shader.vert.spv"),
            .fragShader = new Shader(core->getDevice(), "build/shaders/shader.frag.spv")};

        std::unique_ptr<Pipeline> pipeline = std::make_unique<Pipeline>(pipelineConfig);
        pipelineConfig.vertShader->clean(core->getDevice());
        pipelineConfig.fragShader->clean(core->getDevice());

        RendererConfig rendererConfig{
            .device = core->getDevice(),
            .renderPass = renderPass->getHandle(),
            .swapChainImageViews = core->getSwapChainImageViews(),
            .swapChainExtent = core->getSwapChainExtent(),
            .graphicsQueueFamilyIndex = core->getGraphicsFamilyIndex()};
        std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>(
            rendererConfig);

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

            { // Render a frame

                RenderInfo renderInfo{
                    .device = core->getDevice(),
                    .swapchain = core->getSwapChain(),
                    .swapChainExtent = core->getSwapChainExtent(),
                    .renderPass = renderPass->getHandle(),
                    .pipeline = pipeline->getHandle(),
                    .graphicsQueue = core->getGraphicsQueue(),
                    .presentQueue = core->getPresentQueue()};

                renderer->beginFrame(renderInfo);

                vkCmdDraw(renderer->getCommandBuffer(), 3, 1, 0, 0);

                renderer->presentFrame(renderInfo);
            }
        }

        // Cleanup
        // renderer->clean(core->getDevice());
        pipeline->clean(core->getDevice());
        renderPass->clean(core->getDevice());
        core->clean();

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
