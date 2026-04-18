#ifndef SCENE_HXX
#define SCENE_HXX

#include <vector>
#include <memory>
#include <cstdint>

#include "../../renderer/headers/ubo.hxx"
#include "../../renderer/headers/material.hxx"

class  Camera;
class  PhysicsWorld;
class  Core;
class  Shape;
struct RigidBody;

/**
 * Scene -- manages game objects, physics, camera, and lighting.
 * 
 * Encapsulates all game logic and entities.
 */
class Scene
{
public:
    Scene(Core &core, int windowWidth, int windowHeight);

    // Non-copyable, non-movable
    Scene(const Scene &)            = delete;
    Scene &operator=(const Scene &) = delete;

    ~Scene();

    /**
     * Update the scene (physics, objects, etc.) for the given delta time.
     */
    void update(float deltaTime, float aspect);

    /**
     * Handle camera input (mouse and keyboard).
     */
    void handleInput(float deltaTime, const uint8_t *keys, int mouseX, int mouseY);

    // ---- Accessors for rendering ----------------------------------------

    const Camera &getCamera() const { return *m_camera; }
    
    const LightUBO &getLight() const { return m_light; }
    
    // Get all drawable objects
    const std::vector<struct Drawable> &getDrawables() const;

    // ---- Physics --------------------------------------------------------
    PhysicsWorld &getPhysicsWorld() { return *m_physicsWorld; }

    // ---- Scene setup (called before run()) --------- -----
    void addFloor(float width, float height = 10.0f);
    void addCubeSphere(float x, float y, float z, float radius = 0.6f, int subDivisions = 3,const Material &material = Material::rubber({1.0f, 1.0f, 1.0f}), float mass = 1.0f, bool isStatic = false);
    void addIcoSphere(float x, float y, float z, float radius = 0.6f, int subDivisions = 3, const Material &material = Material::stone({1.0f, 0.2f, 0.1f}), float mass = 1.0f, bool isStatic = false);
    void addBox(float x, float y, float z, float halfX = 0.5f, float halfY = 0.5f, float halfZ = 0.5f, float mass = 2.0f);

private:
    Core &m_core;

    // Core components
    std::unique_ptr<Camera>         m_camera;
    std::unique_ptr<PhysicsWorld>   m_physicsWorld;

    // Game objects (physics bodies)
    std::vector<std::unique_ptr<RigidBody>> m_rigidBodies;

    // Rendering shapes (all derived from Shape base class)
    std::vector<std::unique_ptr<Shape>>     m_shapes;

    // Cached drawables
    mutable std::vector<struct Drawable>    m_cachedDrawables;
    mutable bool                            m_drawablesDirty = true;

    // Lighting
    LightUBO m_light;

    // Helpers
    void initializeCamera(int windowWidth, int windowHeight);
    void initializePhysics();
    void initializeLight();
    void syncRenderables();
    void rebuildDrawables() const;
};

#endif
