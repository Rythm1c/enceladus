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
 * Returns the three local axes of a body (its orientation applied to X,Y,Z).
 * These are the face normals of the OBB (oriented bounding box).
 *
 * Why quaternion * unit vector?
 *   A quaternion represents a rotation. Multiplying it by a unit axis vector
 *   gives us that axis rotated into world space. So if a box has been rotated
 *   45 degrees around Y, its local X axis (1,0,0) becomes (0.707,0,0.707)
 *   in world space -- which is exactly the direction its face normal points.
 */
static std::array<Vector3f, 3> getBoxAxes(const RigidBody &body)
{
    return {
        body.orientation * Vector3f(1.0f, 0.0f, 0.0f),  // local X
        body.orientation * Vector3f(0.0f, 1.0f, 0.0f),  // local Y
        body.orientation * Vector3f(0.0f, 0.0f, 1.0f),  // local Z
    };
}

/**
 * Project an OBB onto an axis and return its half-extent (radius).
 *
 * For a box with half-extents h and axes u0,u1,u2, the shadow on any
 * axis 'a' has half-length:
 *   r = h.x*|dot(a,u0)| + h.y*|dot(a,u1)| + h.z*|dot(a,u2)|
 *
 * Why absolute value?
 *   dot(a, u0) can be negative if the axis and the box face point in
 *   opposite directions. The shadow length is always positive, so we
 *   take the absolute value. This is equivalent to projecting all 8
 *   corners and taking the max, but much cheaper (3 dot products vs 8).
 */
static float projectBox(const RigidBody &box,
                        const std::array<Vector3f, 3> &axes,
                        const Vector3f &testAxis)
{
    const Vector3f &h = box.collider.box.halfExtents;
    return  h.x * std::abs(dot(testAxis, axes[0]))
          + h.y * std::abs(dot(testAxis, axes[1]))
          + h.z * std::abs(dot(testAxis, axes[2]));
}

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

