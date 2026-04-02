#ifndef PHYSICSWORLD_HXX
#define PHYSICSWORLD_HXX

#include <vector>
#include "../../math/headers/vec3.hxx"

struct RigidBody;

/**
 * PhysicsWorld -- drives the simulation.
 *
 * Holds non-owning pointers to RigidBody objects.
 * Callers (main.cpp / scene) own the bodies and their lifetime.
 *
 * Each call to step():
 *   1. Integrate forces -> velocities -> positions (semi-implicit Euler)
 *   2. Detect all collisions (O(n^2) broadphase for now)
 *   3. Resolve each collision with impulse response + positional correction
 *   4. Clear accumulated forces ready for the next frame
 *
 * Usage:
 *   PhysicsWorld world;
 *   world.addBody(&sphereBody);
 *   world.addBody(&floorBody);
 *
 *   // each frame:
 *   world.step(deltaTime);
 *   sphereShape.setTransform(sphereBody.getTransformMatrix());
 */
class PhysicsWorld
{
public:
    PhysicsWorld() = default;

    // Non-copyable (pointers to external bodies would alias)
    PhysicsWorld(const PhysicsWorld &)            = delete;
    PhysicsWorld &operator=(const PhysicsWorld &) = delete;

    // ---- Body management ---------------------------------------------------

    void addBody(RigidBody *body);

    // Removes by pointer identity. O(n) scan. Safe to call during step only
    // if done from within a collision callback (not yet supported -- remove
    // between steps for now).
    void removeBody(RigidBody *body);

    // ---- Simulation --------------------------------------------------------

    /**
     * Advance the simulation by dt seconds.
     * Call once per frame with the frame's delta time.
     * Clamps dt to maxStepSeconds to prevent explosion on lag spikes.
     */
    void step(float dt);

    // ---- Configuration -----------------------------------------------------

    void  setGravity(Vector3f g)     { m_gravity = g;           }
    Vector3f getGravity()      const { return m_gravity;        }
    void  setMaxStep(float s)        { m_maxStep = s;           }
    int   getBodyCount()       const { return (int)m_bodies.size(); }

private:
    std::vector<RigidBody *> m_bodies;

    Vector3f m_gravity  = {0.0f, -9.81f, 0.0f};

    // Maximum dt passed to a single integration step.
    // Prevents instability on very long frames (e.g. after a breakpoint).
    float    m_maxStep  = 1.0f / 60.0f;

    void integrate(float dt);
    void detectAndResolve();
    void clearForces();
};

#endif