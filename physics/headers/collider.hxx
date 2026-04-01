#ifndef COLLIDER_HXX
#define COLLIDER_HXX

#include "../../math/headers/vec3.hxx"

enum class ColliderType
{
    Sphere,
    Box,
    Plane // infinite half-space: all points where dot(normal, x) >= distance

};

struct Collider
{
    ColliderType type = ColliderType::Sphere;

    Collider() : sphere{0.0f} {}

    union{
        struct
        {
            float radius;
        } sphere;

        struct
        {
            // Half-extents: distance from centre to face along each axis.
            // A 1x1x1 cube has halfExtents = {0.5, 0.5, 0.5}
            Vector3f halfExtents;
        } box;

        struct
        {
            // Plane equation: dot(normal, point) = distance
            // normal must be unit length.
            // Example: floor at y=0 facing up: normal={0,1,0}, distance=0
            //          floor at y=-1.5:        normal={0,1,0}, distance=-1.5
            Vector3f normal;
            float    distance;
        } plane;
    };

    static Collider makeSphere(float radius)
    {
        Collider c;
        c.type          = ColliderType::Sphere;
        c.sphere.radius = radius;
        return c;
    }

    static Collider makeBox(Vector3f halfExtents)
    {
        Collider c;
        c.type            = ColliderType::Box;
        c.box.halfExtents = halfExtents;
        return c;
    }

    static Collider makePlane(Vector3f normal, float distance)
    {
        Collider c;
        c.type           = ColliderType::Plane;
        c.plane.normal   = normal;
        c.plane.distance = distance;
        return c;
    }
};

#endif