#include "vec4.hxx"

float Vector4f::mag() const
{
  float x2 = this->x * this->x;
  float y2 = this->y * this->y;
  float z2 = this->z * this->z;
  float w2 = this->w * this->w;

  return sqrt(x2 + y2 + z2 + w2);
}

Vector4f Vector4f::unit() const
{
  float invMag = 1.0 / this->mag();

  return Vector4f(
      this->x * invMag,
      this->y * invMag,
      this->z * invMag,
      this->w * invMag);
}

float dot(const Vector4f &l, const Vector4f &r)
{
  return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
}

Vector4f operator+(const Vector4f &l, const Vector4f &r)
{
  return Vector4f(
      l.x + r.x,
      l.y + r.y,
      l.z + r.z,
      l.w + r.w);
}
Vector4f operator-(const Vector4f &l, const Vector4f &r)
{
  return Vector4f(
      l.x - r.x,
      l.y - r.y,
      l.z - r.z,
      l.w - r.w);
}

Vector4f operator*(const Vector4f &l, const Vector4f &r)
{
  return Vector4f(
      l.x * r.x,
      l.y * r.y,
      l.z * r.z,
      l.w * r.w);
}
Vector4f operator*(const Vector4f &l, float r)
{
  return Vector4f(
      l.x * r,
      l.y * r,
      l.z * r,
      l.w * r);
}
Vector4f operator*(float l, const Vector4f &r)
{
  return Vector4f(
      l * r.x,
      l * r.y,
      l * r.z,
      l * r.w);
}