#ifndef RIGIDBODY_HXX
#define RIGIDBODY_HXX

#include "../../math/headers/math.hxx"
#include "collider.hxx"

struct RigidBody
{
    Vector3f position        = {0.0f, 0.0f, 0.0f};
    Quat     orientation     = Quat();
    Vector3f linearVelocity  = {0.0f, 0.0f, 0.0f};
    Vector3f angularVelocity = {0.0f, 0.0f, 0.0f};

    // ---- Physical properties -----------------------------------------------
    float mass           = 1.0f;
    float invMass        = 1.0f;  // precomputed; 0 for static bodies
    float restitution    = 0.4f;  // bounciness [0 = dead stop, 1 = perfect bounce]
    float linearDamping  = 0.99f; // multiply velocity by this each step (drag)
    float angularDamping = 0.98f; // same for angular velocity
    float friction       = 0.5f;  // used in tangential impulse

    // ---- Inertia (diagonal of the inertia tensor in local space) -----------
    // Stored as inverse so division becomes multiplication in the solver.
    // Computed automatically by setMass() based on collider type.
    Mat3x3   inertiaTensor;
    Mat3x3   invInertiaTensor;

    // ---- Accumulated forces (cleared at the end of each step) --------------
    Vector3f forceAccum  = {0.0f, 0.0f, 0.0f};
    Vector3f torqueAccum = {0.0f, 0.0f, 0.0f};

    // ---- Collision geometry -------------------------------------------------
    Collider collider;

    // ---- Sleep system ------------------------------------------------------
    bool  isStatic   = false;
    bool  isSleeping = false;
    float sleepTimer = 0.0f;

    static constexpr float SLEEP_VELOCITY  = 0.05f;  // speed below which body is a candidate
    static constexpr float SLEEP_TIME      = 0.5f;   // seconds below threshold before sleeping

    /**
     * Set mass and recompute invMass + invInertia based on the current collider.
     * Call after setting the collider and before adding to PhysicsWorld.
     */
    void setMass(float m);

    /**
     * Make this body completely immovable.
     * Equivalent to infinite mass -- absorbs any impulse without moving.
     */
    void makeStatic();

    // ========================================================================
    // Force application
    // ========================================================================

    // Apply a force at the centre of mass (no torque generated)
    void applyForce(Vector3f force);

    // Apply a force at a world-space point (generates both force and torque)
    void applyForceAtPoint(Vector3f force, Vector3f worldPoint);

    // Apply an instantaneous velocity change (bypasses mass, used by solver)
    void applyLinearImpulse(Vector3f impulse);

    // Apply an angular impulse (used by solver)
    void applyAngularImpulse(Vector3f impulse);

    /**
     * Returns the model matrix for this body.
     * Pass directly to shape.setTransform():
     *   shape.setTransform(body.getTransformMatrix());
     */
    Transform getTransform() const;

    Mat3x3 getWorldInvInertia() const;

    /**
     * Velocity of the body at a specific world-space point
     * (accounts for angular velocity).
     * Used by the collision solver to compute relative contact velocity.
     */
    Vector3f velocityAtPoint(Vector3f worldPoint) const;

    /**
     * Returns true if this body should be treated as having infinite mass
     * (isStatic or invMass == 0).
     */
    bool hasFiniteMass() const {return !isStatic && invMass > 0.0f; }

    /** Wake this body from sleep (call when an impulse or force is applied). */
    void wake() { isSleeping = false; sleepTimer = 0.0f; }

private:
    // Recomputes invInertia from mass and current collider geometry.
    void computeInertia();

};

#endif