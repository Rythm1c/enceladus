#ifndef UBO_HXX
#define UBO_HXX

#include "../external/math/mat4.hxx"

struct CameraUBO{
    Mat4x4 view;
    Mat4x4 proj;
};

#endif