CollisionManifold testBoxBox(RigidBody &a, RigidBody &b)
{
    CollisionManifold m;
    m.bodyA = &a;
    m.bodyB = &b;

    auto axesA = getBoxAxes(a);
    auto axesB = getBoxAxes(b);

    const Vector3f &hA = a.collider.box.halfExtents;
    const Vector3f &hB = b.collider.box.halfExtents;

    // Vector from B centre to A centre
    // BA = BO + OA = OA - OB
    Vector3f d =  a.position - b.position;

    float    minOverlap = FLT_MAX;
    Vector3f minAxis    = {0.0f, 1.0f, 0.0f};

    // Helper: test one axis.
    // Returns false if this is a separating axis (no collision on this axis).
    // Updates minOverlap and minAxis if this axis has the smallest overlap so far.

    auto testAxis = [&](Vector3f axis) -> bool 
    {
        float axisLen = axis.mag();
        // Skip near-zero axes (degenerate cross products when edges are parallel)
        if (axisLen < 1e-6f) return true; // not separating, skip

        axis = axis * (1.0f / axisLen);

        // Project both boxes onto this axis
        float rA = projectBox(a, axesA, axis);
        float rB = projectBox(b, axesB, axis);

        // Distance between centres projected onto axis
        float centerDist = std::abs(dot(d, axis));

        // If centre distance exceeds sum of radii => separating axis found
        if (centerDist > (rA + rB)) return false;

        float overlap = (rA + rB) - centerDist;
        if(overlap < minOverlap)
        {
            minOverlap = overlap;
            // Ensure axis points from B toward A (our normal convention)
            minAxis = (dot(d, axis) < 0.0f) ? axis *-1.0f : axis;
        }

        return true; // not a separating axis

    };

    // ---- Test 3 face normals of A ----------------------------------------
    if (!testAxis(axesA[0])) return m;
    if (!testAxis(axesA[1])) return m;
    if (!testAxis(axesA[2])) return m;

    // ---- Test 3 face normals of B ----------------------------------------
    if (!testAxis(axesB[0])) return m;
    if (!testAxis(axesB[1])) return m;
    if (!testAxis(axesB[2])) return m;

    // ---- Test 9 edge cross products ---------------------------------------
    // Each of A's 3 edges crossed with each of B's 3 edges.
    // Cross product of two parallel edges is zero -- testAxis handles that.
    for (int i = 0; i < 3; ++i)
        for(int j =0; j < 3; ++j)
            if(!testAxis(cross(axesA[i], axesB[j]))) return m;
    
    // No separating axis found -- boxes are overlapping
    m.hasCollision = true;
    m.depth        = minOverlap;
    m.normal       = minAxis;

    // Contact point: midpoint between the two "touching" surfaces along the axis.
    // A's surface point in the collision direction: move from A toward B by rA
    // B's surface point in the collision direction: move from B toward A by rB
    // The contact is halfway between these two surface points.
    //
    // This is approximate but stable for impulse resolution.
    // A proper implementation would clip the incident face polygon to the
    // reference face (Sutherland-Hodgman) for multi-point manifolds.
    float rA = projectBox(a, axesA, minAxis);
    float rB = projectBox(b, axesB, minAxis);

    Vector3f pointA = a.position - minAxis * rA; // A's surface toward B
    Vector3f pointB = b.position + minAxis * rB; // B's surface toward A
    m.contactPoint  = (pointA + pointB) * 0.5f;

    return m;

}
// =============================================================================
// Box vs Plane
// =============================================================================
//
// A box has 8 corners. We test each corner against the plane.
// A corner is penetrating if its signed distance is negative.
//   We average the penetrating corners
//   to get the contact point, which gives stable multi-point contact
//   (a box lying flat on a plane has 4 contact points -- averaging them
//   gives the centre of the contact face, which prevents rocking).
//
// The deepest penetrating corner gives the penetration depth.
// Normal is always the plane normal (box always pushed away from plane).
CollisionManifold testBoxPlane(RigidBody &box, RigidBody &plane)
{
    CollisionManifold m;
    m.bodyA = &box;
    m.bodyB = &plane;

    const Vector3f n = plane.collider.plane.normal;
    const float    d = plane.collider.plane.distance;
    const Vector3f h = box.collider.box.halfExtents;

    // Get the 3 local axes of the box in world space
    auto axes = getBoxAxes(box);

    // Build all 8 corners of the box in world space.
    // Each corner is: centre + combination of +/- halfExtents along each axis
    //
    //   corner = position
    //          + sx * h.x * axisX    (sx = +1 or -1)
    //          + sy * h.y * axisY
    //          + sz * h.z * axisZ
    //
    // There are 2^3 = 8 sign combinations.
    Vector3f corners[8];
    int idx = 0;
    for (int sx : {-1, 1})
    for (int sy : {-1, 1})
    for (int sz : {-1, 1})
    {
        corners[idx++] = box.position
            + axes[0] * (h.x * sx)
            + axes[1] * (h.y * sy)
            + axes[2] * (h.z * sz);
    }

    // Find all penetrating corners and track the deepest
    float    minDist      = FLT_MAX;
    Vector3f contactSum   = {0,0,0};
    int      contactCount = 0;

    for (int i = 0; i < 8; ++i)
    {
        float dist = dot(corners[i], n) - d; // signed distance to plane
        if (dist < 0.0f) // corner is below the plane = penetrating
        {
            if (dist < minDist) minDist = dist;
            contactSum   = contactSum + corners[i];
            contactCount++;
        }
    }

    if (contactCount == 0) return m; // no corners penetrating

    m.hasCollision = true;
    m.depth        = -minDist; // minDist is negative, depth is positive
    m.normal       = n;
    // Average of penetrating corners gives a stable contact point
    m.contactPoint = contactSum * (1.0f / (float)contactCount);
    return m;
}

// =============================================================================
// Box vs Sphere
// =============================================================================
//
// Algorithm: find the closest point on the box to the sphere centre,
// then check if that distance is less than the sphere radius.
//
// To use the simple "clamp to box bounds" trick we need the sphere centre
// in the BOX'S LOCAL SPACE (axis-aligned in that frame). Then we clamp,
// then transform the result back to world space.

