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
        if (body->isSleeping)       continue; // sleeping bodies skip integration

        // ---- Linear integration (semi-implicit Euler) ----------------------
        Vector3f accel = m_gravity + body->forceAccum * body->invMass;
        body->linearVelocity += accel * dt;
        body->linearVelocity  = body->linearVelocity * std::pow(body->linearDamping, dt);
        body->position        = body->position + body->linearVelocity * dt;

        /* // ---- Angular integration -------------------------------------------
        Vector3f angAccel = body->getWorldInvInertia() * body->torqueAccum;
        body->angularVelocity += angAccel * dt;
        body->angularVelocity  = body->angularVelocity * std::pow(body->angularDamping, dt);

        // Quaternion integration: dq/dt = 0.5 * omega_quat * q
        Quat omegaQuat(
            body->angularVelocity.x,
            body->angularVelocity.y,
            body->angularVelocity.z,
            0.0f);
        Quat dq           = omegaQuat * body->orientation;
        body->orientation = body->orientation + (0.5f * dt) * dq;
        body->orientation = body->orientation.unit(); // renormalise to prevent drift */

        // ---- Sleep test ------------------------------------------------
        float linearSpeed  = body->linearVelocity.mag();
        float angularSpeed = body->angularVelocity.mag();

        if (linearSpeed < RigidBody::SLEEP_VELOCITY &&
            angularSpeed < RigidBody::SLEEP_VELOCITY)
        {
            body->sleepTimer += dt;
            if (body->sleepTimer >= RigidBody::SLEEP_TIME)
            {
                // Zero out residual velocity to prevent micro-movement on wake
                body->linearVelocity  = {0.0f, 0.0f, 0.0f};
                body->angularVelocity = {0.0f, 0.0f, 0.0f};
                body->isSleeping      = true;
            }
        }
        else
        {
            body->sleepTimer = 0.0f;
        }
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

            // Skip static-static (never collide meaningfully)
            if (!a.hasFiniteMass() && !b.hasFiniteMass()) continue;

            // Skip sleeping-sleeping pairs UNLESS one is static
            // (static bodies wake sleeping ones when another dynamic body hits them)
            if (a.isSleeping && b.isSleeping) continue;

            Contact manifold = testCollision(a, b);
            if (manifold.hasCollision)
            {
                // Wake sleeping bodies that get hit
                if (a.isSleeping) a.wake();
                if (b.isSleeping) b.wake();

                resolveCollision(manifold);
            }
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
