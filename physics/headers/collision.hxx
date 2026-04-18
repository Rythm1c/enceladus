#ifndef COLLISION_HXX
#define COLLISION_CXX

#include "../../math/headers/vec3.hxx"

struct RigidBody;

// =============================================================================
// ContactPoint -- one point of contact between two bodies.
// =============================================================================

struct Contact
{
    bool       hasCollision = false;

    Vector3f   pointAWorldSpace;
    Vector3f   pointBWorldSpace;

    Vector3f   pointALocalSpace;
    Vector3f   pointBLocalSpace;

    float      penetrationDepth = 0.0f; // how much the bodies overlap at this contact
    float      timeOfImpact     = 0.0f; // for continuous collision detection (not implemented yet)

    RigidBody *bodyA = nullptr;
    RigidBody *bodyB = nullptr;

    Vector3f   normal; // from A to B

};

// =============================================================================
// Collision detection functions
// Each returns a CollisionManifold. Check .hasCollision before using.
// =============================================================================

Contact testSphereSphere(RigidBody &a,      RigidBody &b);
Contact testBoxBox      (RigidBody &a,      RigidBody &b);
Contact testSpherePlane (RigidBody &sphere, RigidBody &plane);
Contact testBoxPlane    (RigidBody &box,    RigidBody &plane);
Contact testBoxSphere   (RigidBody &box,    RigidBody &sphere);

Contact testCollision(RigidBody &a, RigidBody &b);

// =============================================================================
// Collision resolution
// =============================================================================

void resolveCollision(Contact &contact);

#endif