#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>

#include "../headers/core.hxx"
#include "../headers/renderpass.hxx"
#include "../headers/renderer.hxx"
#include "../headers/descriptor.hxx"
#include "../headers/pipeline.hxx"
#include "../headers/swapchain.hxx"
#include "../headers/vertex.hxx"
#include "../headers/shape.hxx"
#include "../headers/camera.hxx"

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
        pushRange.size       = sizeof(ModelPushConstants);

        auto attribDescs = Vertex::getAttributeDescriptions();

        auto descriptor = std::make_unique<Descriptor>(*core, MAX_FRAMES_IN_FLIGHT);

        PipelineConfig pipelineConfig{
            .core                  = *core,
            .renderPass            = renderPass->getHandle(),
            .swapChainExtent       = swapchain->getExtent(),
            .vertShader            = &vertShader,
            .fragShader            = &fragShader,
            .descriptorSetLayout   = descriptor->getLayout(),
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
            .descriptor          = *descriptor,
        };
        auto renderer = std::make_unique<Renderer>(rendererConfig);

        // ---- Camera ---------------------------------------------------------
        Camera camera(
            Vector3f{ 0.0f,  0.0f, 3.0f},   // position: 3 units back
            Vector3f{ 0.0f,  0.0f, 1.0f},   // target direction: towards -Z
            Vector3f{ 0.0f, -1.0f, 0.0f},   // up:       Y-up (Vulkan corrected)
            45.0f,
            static_cast<float>(window_width) / static_cast<float>(window_height),
            0.1f,   // near plane
            100.0f  // far plane
        );

        Triangle triangle(*core);
        triangle.upload();
        triangle.setScale({1.5f, 1.5f, 1.0f});
        triangle.setRotation(360.0f, {0.0f, 0.0f, 1.0f});
        //triangle.setRotation(180, {0.0, 1.0, 0.0});

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
                renderer->clearColor(0.2, 0.3, 0.6);

                uint32_t frameIndex = renderer->getFrame(
                    swapchain->getHandle(),
                    swapchain->getExtent());

                renderer->beginRecording(
                    renderPass->getHandle(),
                    frameIndex,
                    swapchain->getExtent());

                renderer->bindPipeline(*pipeline);

                renderer->bindDescriptors(camera.getUBO(), pipeline->getLayout());

                renderer->drawShape(triangle, *pipeline);

                renderer->endRecording();
                renderer->presentFrame(swapchain->getHandle(), frameIndex);

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
