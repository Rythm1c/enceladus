#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "mat4.hxx"
#include "quaternion.hxx"
#include "vec3.hxx"

class Transform
{
public:
  Transform()  = default;
  
  Transform(const Mat4x4 &mat);
  Transform(const Vector3f &translation, const Quat &orientation, const Vector3f &scaling)
      : translation(translation), orientation(orientation), scaling(scaling) {}

  ~Transform() = default;

  Vector3f translation = Vector3f();
  Quat     orientation = Quat();
  Vector3f scaling     = Vector3f(1.0);

  /// @brief combines the translation, rotation and scaling members to produce a
  /// transformation matrix
  /// @return finall transform matrix
  Mat4x4    toMat4x4()     const;
  Transform inverse()      const;

  /// @brief combines two transforms
  /// @param t1 first transform
  /// @param t2 second transform
  /// @return combined transform
  static Transform combine(const Transform &t1, const Transform &t2);
};

#endif
