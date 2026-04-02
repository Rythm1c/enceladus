#include "headers/scene.hxx"

#include <SDL2/SDL.h>
#include "../scene/headers/camera.hxx"
#include "../scene/headers/shape.hxx"
#include "../physics/headers/physicsworld.hxx"
#include "../physics/headers/rigidbody.hxx"
#include "../physics/headers/collider.hxx"
#include "../renderer/headers/core.hxx"
#include "../renderer/headers/drawable.hxx"

Scene::Scene(Core &core, int windowWidth, int windowHeight)
    : m_core(core)
{
    initializeCamera(windowWidth, windowHeight);
    initializePhysics();
    initializeLight();
}

Scene::~Scene() = default;

void Scene::initializeCamera(int windowWidth, int windowHeight)
{
    m_camera = std::make_unique<Camera>(
        Vector3f(0.0f, 0.0f, 3.0f),    // position: 3 units back
        -90.0f,                         // yaw: looking toward -Z
        0.0f,                           // pitch: level
        Vector3f(0.0f, 1.0f, 0.0f),     // world up
        45.0f,
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight));
}

void Scene::initializePhysics()
{
    m_physicsWorld = std::make_unique<PhysicsWorld>();
}

void Scene::initializeLight()
{
    m_light.direction = {0.6f, -1.0f, 0.4f, 0.0f};  // angled sun
    m_light.color     = {1.0f,  0.95f, 0.85f, 1.0f}; // warm white
    m_light.ambient   = {0.1f,  0.1f,  0.15f, 0.0f}; // cool ambient
}

void Scene::addFloor(float width, float height)
{
    // Floor (static plane)
    auto floorBody = std::make_unique<RigidBody>();
    floorBody->collider = Collider::makePlane(Vector3f(0.0f, 1.0f, 0.0f), -1.5f);
    floorBody->makeStatic();
    m_physicsWorld->addBody(floorBody.get());

    // Floor rendering shape
    auto floor = std::make_unique<Plane>(m_core, width, Vector3f(0.35f, 0.4f, 0.35f), height);
    floor->setPosition({0.0f, -1.5f, 0.0f});
    floor->upload();
    
    m_shapes.push_back(std::move(floor));
    m_rigidBodies.push_back(std::move(floorBody));
    m_drawablesDirty = true;
}

void Scene::addSphere(float x, float y, float z, float radius, float mass)
{
    // Physics body
    auto sphereBody = std::make_unique<RigidBody>();
    sphereBody->position   = Vector3f(x, y, z);
    sphereBody->collider   = Collider::makeSphere(radius);
    sphereBody->restitution = 0.6f;
    sphereBody->friction    = 0.4f;
    sphereBody->setMass(mass);
    
    RigidBody *bodyPtr = sphereBody.get();
    m_physicsWorld->addBody(bodyPtr);

    // Rendering sphere
    auto sphere = std::make_unique<Icosphere>(m_core, radius, 3);
    sphere->setPosition({x, y, z});
    sphere->upload();
    
    m_shapes.push_back(std::move(sphere));
    m_rigidBodies.push_back(std::move(sphereBody));
    m_drawablesDirty = true;
}

void Scene::addBox(float x, float y, float z, float halfX, float halfY, float halfZ, float mass)
{
    // Physics body
    auto boxBody = std::make_unique<RigidBody>();
    boxBody->position    = Vector3f(x, y, z);
    boxBody->collider    = Collider::makeBox(Vector3f(halfX, halfY, halfZ));
    boxBody->restitution = 0.3f;
    boxBody->friction    = 0.6f;
    boxBody->setMass(mass);
    
    RigidBody *bodyPtr = boxBody.get();
    m_physicsWorld->addBody(bodyPtr);

    // Rendering cube
    auto cube = std::make_unique<Cube>(m_core);
    cube->setPosition({x, y, z});
    cube->upload();
    
    m_shapes.push_back(std::move(cube));
    m_rigidBodies.push_back(std::move(boxBody));
    m_drawablesDirty = true;
}

void Scene::update(float deltaTime)
{
    // Update physics
    // m_physicsWorld->step(deltaTime);
    
    // Sync render state with physics bodies
    syncRenderables();
}

void Scene::handleInput(float deltaTime, const uint8_t *keys, int mouseX, int mouseY)
{
    if (keys)
    {
        if (keys[SDL_SCANCODE_W])
            m_camera->processKeyboard(CameraMovement::Forward, deltaTime);
        if (keys[SDL_SCANCODE_S])
            m_camera->processKeyboard(CameraMovement::Backward, deltaTime);
        if (keys[SDL_SCANCODE_A])
            m_camera->processKeyboard(CameraMovement::Left, deltaTime);
        if (keys[SDL_SCANCODE_D])
            m_camera->processKeyboard(CameraMovement::Right, deltaTime);
        if (keys[SDL_SCANCODE_E])
            m_camera->processKeyboard(CameraMovement::Up, deltaTime);
        if (keys[SDL_SCANCODE_Q])
            m_camera->processKeyboard(CameraMovement::Down, deltaTime);
    }

    if (mouseX != 0 || mouseY != 0)
    {
        m_camera->processMouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
    }
}

void Scene::syncRenderables()
{
    // Sync physics bodies with rendering shapes
    // Match each physics body with its corresponding shape by index
    // Shapes are added in the same order as their corresponding physics bodies (after floor)
    
    int shapeIdx = 0;  // Start from first shape
    
    // Skip plane shape if it exists (floor is static, doesn't need syncing)
    if (shapeIdx < m_shapes.size() && m_rigidBodies.size() > 0)
    {
        // Check if first body is static (plane)
        if (m_rigidBodies[0]->collider.type == ColliderType::Plane)
        {
            shapeIdx++;  // Skip the floor shape
        }
    }

    // Sync dynamic bodies with shapes
    for (size_t bodyIdx = 0; bodyIdx < m_rigidBodies.size() && shapeIdx < m_shapes.size(); ++bodyIdx)
    {
        const auto &body = m_rigidBodies[bodyIdx];
        if (body->collider.type != ColliderType::Plane)  // Skip static plane
        {
            m_shapes[shapeIdx]->setPosition(body->position);
            m_shapes[shapeIdx]->setRotation(body->orientation);
            shapeIdx++;
        }
    }

    m_drawablesDirty = true;
}

const std::vector<Drawable> &Scene::getDrawables() const
{
    if (m_drawablesDirty)
    {
        rebuildDrawables();
        m_drawablesDirty = false;
    }
    return m_cachedDrawables;
}

void Scene::rebuildDrawables() const
{
    m_cachedDrawables.clear();

    // Add all shapes in order
    for (const auto &shape : m_shapes)
    {
        m_cachedDrawables.push_back(shape->getDrawData());
    }
}
