#include "headers/collision.hxx"
#include "headers/rigidbody.hxx"

#include <cmath>

// =============================================================================
// Detection
// =============================================================================

CollisionManifold testSphereSphere(RigidBody &a, RigidBody &b)
{
    CollisionManifold m;
    m.bodyA = &a;
    m.bodyB = &b;

    const float rA   = a.collider.sphere.radius;
    const float rB   = b.collider.sphere.radius;
    const float rSum = rA + rB;

    Vector3f delta = a.position - b.position;
    const float distSq = dot(delta, delta);

    // Early out: squared distance check avoids sqrt when not colliding
    if (distSq >= (rSum * rSum))
        return m;

    const float dist = std::sqrt(distSq);

    m.hasCollision = true;
    m.depth        = rSum - dist;

    // Normal from B toward A -- if centres coincide use world up to avoid NaN
    m.normal = (dist > 1e-6f) ? delta * (1.0f / dist) : Vector3f(0.0f, 1.0f, 0.0f);

    // Contact point: surface of sphere B in the direction of A
    m.contactPoint = b.position + m.normal * rB;

    return m;
}

CollisionManifold testSpherePlane(RigidBody &sphere, RigidBody &plane)
{
    CollisionManifold m;
    m.bodyA = &sphere;
    m.bodyB = &plane;

    const float     r = sphere.collider.sphere.radius;
    const Vector3f &n = plane.collider.plane.normal;
    const float     d = plane.collider.plane.distance;

    // Signed distance from sphere centre to plane:
    // positive = sphere is on the side the normal points to
    const float signedDist = dot(sphere.position, n) - d;

    // sphere is fully above the plane
    if (signedDist >= r) return m;

    m.hasCollision = true;
    m.depth        = r - signedDist; // how far into the plane
    m.normal       = n;              // plane normal always points from plane (B) toward sphere (A)
    m.contactPoint = sphere.position - n * r; // bottom of sphere

    return m;

}

CollisionManifold testCollision(RigidBody &a, RigidBody &b)
{
    if (a.collider.type == ColliderType::Sphere &&
        b.collider.type == ColliderType::Sphere)
        return testSphereSphere(a, b);
    
    if (a.collider.type == ColliderType::Sphere &&
        b.collider.type == ColliderType::Plane)
        return testSpherePlane(a, b);
    
    if (a.collider.type == ColliderType::Plane &&
        b.collider.type == ColliderType::Sphere)
    {
        // Flip: testSpherePlane expects sphere first
        CollisionManifold m = testSpherePlane(b, a);
        if(m.hasCollision)
        {
             // Swap A/B and flip normal to maintain convention
            std::swap(m.bodyA, m.bodyB);
            m.normal = m.normal * -1.0f;
        }
        return m;
    }
    
    CollisionManifold empty;
    empty.bodyA = &a;
    empty.bodyB = &b;
    return empty;
}

// =============================================================================
// Resolution
// =============================================================================

void resolveCollision(CollisionManifold &manifold)
{
    if(!manifold.hasCollision) return;

    RigidBody &a = *manifold.bodyA;
    RigidBody &b = *manifold.bodyB;

    const Vector3f &n = manifold.normal;

    // ---- 1. Normal impulse --------------------------------------------------

    // Relative velocity at the contact point
    Vector3f velA = a.velocityAtPoint(manifold.contactPoint);
    Vector3f velB = b.velocityAtPoint(manifold.contactPoint);
    Vector3f relVel = velA - velB;

    // Component of relative velocity along the collision normal
    float velAlongNormal = dot(relVel , n);

    // Bodies already separating -- no impulse needed
    if(velAlongNormal > 0.0f) return;

    // Combined restitution (use minimum -- more physically conservative)
    float e = (a.restitution < b.restitution) ? a.restitution : b.restitution;

    // Angular contribution to the denominator:
    //   (r x n) . I^-1 . (r x n)  for each body
    // This accounts for how the impulse affects rotation.
    auto angularFactor = [](const RigidBody &body, const Vector3f &r, const Vector3f &normal) -> float
    {
        Vector3f rxn = cross(r, normal);

        Vector3f invI_rxn = Vector3f(
            rxn.x * body.invInertia.x,
            rxn.y * body.invInertia.y,
            rxn.z * body.invInertia.z
        );
        return dot(cross(invI_rxn, r), normal);
    };

    Vector3f rA = manifold.contactPoint - a.position;
    Vector3f rB = manifold.contactPoint - b.position;

    float denom = a.invMass + b.invMass
                + angularFactor(a, rA, n)
                + angularFactor(b, rB, n);
    
    if(denom < 1e-8f) return;

    // Magnitude of the normal impulse
    float j = -(1.0f + e) * velAlongNormal / denom;

    Vector3f impulse = n * j;

    a.applyLinearImpulse(        impulse);
    b.applyLinearImpulse(-1.0f * impulse);

    a.applyAngularImpulse(        cross(rA, impulse));
    b.applyAngularImpulse(-1.0f * cross(rB, impulse));

    // ---- 2. Friction impulse ------------------------------------------------

    // Recompute relative velocity after normal impulse
    velA = a.velocityAtPoint(manifold.contactPoint);
    velB = b.velocityAtPoint(manifold.contactPoint);
    relVel = velA - velB;

    Vector3f tangent = relVel - n * dot(relVel, n);
    float tangentLen = tangent.mag();

    if(tangentLen > 1e-6f)
    {
        tangent = tangent * (1.0f / tangentLen);
        float denomT  = a.invMass + b.invMass 
                    + angularFactor(a, rA, tangent)
                    + angularFactor(b, rB, tangent);
        
        if(denomT > 1e-8f)
        {
            float jt = -dot(relVel, tangent) / denomT;

            // Coulomb's law: friction impulse clamped by mu * normal impulse
            float mu     = (a.friction + b.friction) * 0.5f;
            float jClamp = mu * j; // j is already the normal impulse magnitude

            // Use static friction if sliding is small, dynamic otherwise
            Vector3f frictionImpulse;
            if (std::abs(jt) <= std::abs(jClamp))
                frictionImpulse = tangent * jt;           // static
            else
                frictionImpulse = tangent * (-jClamp);    // dynamic (kinetic)

            a.applyLinearImpulse(       frictionImpulse);
            b.applyLinearImpulse(-1.0 * frictionImpulse);
            a.applyAngularImpulse(       cross(rA,  frictionImpulse));
            b.applyAngularImpulse(-1.0 * cross(rB, -1.0 * frictionImpulse));
        }
    }

    // ---- 3. Positional correction (Baumgarte stabilisation) ----------------
    // Without this, floating-point errors cause objects to slowly sink into
    // each other over many frames. We push bodies apart by a fraction of the
    // penetration depth each step.
    //
    // slop:  small allowed penetration before correction kicks in (prevents jitter)
    // percent: fraction of penetration corrected per step (0.2-0.4 is typical)
    const float slop    = 0.01f;
    const float percent = 0.3f;

    float correctionMag = std::max(manifold.depth - slop, 0.0f)
                        / (a.invMass + b.invMass)
                        * percent;
    Vector3f correction = n * correctionMag;

    if (a.hasFiniteMass()) a.position += correction * a.invMass;
    if (b.hasFiniteMass()) b.position -= correction * b.invMass;

} 