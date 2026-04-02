#include "headers/rigidbody.hxx"

void RigidBody::setMass(float m)
{
    mass    = m;
    invMass = (m > 0.0f) ? 1.0f / m : 0.0f;

    computeInertia();
}

void RigidBody::makeStatic()
{
    isStatic         = true;
    mass             = 0.0f;
    invMass          = 0.0f;
    inertiaTensor    = Mat3x3(); // all zeros (default constructor)
    invInertiaTensor = Mat3x3();
    linearVelocity   = {0.0f, 0.0f, 0.0f};
    angularVelocity  = {0.0f, 0.0f, 0.0f};
}

void RigidBody::computeInertia()
{
    if (invMass == 0.0f)
    {
        inertiaTensor    = Mat3x3();
        invInertiaTensor = Mat3x3();
        return;
    }

    // Build a diagonal Mat3x3 from the three principal-axis inertias.
    // Off-diagonal elements are zero because the local frame is aligned
    // with the principal axes of the shape by definition.
    //
    // The diagonal entries come from standard analytic formulas:
    //   Sphere:  I = 2/5 * m * r^2  (same on all three axes -- fully symmetric)
    //   Box:     Ixx = 1/12 * m * (dy^2 + dz^2)  where d = full side length = 2*halfExtent
    //
    // Plate:     Plane bodies are always static, so this branch just zeroes out.

    float Ixx = 0.0f, Iyy = 0.0f, Izz = 0.0f;

    switch (collider.type)
    {
        // Solid sphere: I = 2/5 * m * r^2  (same on all axes)
        case ColliderType::Sphere:
            {
                const float r = collider.sphere.radius;
                Ixx = Iyy = Izz = (2.0f / 5.0f) * mass * r * r;
                break;
            }
        case ColliderType::Box:
            {
                // Solid box, diagonal inertia tensor:
                // Ixx = 1/12 * m * (hy^2 + hz^2)   where h = half-extent * 2
                // Full side lengths (half-extents * 2)
                const Vector3f h  = collider.box.halfExtents;
                const float    dx = 2.0f * h.x;
                const float    dy = 2.0f * h.y;
                const float    dz = 2.0f * h.z;
                Ixx = (1.0f / 12.0f) * mass * (dy * dy + dz * dz);
                Iyy = (1.0f / 12.0f) * mass * (dx * dx + dz * dz);
                Izz = (1.0f / 12.0f) * mass * (dx * dx + dy * dy);
                break;
            }
            
        case ColliderType::Plane:
            break;
    }

    // Store as diagonal Mat3x3
    inertiaTensor = Mat3x3(
        Vector3f(Ixx,  0.0f, 0.0f),
        Vector3f(0.0f, Iyy,  0.0f),
        Vector3f(0.0f, 0.0f, Izz));

    // Invert: for a diagonal matrix this is just inverting each diagonal entry.
    // Guard against zero (shouldn't happen for finite mass, but be safe).
    auto safeInv = [](float x) { return x > 1e-10f ? 1.0f / x : 0.0f; };

    invInertiaTensor = Mat3x3(
        Vector3f(safeInv(Ixx), 0.0f,          0.0f),
        Vector3f(0.0f,         safeInv(Iyy),  0.0f),
        Vector3f(0.0f,         0.0f,          safeInv(Izz)));
}

void RigidBody::applyForce(Vector3f force)
{
    if(!hasFiniteMass()) return;
    wake();
    forceAccum += force;
}

void RigidBody::applyForceAtPoint(Vector3f force, Vector3f worldPoint)
{
    if(!hasFiniteMass()) return;
    wake();
    forceAccum += force;
    // Torque = r x F  where r is the vector from centre of mass to the point
    torqueAccum += cross(worldPoint - position, force);
}

void RigidBody::applyLinearImpulse(Vector3f impulse)
{
    if(!hasFiniteMass()) return;
    wake();
    linearVelocity += impulse * invMass;
}

void RigidBody::applyAngularImpulse(Vector3f impulse)
{
    if(!hasFiniteMass()) return;
    wake();
    // Δω = I_world^-1 * impulse
    // This is a full matrix-vector multiply, not component-wise.
    // The world-space tensor correctly accounts for the body's current rotation.
    angularVelocity += getWorldInvInertia() * impulse;
}
Mat3x3 RigidBody::getWorldInvInertia() const
{
    // Transform the local inverse inertia tensor into world space:
    //   I_world_inv = R * I_local_inv * R^T
    //
    //   The local tensor is defined relative to the body's own axes.
    //   When the body has rotated (R != identity), its local axes no longer
    //   align with the world axes. Multiplying by R rotates the tensor
    //   into world space so that the impulse (which is in world space)
    //   correctly maps to the right angular velocity change.
    //
    // For static bodies or those with zero inertia, this returns a zero matrix,
    // which correctly produces no angular velocity change from any impulse.

    Mat3x3 R = orientation.toMat3x3();
    return R * invInertiaTensor * R.transpose();
}
Vector3f RigidBody::velocityAtPoint(Vector3f worldPoint) const
{
    // v_point = v_linear + omega x r
    Vector3f r = worldPoint - position;
    return linearVelocity + cross(angularVelocity, r);
}

Transform RigidBody::getTransform() const
{
    // Build model matrix: translate(position) * rotate(orientation)
    // No scale -- physics bodies are unit-scale (scale is baked into collider dims)
    Transform transform;
    transform.translation = position;
    transform.orientation = orientation;

    return transform;
}