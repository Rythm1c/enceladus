#ifndef QUATERNION_H
#define QUATERNION_H

#include "utils.hxx"
#include <array>

#define QUAT_EPSILON 0.000001f

struct Mat3x3;
struct Mat4x4;
struct Vector3f;

struct Quat
{
  union
  {
    struct
    {
      float x;
      float y;
      float z;
      float s;
    };

    float v[4];
  };

  Quat() : x(0.0), y(0.0), z(0.0), s(1.0) {}
  Quat(float v) : x(v), y(v), z(v), s(v) {}
  Quat(float _x, float _y, float _z, float _s) : x(_x), y(_y), z(_z), s(_s) {}

  /// @brief creates a quaternion from an angle and specified axis
  /// @param 1: angle
  /// @param 2: axis
  Quat(float, Vector3f);

  float norm() const;
  Quat  unit() const;
  
  // conjugate() of a unit quaternion is its inverse rotation
  Quat conjugate() const;
  Quat inverse()   const;

  Mat3x3 toMat3x3() const;
  Mat4x4 toMat4x4() const;
};
Vector3f axis(Quat q);
float dot(const Quat &lhs, const Quat &rhs);
Quat mix(Quat from, Quat to, float t);

Quat operator+(const Quat &lhs, const Quat &rhs);

Quat operator*(float lhs, const Quat &rhs);
Quat operator*(const Quat lhs, float &rhs);

Vector3f operator*(const Vector3f &lhs, const Quat &rhs);
Vector3f operator*(const Quat &lhs, const Vector3f &rhs);

Quat operator*(const Quat &lhs, const Quat &rhs);

bool operator==(const Quat &left, const Quat &right);
bool operator!=(const Quat &a, const Quat &b);

#endif
