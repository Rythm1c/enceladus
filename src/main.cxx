#include <iostream>
#include "headers/application.hxx"
#include "headers/scene.hxx"

int main(int argc, char *argv[])
{
    try
    {
        const int WINDOW_WIDTH  = 800;
        const int WINDOW_HEIGHT = 600;

        // Initialize application (Vulkan, SDL, rendering pipeline)
        Application app("Enceladus - Vulkan | SDL2", WINDOW_WIDTH, WINDOW_HEIGHT);

        // Create and setup scene (explicit scope so it's destroyed before app)
        {
            Scene scene(app.getCore(), WINDOW_WIDTH, WINDOW_HEIGHT);

            // Add game objects
            scene.addFloor(10.0f);
            scene.addSphere(1.0f, 2.0f, -1.0f, 0.6f, 1.0f);
            scene.addBox(3.0f, 3.0f, -2.0f, 0.5f, 0.5f, 0.5f, 2.0f);
            scene.addBox(3.0f, 5.0f, -2.0f, 0.5f, 0.5f, 0.5f, 2.0f);

            // Run the main loop
            app.run(scene);
        }
        // scene is now destroyed, releasing all buffers
        // app will be destroyed next, destroying command buffers

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
}
