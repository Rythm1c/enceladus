#include "headers/physicsworld.hxx"
#include "headers/rigidbody.hxx"
#include "headers/collision.hxx"

#include <algorithm>
#include <cmath>

// =============================================================================
// Body management
// =============================================================================

void PhysicsWorld::addBody(RigidBody *body)
{
    if (body) m_bodies.push_back(body);
}

void PhysicsWorld::removeBody(RigidBody *body)
{
    m_bodies.erase(
        std::remove(m_bodies.begin(), m_bodies.end(), body),
        m_bodies.end());
}

// =============================================================================
// Step
// =============================================================================

void PhysicsWorld::step(float dt)
{
    // Clamp to prevent instability on lag spikes or after hitting a breakpoint
    if (dt > m_maxStep) dt = m_maxStep;
    if (dt <= 0.0f)     return;

    integrate(dt);
    detectAndResolve();
    clearForces();
}

// =============================================================================
// Private
// =============================================================================

void PhysicsWorld::integrate(float dt)
{
    for (RigidBody *body : m_bodies)
    {
        if (!body->hasFiniteMass()) continue;

        // ---- Linear integration (semi-implicit Euler) ----------------------
        //
        // Semi-implicit differs from explicit Euler in one key way:
        //   Explicit:      pos += oldVelocity * dt
        //   Semi-implicit: vel += accel * dt
        //                  pos += newVelocity * dt   <-- uses updated velocity
        //
        // This makes it symplectic (energy-conserving) which prevents the
        // slow energy gain that causes explicit Euler simulations to explode.

        // Gravity is a body force: F = m * g, so accel = g (mass cancels)
        Vector3f accel = m_gravity + body->forceAccum * body->invMass;

        body->linearVelocity += accel * dt;

        // Damping: multiply velocity by damping^dt rather than subtracting,
        // so the effect is frame-rate independent.
        // For small dt, pow(d, dt) ≈ 1 - (1-d)*dt which is the linear approx.
        body->linearVelocity  = body->linearVelocity  * std::pow(body->linearDamping,  dt);

        body->position += body->linearVelocity * dt;

        // ---- Angular integration -------------------------------------------

        // Angular acceleration = I^-1 * torque (diagonal inertia tensor)
        Vector3f angAccel(
            body->torqueAccum.x * body->invInertia.x,
            body->torqueAccum.y * body->invInertia.y,
            body->torqueAccum.z * body->invInertia.z);

        body->angularVelocity += angAccel * dt;
        body->angularVelocity  = body->angularVelocity * std::pow(body->angularDamping, dt);

        // Integrate orientation:
        //   dq/dt = 0.5 * omega_quat * q
        // where omega_quat = Quat(0, angularVelocity)
        // This is the standard quaternion derivative formula.
        Quat omegaQuat(
            body->angularVelocity.x,
            body->angularVelocity.y,
            body->angularVelocity.z,
            0.0f);

        // q_new = q + 0.5 * omegaQuat * q * dt
        Quat dq      = omegaQuat * body->orientation;
        body->orientation = body->orientation + (0.5f * dt) * dq;

        // Renormalise every step to prevent quaternion drift.
        // Drift accumulates because we're doing Euler integration on the
        // quaternion, which doesn't preserve the unit constraint exactly.
        body->orientation = body->orientation.unit();
    }
}

void PhysicsWorld::detectAndResolve()
{
    // Brute-force O(n^2) broadphase -- fine for small scenes.
    // Each pair tested exactly once (j starts at i+1).
    for (size_t i = 0; i < m_bodies.size(); ++i)
    {
        for (size_t j = i + 1; j < m_bodies.size(); ++j)
        {
            RigidBody &a = *m_bodies[i];
            RigidBody &b = *m_bodies[j];

            // Skip static-static pairs -- they can never collide meaningfully
            if (!a.hasFiniteMass() && !b.hasFiniteMass()) continue;

            CollisionManifold manifold = testCollision(a, b);
            if (manifold.hasCollision)
                resolveCollision(manifold);
        }
    }
}

void PhysicsWorld::clearForces()
{
    for (RigidBody *body : m_bodies)
    {
        body->forceAccum  = {0.0f, 0.0f, 0.0f};
        body->torqueAccum = {0.0f, 0.0f, 0.0f};
    }
}
