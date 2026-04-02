//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
// home made quaternion math lib cause i have a big ego.
// "john vince - quaternions for for computer graphics" was a massive help along
// with "gabor szauer - hands on c++ game animation programming packt", both
// great books.

#include "headers/quaternion.hxx"
#include "headers/mat3.hxx"
#include "headers/mat4.hxx"
#include "headers/vec3.hxx"
#include <cmath>

Quat::Quat(float angle, Vector3f axis)
{
  float s = std::sin(to_radians(angle / 2.0));
  float c = std::cos(to_radians(angle / 2.0));

  Vector3f unit = axis.unit();

  this->s = c;
  this->x = unit.x * s;
  this->y = unit.y * s;
  this->z = unit.z * s;
}

float Quat::norm() const
{
  float x2 = std::pow(x, 2.0);
  float y2 = std::pow(y, 2.0);
  float z2 = std::pow(z, 2.0);
  float s2 = std::pow(s, 2.0);

  return std::sqrt(x2 + y2 + z2 + s2);
}

Quat Quat::unit() const
{
  float coeff = 1.0 / this->norm();

  return Quat(x * coeff, y * coeff, z * coeff, s * coeff);
}

Quat Quat::conjugate() const { return Quat(-x, -y, -z, s); }

Quat Quat::inverse() const
{
  float lenSqrd = x * x + y * y + z * z + s * s;
  float invLen = 1.0 / lenSqrd;

  return this->conjugate() * invLen;
}

float dot(const Quat &lhs, const Quat &rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.s * rhs.s;
}

Vector3f axis(Quat q) { return Vector3f(q.x, q.y, q.z); }

Quat mix(Quat from, Quat to, float t) { return (1.0 - t) * from + t * to; }

Mat3x3 Quat::toMat3x3() const
{
  float x2 = std::pow(x, 2.0);
  float y2 = std::pow(y, 2.0);
  float z2 = std::pow(z, 2.0);
  
  // Use the same formula as toMat4x4() for consistency
  // This produces a proper rotation matrix where rows are the rotated basis vectors
  return Mat3x3(
      1.0 - 2.0 * (y2 + z2), 2.0 * (x * y - s * z), 2.0 * (x * z + s * y),
      2.0 * (x * y + s * z), 1.0 - 2.0 * (x2 + z2), 2.0 * (y * z - s * x),
      2.0 * (x * z - s * y), 2.0 * (y * z + s * x), 1.0 - 2.0 * (x2 + y2));
}
Mat4x4 Quat::toMat4x4() const
{
  Mat4x4 result = Mat4x4();

  float x2 = std::pow(x, 2.0);
  float y2 = std::pow(y, 2.0);
  float z2 = std::pow(z, 2.0);
  // first row
  result.xx = 1.0 - 2.0 * (y2 + z2);
  result.xy = 2.0 * (x * y - s * z);
  result.xz = 2.0 * (x * z + s * y);
  // second row
  result.yx = 2.0 * (x * y + s * z);
  result.yy = 1.0 - 2.0 * (x2 + z2);
  result.yz = 2.0 * (y * z - s * x);
  // third row
  result.zx = 2.0 * (x * z - s * y);
  result.zy = 2.0 * (y * z + s * x);
  result.zz = 1.0 - 2.0 * (x2 + y2);

  result.ww = 1.0;

  return result;
}
// operator overloads
//________________________________________________________________________
//________________________________________________________________________

Quat operator+(const Quat &lhs, const Quat &rhs)
{
  return Quat(
      lhs.x + rhs.x,
      lhs.y + rhs.y,
      lhs.z + rhs.z,
      lhs.s + rhs.s);
}

Quat operator*(float lhs, const Quat &rhs)
{
  return Quat(
      lhs * rhs.x,
      lhs * rhs.y,
      lhs * rhs.z,
      lhs * rhs.s);
}
Quat operator*(const Quat lhs, float &rhs) { return rhs * lhs; }

Vector3f operator*(const Quat &lhs, const Vector3f &rhs)
{
  Vector3f a = axis(lhs) * 2.0 * dot(axis(lhs), rhs);
  Vector3f b = rhs * (lhs.s * lhs.s - dot(axis(lhs), axis(lhs)));
  Vector3f c = cross(axis(lhs), rhs) * 2.0 * lhs.s;

  return a + b + c;
}

Vector3f operator*(Vector3f &lhs, Quat &rhs) { return rhs * lhs; }

Quat operator*(const Quat &lhs, const Quat &rhs)
{
  Quat result = Quat(0.0);

  result.x = lhs.s * rhs.x + lhs.x * rhs.s + lhs.y * rhs.z - lhs.z * rhs.y;
  result.y = lhs.s * rhs.y + lhs.y * rhs.s + lhs.z * rhs.x - lhs.x * rhs.z;
  result.z = lhs.s * rhs.z + lhs.z * rhs.s + lhs.x * rhs.y - lhs.y * rhs.x;
  result.s = lhs.s * rhs.s - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;

  return result;
}

bool operator==(const Quat &left, const Quat &right)
{
  return (
      fabsf(left.x - right.x)   <= QUAT_EPSILON &&
      fabsf(left.y - right.y) <= QUAT_EPSILON &&
      fabsf(left.z - right.z) <= QUAT_EPSILON &&
      fabsf(left.s - right.s) <= QUAT_EPSILON);
}
bool operator!=(const Quat &a, const Quat &b) { return !(a == b); }