// Special case: sphere centre INSIDE the box
//   The clamp gives the centre itself (distance = 0).
//   We find which face we're closest to and push out through it.
//   This prevents the normal flipping to (0,0,0).
CollisionManifold testBoxSphere(RigidBody &box, RigidBody &sphere)
{
    CollisionManifold m;
    m.bodyA = &box;
    m.bodyB = &sphere;

    const float    r = sphere.collider.sphere.radius;
    const Vector3f h = box.collider.box.halfExtents;

    // sphere centre in box local space
    Vector3f localSphere = box.orientation.conjugate()
                         * (sphere.position - box.position);
    
    // closest point on AABB (in local space)
    Vector3f closest;
    closest.x = clampF(localSphere.x, -h.x, h.x);
    closest.y = clampF(localSphere.y, -h.y, h.y);
    closest.z = clampF(localSphere.z, -h.z, h.z);

    // Is the sphere centre inside the box?
    bool inside = ( localSphere.x == closest.x &&
                    localSphere.y == closest.y &&
                    localSphere.z == closest.z);
    
    Vector3f delta  = localSphere - closest;
    float    distSq = dot(delta, delta);

    if (!inside && distSq >= (r * r)) return m; // outside sphere, no collision

    m.hasCollision = true;


    if (inside)
    {
        // Find the axis of minimum penetration (closest face)
        // Check how far we are from each face along each local axis
        float dists[6] = {
            h.x - localSphere.x,  // +X face
            h.x + localSphere.x,  // -X face
            h.y - localSphere.y,  // +Y face
            h.y + localSphere.y,  // -Y face
            h.z - localSphere.z,  // +Z face
            h.z + localSphere.z,  // -Z face
        };
        Vector3f faceNormals[6] = {
            { 1,0,0},{-1,0,0},
            { 0,1,0},{0,-1,0},
            { 0,0,1},{0,0,-1}
        };

        // Find the face with minimum distance (shallowest penetration)
        int minIdx = 0;
        for (int i = 1; i < 6; ++i)
            if (dists[i] < dists[minIdx]) minIdx = i;

        // Push sphere out through that face
        // Transform local normal back to world space
        auto axes = getBoxAxes(box);
        Vector3f localNormal = faceNormals[minIdx];
        Vector3f worldNormal = axes[0] * localNormal.x
                             + axes[1] * localNormal.y
                             + axes[2] * localNormal.z;

        m.depth        = r + dists[minIdx];
        m.normal       = worldNormal * -1.0f; // from box(B) toward sphere(A)
        m.contactPoint = sphere.position;
    }
    else
    {
        float dist = std::sqrt(distSq);
        // Transform closest point back to world space
        auto axes = getBoxAxes(box);
        Vector3f worldClosest = box.position
            + axes[0] * closest.x
            + axes[1] * closest.y
            + axes[2] * closest.z;

        m.depth        = r - dist;
        // Normal from sphere (B) toward closest point on box (A)
        // Convention: normal always points FROM bodyB TOWARD bodyA
        m.normal       = (dist > 1e-6f)
                       ? (worldClosest - sphere.position) * (1.0f / dist)
                        : Vector3f(0.0f, 1.0f, 0.0f);
        m.contactPoint = worldClosest;
    }

    return m;
}


CollisionManifold testCollision(RigidBody &a, RigidBody &b)
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
        CollisionManifold m = testSpherePlane(b, a);
        if (m.hasCollision) { std::swap(m.bodyA, m.bodyB); m.normal = m.normal * -1.0f; }
        return m;
    }

    // Box vs Plane
    if (tA == ColliderType::Box && tB == ColliderType::Plane)
        return testBoxPlane(a, b);

    if (tA == ColliderType::Plane && tB == ColliderType::Box)
    {
        CollisionManifold m = testBoxPlane(b, a);
        if (m.hasCollision) { std::swap(m.bodyA, m.bodyB); m.normal = m.normal * -1.0f; }
        return m;
    }

    // Box vs Sphere
    if (tA == ColliderType::Box && tB == ColliderType::Sphere)
        return testBoxSphere(a, b);

    if (tA == ColliderType::Sphere && tB == ColliderType::Box)
    {
        CollisionManifold m = testBoxSphere(b, a);
        if (m.hasCollision) { std::swap(m.bodyA, m.bodyB); m.normal = m.normal * -1.0f; }
        return m;
    }

    // Box vs Box
    if (tA == ColliderType::Box && tB == ColliderType::Box)
        return testBoxBox(a, b);

    // Plane vs Plane -- static, never collide meaningfully
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