#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>

#include "../headers/core.hxx"
#include "../headers/renderpass.hxx"
#include "../headers/renderer.hxx"
#include "../headers/pipeline.hxx"
#include "../headers/swapchain.hxx"
#include "../headers/vertex.hxx"
#include "../headers/shape.hxx"

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

        const int window_width  = 800;
        const int window_height = 600;

        // Create SDL2 window
        SDL_Window *window = SDL_CreateWindow(
            "Enceladus - Vulkan | SDL2",
            100, 100,
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

        auto core       = std::make_unique<Core>(window);
        auto swapchain  = std::make_unique<Swapchain>(*core, window);
        auto renderPass = std::make_unique<RenderPass>(*core, swapchain->getFormat());

        // ---- Pipeline -------------------------------------------------------
        // Shaders are RAII — destroyed when they go out of scope after pipeline creation.
        Shader vertShader(*core, "build/shaders/shader.vert.spv");
        Shader fragShader(*core, "build/shaders/shader.frag.spv");

        // Push constants: a single vec2 offset pushed in the vertex stage.
        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset     = 0;
        pushRange.size       = sizeof(PushConstants2D);

        auto attribDescs = Vertex::getAttributeDescriptions();

        PipelineConfig pipelineConfig{
            .core                  = *core,
            .renderPass            = renderPass->getHandle(),
            .swapChainExtent       = swapchain->getExtent(),
            .vertShader            = &vertShader,
            .fragShader            = &fragShader,
            .bindingDescriptions   = {Vertex::getBindingDescription()},
            .attributeDescriptions = {attribDescs.begin(), attribDescs.end()},
            .pushConstantRanges    = {pushRange},
        };
        auto pipeline = std::make_unique<Pipeline>(pipelineConfig);

        // ---- Renderer -------------------------------------------------------
        RendererConfig rendererConfig{
            .core                = *core,
            .renderPass          = renderPass->getHandle(),
            .swapChainExtent     = swapchain->getExtent(),
            .swapChainImageViews = swapchain->getImageViews(),
        };
        auto renderer = std::make_unique<Renderer>(rendererConfig);

        Triangle triangle(*core, {0.0f, 0.0f}, 0.5f);
        triangle.upload();

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

                uint32_t frameIndex = renderer->getFrame(
                    swapchain->getHandle(),
                    swapchain->getExtent());

                renderer->beginRecording(
                    renderPass->getHandle(),
                    frameIndex,
                    swapchain->getExtent());

                renderer->bindPipeline(*pipeline);
                renderer->drawShape(triangle, *pipeline);

                renderer->endRecording();
                renderer->presentFrame(swapchain->getHandle(), frameIndex);

                // renderer->presentFrame(renderInfo);
            }
        }

        vkDeviceWaitIdle(core->getDevice());


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
