#ifndef COLLISION_HXX
#define COLLISION_CXX

#include "../../math/headers/vec3.hxx"

struct RigidBody;

/**
 * CollisionManifold -- result of a collision test between two bodies.
 *
 * If hasCollision is false all other fields are undefined.
 *
 * Convention:
 *   normal always points FROM bodyB TOWARD bodyA.
 *   Resolving the collision pushes bodyA in the +normal direction
 *   and bodyB in the -normal direction.
 */
struct CollisionManifold
{
    bool hasCollision = false;

    Vector3f normal; // unit vector form a to b
    float    depth = 0.0f; // penetration depth (positive = overlapping)
    Vector3f contactPoint; // world-space point of deepest contact

    RigidBody *bodyA = nullptr;
    RigidBody *bodyB = nullptr;
};

// =============================================================================
// Collision detection functions
// Each returns a CollisionManifold. Check .hasCollision before using.
// =============================================================================

CollisionManifold testSphereSphere(RigidBody &a,      RigidBody &b);
CollisionManifold testBoxBox      (RigidBody &a,      RigidBody &b);
CollisionManifold testSpherePlane (RigidBody &sphere, RigidBody &plane);
CollisionManifold testBoxPlane    (RigidBody &box,    RigidBody &plane);
CollisionManifold testBoxSphere   (RigidBody &box,    RigidBody &sphere);



/**
 * Dispatch: test any two bodies against each other.
 * Automatically selects the correct test based on collider types.
 * Returns an empty manifold (hasCollision=false) for unsupported pairs
 */
CollisionManifold testCollision(RigidBody &a, RigidBody &b);

// =============================================================================
// Collision resolution
// =============================================================================

/**
 * Resolve a detected collision using impulse-based response.
 *
 * Applies:
 *   1. Normal impulse   -- separates overlapping bodies, respects restitution
 *   2. Friction impulse -- opposes tangential relative motion
 *   3. Position correction -- pushes bodies apart to remove residual penetration
 *                             (Baumgarte stabilisation, scaled to avoid jitter)
 */
void resolveCollision(CollisionManifold &manifold);

#endif