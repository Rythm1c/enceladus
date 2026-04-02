#ifndef APPLICATION_HXX
#define APPLICATION_HXX

#include <memory>
#include <string>

struct SDL_Window;
class Core;
class Swapchain;
class RenderPass;
class Renderer;
class Pipeline;
class Descriptor;
class ShadowMap;
class Scene;

/**
 * Application -- manages Vulkan/SDL initialization, rendering pipeline,
 * and the main event loop.
 * 
 * Owns all graphics infrastructure and orchestrates rendering.
 */
class Application
{
public:
    /**
     * Initialize the application with window, Vulkan, and rendering pipeline.
     * 
     * @param title    Window title
     * @param width    Window width in pixels
     * @param height   Window height in pixels
     */
    Application(const std::string &title, int width, int height);

    // Non-copyable, but movable
    Application(const Application &)            = delete;
    Application &operator=(const Application &) = delete;
    Application(Application &&)                 = default;
    Application &operator=(Application &&)      = default;

    ~Application();

    /**
     * Run the main game loop with the given scene.
     * Handles rendering, timing, and input until the user quits.
     * 
     * @param scene The scene to render and update
     */
    void run(Scene &scene);

    // Accessors
    int   getWidth()  const { return m_width;  }
    int   getHeight() const { return m_height; }
    Core &getCore()         { return *m_core; }

private:
    // Window/System
    SDL_Window *m_window = nullptr;
    int         m_width;
    int         m_height;
    bool        m_windowResized = false;

    // Graphics pipeline (initialized in order)
    std::unique_ptr<Core>       m_core;
    std::unique_ptr<Swapchain>  m_swapchain;
    std::unique_ptr<RenderPass> m_renderPass;
    std::unique_ptr<ShadowMap>  m_shadowMap;
    std::unique_ptr<Descriptor> m_descriptor;
    std::unique_ptr<Pipeline>   m_pipeline;
    std::unique_ptr<Renderer>   m_renderer;

    // Initialization helpers
    void initializeSDL(const std::string &title);
    void initializeVulkan();
    void shutdownSDL();
    void shutdownVulkan();

    // Main loop helpers
    bool pollEvents();
    void handleInput(Scene &scene, float deltaTime);
    void updateScene(Scene &scene, float deltaTime);
    void renderFrame(Scene &scene);
    void recreateSwapchain();
};

#endif
