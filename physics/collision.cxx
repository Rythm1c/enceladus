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

// Add a contact point to the manifold (up to 4 max)
static void addContact(CollisionManifold &m, Vector3f pos, float depth)
{
    if (m.contactCount < 4)
    {
        m.contacts[m.contactCount].position = pos;
        m.contacts[m.contactCount].depth    = depth;
        ++m.contactCount;
    }
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
    m.bodyA = &a; m.bodyB = &b;

    const float rA = a.collider.sphere.radius;
    const float rB = b.collider.sphere.radius;
    const float rSum = rA + rB;

    Vector3f delta = a.position - b.position;
    float distSq   = dot(delta, delta);
    if (distSq >= rSum * rSum) return m;

    float dist     = std::sqrt(distSq);
    m.hasCollision = true;
    m.normal = (dist > 1e-6f) ? delta * (1.0f / dist) : Vector3f(0.0f, 1.0f, 0.0f);

    addContact(m, b.position + m.normal * rB, rSum - dist);
    return m;
}

CollisionManifold testSpherePlane(RigidBody &sphere, RigidBody &plane)
{
    CollisionManifold m;
    m.bodyA = &sphere; m.bodyB = &plane;

    const float    r = sphere.collider.sphere.radius;
    const Vector3f n = plane.collider.plane.normal;
    const float    d = plane.collider.plane.distance;

    float signedDist = dot(sphere.position, n) - d;
    if (signedDist >= r) return m;

    m.hasCollision = true;
    m.normal       = n;
    addContact(m, sphere.position - n * r, r - signedDist);
    return m;

}

