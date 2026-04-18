#include "headers/collision.hxx"
#include "headers/rigidbody.hxx"

#include <cmath>
#include <cfloat>
#include <array>
#include <algorithm>

// =============================================================================
// Internal helpers
// =============================================================================

/**
 * Clamp a float between lo and hi.
 */
static float clampF(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

// =============================================================================
// Detection
// =============================================================================
//   Two spheres overlap when the distance between their centres is less than
//   the sum of their radii.
//   dist < rA + rB  =>  collision
Contact testSphereSphere(RigidBody &a, RigidBody &b)
{
    Contact c;
    c.bodyA = &a; c.bodyB = &b;

    const float rA =   a.collider.sphere.radius;
    const float rB =   b.collider.sphere.radius;
    const float rSum = rA + rB;

    // vector AB = AO +OB = AO - BO
    Vector3f delta = a.position - b.position;
    float distSq   = dot(delta, delta);
    if (distSq >= rSum * rSum) return c;

    float dist     = std::sqrt(distSq);
    c.hasCollision = true;
    c.normal = (dist > 1e-6f) ? delta * (1.0f / dist) : Vector3f(0.0f, 1.0f, 0.0f);

    c.pointAWorldSpace = a.position - c.normal * rA;
    c.pointBWorldSpace = b.position + c.normal * rB;

    c.pointALocalSpace = c.pointAWorldSpace - a.position;
    c.pointBLocalSpace = c.pointBWorldSpace - b.position;

    c.penetrationDepth = rSum - dist;
    
    return c;
}

Contact testSpherePlane(RigidBody &sphere, RigidBody &plane)
{
    Contact c;
    /*m.bodyA = &sphere; m.bodyB = &plane;

    const float    r = sphere.collider.sphere.radius;
    const Vector3f n = plane.collider.plane.normal;
    const float    d = plane.collider.plane.distance;

    float signedDist = dot(sphere.position, n) - d;
    if (signedDist >= r) return m;

    m.hasCollision = true;
    m.normal       = n;
    addContact(m, sphere.position - n * r, r - signedDist);
    */
    return c;
}

Contact testBoxBox(RigidBody &a, RigidBody &b)
{
    Contact c;
    
    return c;
}
// =============================================================================
// Box vs Plane -- up to 4 contact points
// =============================================================================
Contact testBoxPlane(RigidBody &box, RigidBody &plane)
{
    Contact c;
    
    return c;
}

// =============================================================================
// Box vs Sphere
// =============================================================================

Contact testBoxSphere(RigidBody &box, RigidBody &sphere)
{
    Contact c;
    
    return c;
}


Contact testCollision(RigidBody &a, RigidBody &b)
{
    const ColliderType tA = a.collider.type;
    const ColliderType tB = b.collider.type;

    // Sphere vs Sphere
    if (tA == ColliderType::Sphere && tB == ColliderType::Sphere)
        return testSphereSphere(a, b);

    // Sphere vs Plane
    if (tA == ColliderType::Sphere && tB == ColliderType::Plane)
        return testSpherePlane(a, b);

    if (tA == ColliderType::Plane && tB == ColliderType::Sphere)
    {
        Contact c = testSpherePlane(b, a);
        if (c.hasCollision) { std::swap(c.bodyA, c.bodyB); c.normal = c.normal * -1.0f; }
        return c;
    }

    // Box vs Plane
    if (tA == ColliderType::Box && tB == ColliderType::Plane)
        return testBoxPlane(a, b);

    if (tA == ColliderType::Plane && tB == ColliderType::Box)
    {
        Contact c = testBoxPlane(b, a);
        if (c.hasCollision) { std::swap(c.bodyA, c.bodyB); c.normal = c.normal * -1.0f; }
        return c;
    }

    // Box vs Sphere
    if (tA == ColliderType::Box && tB == ColliderType::Sphere)
        return testBoxSphere(a, b);

    if (tA == ColliderType::Sphere && tB == ColliderType::Box)
    {
        Contact c = testBoxSphere(b, a);
        if (c.hasCollision) { std::swap(c.bodyA, c.bodyB); c.normal = c.normal * -1.0f; }
        return c;
    }

    // Box vs Box
    if (tA == ColliderType::Box && tB == ColliderType::Box)
        return testBoxBox(a, b);

    // Plane vs Plane -- static, never collide meaningfully
    Contact empty;
    empty.bodyA = &a;
    empty.bodyB = &b;
    return empty;
}

// =============================================================================
// Resolution
// =============================================================================

void resolveCollision(Contact &contact)
{
    if (!contact.hasCollision) return;

    RigidBody *bodyA = contact.bodyA;
    RigidBody *bodyB = contact.bodyB;

    bodyA->linearVelocity = Vector3f(0.0f);
    bodyB->linearVelocity = Vector3f(0.0f);

    const float tA = bodyA->invMass / (bodyA->invMass + bodyB->invMass);
    const float tB = bodyB->invMass / (bodyA->invMass + bodyB->invMass);

    const Vector3f ds = contact.pointBWorldSpace - contact.pointAWorldSpace;
    bodyA->position   += ds * tA;
    bodyB->position   -= ds * tB;
} 