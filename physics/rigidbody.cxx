#include "headers/rigidbody.hxx"

void RigidBody::setMass(float m)
{
    mass    = m;
    invMass = (m > 0.0f) ? 1.0f / m : 0.0f;

    computeInertia();
}

void RigidBody::makeStatic()
{
    isStatic        = 0.0f;
    mass            = 0.0f;
    invMass         = 0.0f;
    invInertia      = {0.0f, 0.0f, 0.0f};
    linearVelocity  = {0.0f, 0.0f, 0.0f};
    angularVelocity = {0.0f, 0.0f, 0.0f};
}

void RigidBody::computeInertia()
{
    if (invMass == 0.0f)
    {
        invInertia = {0.0f, 0.0f, 0.0f};
        return;
    }

    switch (collider.type)
    {
        // Solid sphere: I = 2/5 * m * r^2  (same on all axes)
        case ColliderType::Sphere:
            {
                const float r    = collider.sphere.radius;
                const float I    = (2.0f / 5.0f) * mass * r * r;
                const float invI = (I > 0.0f) ? 1.0f / I : 0.0f;
                invInertia = {invI, invI, invI};
                break;
            }
            
        case ColliderType::Box:
            {
                // Solid box, diagonal inertia tensor:
                // Ixx = 1/12 * m * (hy^2 + hz^2)   where h = half-extent * 2
                const Vector3f h = collider.box.halfExtents;
                const float dx  = 2.0f * h.x, dy = 2.0f * h.y, dz = 2.0f * h.z;
                const float Ixx = (1.0f / 12.0f) * mass * (dy * dy + dz * dz);
                const float Iyy = (1.0f / 12.0f) * mass * (dx * dx + dz * dz);
                const float Izz = (1.0f / 12.0f) * mass * (dx * dx + dy * dy);
                invInertia = {
                    (Ixx > 0.0f) ? 1.0f / Ixx : 0.0f,
                    (Iyy > 0.0f) ? 1.0f / Iyy : 0.0f,
                    (Izz > 0.0f) ? 1.0f / Izz : 0.0f,
                };
                break;
            }
            
        case ColliderType::Plane:
            {
                invInertia = {0.0f, 0.0f, 0.0f};
                break;
            }
            break;
    }
}

void RigidBody::applyForce(Vector3f force)
{
    if(!hasFiniteMass()) return;
    forceAccum += force;
}

void RigidBody::applyForceAtPoint(Vector3f force, Vector3f worldPoint)
{
    if(!hasFiniteMass()) return;
    forceAccum += force;
    // Torque = r x F  where r is the vector from centre of mass to the point
    Vector3f r  = worldPoint - position;
    torqueAccum += cross(r, force);
}

void RigidBody::applyLinearImpulse(Vector3f impulse)
{
    if(!hasFiniteMass()) return;
    linearVelocity += impulse * invMass;
}

void RigidBody::applyAngularImpulse(Vector3f impulse)
{
    if(!hasFiniteMass()) return;
    // Angular velocity change = I^-1 * impulse
    // invInertia is diagonal so this is component-wise multiplication
    angularVelocity += Vector3f(
        impulse.x * invInertia.x,
        impulse.y * invInertia.y,
        impulse.z * invInertia.z);
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