CollisionManifold testBoxBox(RigidBody &a, RigidBody &b)
{
    CollisionManifold m;
    m.bodyA = &a; m.bodyB = &b;

    auto axesA = getBoxAxes(a);
    auto axesB = getBoxAxes(b);

    const Vector3f &hA = a.collider.box.halfExtents;
    const Vector3f &hB = b.collider.box.halfExtents;
    Vector3f d = a.position - b.position;

    float    minOverlap = FLT_MAX;
    Vector3f minAxis    = {0.0f, 1.0f, 0.0f};

    auto testAxis = [&](Vector3f axis) -> bool
    {
        float axisLen = axis.mag();
        if (axisLen < 1e-6f) return true;
        axis = axis * (1.0f / axisLen);

        float rA         = projectBox(a, axesA, axis);
        float rB         = projectBox(b, axesB, axis);
        float centreDist = std::abs(dot(d, axis));
        if (centreDist > rA + rB) return false;

        float overlap = (rA + rB) - centreDist;
        if (overlap < minOverlap)
        {
            minOverlap = overlap;
            minAxis    = (dot(d, axis) < 0.0f) ? axis * -1.0f : axis;
        }
        return true;
    };

    if (!testAxis(axesA[0])) return m;
    if (!testAxis(axesA[1])) return m;
    if (!testAxis(axesA[2])) return m;
    if (!testAxis(axesB[0])) return m;
    if (!testAxis(axesB[1])) return m;
    if (!testAxis(axesB[2])) return m;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (!testAxis(cross(axesA[i], axesB[j]))) return m;

    m.hasCollision = true;
    m.normal       = minAxis;

    // Find contact points: corners of box B that penetrate box A.
    // Transform each corner of B into A's local space and check if it's inside A.
    // This gives up to 4 contacts for face-face configurations.
    const Vector3f hBe = b.collider.box.halfExtents;
    for (int sx : {-1,1})
    for (int sy : {-1,1})
    for (int sz : {-1,1})
    {
        if (m.contactCount >= 4) break;

        Vector3f worldCorner = b.position
            + axesB[0] * (hBe.x * sx)
            + axesB[1] * (hBe.y * sy)
            + axesB[2] * (hBe.z * sz);

        // Check if this corner is inside box A
        Vector3f localCorner = a.orientation.conjugate() * (worldCorner - a.position);
        const Vector3f &hAe  = a.collider.box.halfExtents;
        if (std::abs(localCorner.x) <= hAe.x + 1e-4f &&
            std::abs(localCorner.y) <= hAe.y + 1e-4f &&
            std::abs(localCorner.z) <= hAe.z + 1e-4f)
        {
            addContact(m, worldCorner, minOverlap);
        }
    }

    // If no corners of B were inside A, fall back to midpoint approximation
    if (m.contactCount == 0)
    {
        float rA = projectBox(a, axesA, minAxis);
        float rB = projectBox(b, axesB, minAxis);
        Vector3f pA = a.position - minAxis * rA;
        Vector3f pB = b.position + minAxis * rB;
        addContact(m, (pA + pB) * 0.5f, minOverlap);
    }

    return m;
}
// =============================================================================
// Box vs Plane -- up to 4 contact points
//
// Each penetrating corner is stored as a separate contact point.
// This is the key change from the single-point version:
//
//   BEFORE: average penetrating corners -> 1 contact at the centroid
//   AFTER:  store each corner -> N contacts, each receiving 1/N impulse
//
//   When a box rests flat on a plane, all 4 bottom corners penetrate equally.
//   With 1 contact at the centroid, the impulse is applied at the box centre --
//   correct for linear velocity but it applies zero torque (r = 0).
//   With 4 contacts at the actual corners, each one applies a small torque.
//   These torques cancel out for a flat box (symmetric) which is correct.
//   For a slightly tilted box, the deeper corners produce stronger torques
//   that actively rotate the box back to flat -- physical behaviour.
// =============================================================================
CollisionManifold testBoxPlane(RigidBody &box, RigidBody &plane)
{
    CollisionManifold m;
    m.bodyA = &box; m.bodyB = &plane;

    const Vector3f n = plane.collider.plane.normal;
    const float    d = plane.collider.plane.distance;
    const Vector3f h = box.collider.box.halfExtents;
    auto           axes = getBoxAxes(box);

    // Build all 8 corners in world space
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

    // Test each corner -- store every penetrating one as its own contact
    /* for (int i = 0; i < 8 && m.contactCount < 4; ++i)
    {
        float dist = dot(corners[i], n) - d;
        if (dist < 0.0f)
        {
            addContact(m, corners[i], -dist);
        }
    }*/

    struct Candidate
    {
        Vector3f pos;
        float depth;
    };

    Candidate candidates[8];
    int count = 0;

    for (int i = 0; i < 8; ++i)
    {
        float dist = dot(corners[i], n) - d;

        if (dist < 0.0f)
        {
            candidates[count++] =
            {
                corners[i],
                -dist
            };
        }
    }

    // sort deepest first
    std::sort(
        candidates,
        candidates + count,
        [](const Candidate &a, const Candidate &b)
        {
            return a.depth > b.depth;
        }
    );

    // take up to 4
    for (int i = 0; i < std::min(count, 4); ++i)
    {
        addContact(m,
            candidates[i].pos,
            candidates[i].depth);
    }
    if (m.contactCount > 0)
    {
        m.hasCollision = true;
        m.normal       = n;
    }
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
    m.bodyA = &box; m.bodyB = &sphere;

    const float    r = sphere.collider.sphere.radius;
    const Vector3f h = box.collider.box.halfExtents;

    // Sphere centre in box local space (conjugate = inverse rotation for unit quats)
    Vector3f localSphere = box.orientation.conjugate()
                         * (sphere.position - box.position);

    Vector3f closest;
    closest.x = clampF(localSphere.x, -h.x, h.x);
    closest.y = clampF(localSphere.y, -h.y, h.y);
    closest.z = clampF(localSphere.z, -h.z, h.z);

    bool inside = ( localSphere.x == closest.x &&
                    localSphere.y == closest.y &&
                    localSphere.z == closest.z);

    Vector3f delta = localSphere - closest;
    float distSq   = dot(delta, delta);
    if (!inside && distSq >= r * r) return m;

    m.hasCollision = true;

    if (inside)
    {
        // Find the axis of least penetration to push sphere out of box
        float dists[6] = {
            h.x - localSphere.x, h.x + localSphere.x,
            h.y - localSphere.y, h.y + localSphere.y,
            h.z - localSphere.z, h.z + localSphere.z,
        };
        Vector3f faceNormals[6] = {
            {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
        };
        int minIdx = 0;
        for (int i = 1; i < 6; ++i)
            if (dists[i] < dists[minIdx]) minIdx = i;

        auto axes = getBoxAxes(box);
        Vector3f localNormal = faceNormals[minIdx];
        Vector3f worldNormal = axes[0]*localNormal.x + axes[1]*localNormal.y + axes[2]*localNormal.z;

        m.normal = worldNormal * -1.0f;
        addContact(m, sphere.position, r + dists[minIdx]);
    }
    else
    {
        float dist = std::sqrt(distSq);
        auto  axes = getBoxAxes(box);
        Vector3f worldClosest = box.position
            + axes[0]*closest.x + axes[1]*closest.y + axes[2]*closest.z;

        m.normal = (dist > 1e-6f) ? (sphere.position - worldClosest) * (1.0f / dist) : Vector3f(0.0f, 1.0f, 0.0f);
        addContact(m, worldClosest, r - dist);
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
    if (!manifold.hasCollision || manifold.contactCount == 0) return;

    RigidBody &a = *manifold.bodyA;
    RigidBody &b = *manifold.bodyB;

    const Vector3f &n    = manifold.normal;
    const float invCount = 1.0f / static_cast<float>(manifold.contactCount);

    auto angFactor = [](const RigidBody &body,
                        const Vector3f  &r,
                        const Vector3f  &axis) -> float
    {
        Vector3f rxn  = cross(r, axis);
        Vector3f iRxn = body.getWorldInvInertia() * rxn;
        return dot(cross(iRxn, r), axis);
    };

    for (int ci = 0; ci < manifold.contactCount; ++ci)
    {
        const Vector3f &cp = manifold.contacts[ci].position;

        Vector3f rA = cp - a.position;
        Vector3f rB = cp - b.position;

        Vector3f relVel = a.velocityAtPoint(cp) - b.velocityAtPoint(cp);

        float velAlongNormal = dot(relVel, n);
        if (velAlongNormal > 0.0f) continue;

        float e = (velAlongNormal < -0.5f)
                ? std::min(a.restitution, b.restitution)
                : 0.0f;

        float denom = a.invMass + b.invMass
                    + angFactor(a, rA, n)
                    + angFactor(b, rB, n);
        if (denom < 1e-8f) continue;

        float    j       = -(1.0f + e) * velAlongNormal / denom * invCount;
        Vector3f impulse = n * j;

        a.applyLinearImpulse( impulse);
        b.applyLinearImpulse(-1.0*impulse);

        // KEY FIX: one negation only on B's angular impulse
        // cross(r, impulse) gives the torque direction from a force at that point
        // A gets positive torque, B gets equal and opposite (Newton's 3rd law)
        a.applyAngularImpulse( cross(rA, impulse));
        b.applyAngularImpulse(-1.0*cross(rB, impulse));

        // Friction
        relVel = a.velocityAtPoint(cp) - b.velocityAtPoint(cp);
        Vector3f tangent    = relVel - n * dot(relVel, n);
        float    tangentLen = tangent.mag();

        if (tangentLen > 1e-6f)
        {
            tangent = tangent * (1.0f / tangentLen);

            float denomT = a.invMass + b.invMass
                        + angFactor(a, rA, tangent)
                        + angFactor(b, rB, tangent);

            if (denomT > 1e-8f)
            {
                float jt = -dot(relVel, tangent) / denomT * invCount;
                float mu = (a.friction + b.friction) * 0.5f;

                // Static friction if clamping not needed, kinetic otherwise
                // Kinetic: use copysign to preserve sliding direction
                Vector3f fImpulse = (std::abs(jt) <= mu * std::abs(j))
                                  ? tangent * jt
                                  : tangent * std::copysign(-mu * std::abs(j), jt);

                a.applyLinearImpulse( fImpulse);
                b.applyLinearImpulse(-1.0*fImpulse);

                // Apply torques from friction at this contact point
                a.applyAngularImpulse( cross(rA, fImpulse));
                b.applyAngularImpulse(-1.0*cross(rB, fImpulse));
            }
        }
    }

    // Baumgarte
    const float slop   = 0.01f;
    const float pct    = 0.1f;
    float       totInv = a.invMass + b.invMass;
    if (totInv > 1e-8f)
    {
        float corrMag = std::max(manifold.maxDepth() - slop, 0.0f) / totInv * pct;
        Vector3f corr = n * corrMag;
        if (a.hasFiniteMass()) a.position = a.position + corr * a.invMass;
        if (b.hasFiniteMass()) b.position = b.position - corr * b.invMass;
    }
} 