#include "headers/vec3.hxx"

/*function definitions for vector operations*/

float dot(const Vector3f &p1, const Vector3f &p2)
{
  return (p1.x * p2.x) + (p1.y * p2.y) + (p1.z * p2.z);
}
float Vector3f::magSqrd() const
{
  float x2 = pow(this->x, 2.0f);
  float y2 = pow(this->y, 2.0f);
  float z2 = pow(this->z, 2.0f);

  return (x2 + y2 + z2);
}
float Vector3f::mag() const { return sqrt(this->magSqrd()); }

Vector3f cross(const Vector3f &p1, const Vector3f &p2)
{
  return Vector3f(
      p1.y * p2.z - p1.z * p2.y,
      p1.z * p2.x - p1.x * p2.z,
      p1.x * p2.y - p1.y * p2.x);
}

Vector3f Vector3f::unit() const
{

  float invMag = 1.0f / this->mag();

  return Vector3f(
      invMag * this->x,
      invMag * this->y,
      invMag * this->z);
}
Vector3f reflect(const Vector3f &v, const Vector3f &n)
{
  Vector3f v_new = -2.0f * n * dot(n, v) + v;
  return v_new;
}
Vector3f clamp(const Vector3f &v, const Vector3f &min, const Vector3f &max)
{
  Vector3f v_new = v;
  v_new.x = clamp(v_new.x, min.x, max.x);
  v_new.y = clamp(v_new.y, min.y, max.y);
  v_new.z = clamp(v_new.z, min.z, max.z);

  return v_new;
}
Vector3f lerp(Vector3f a, Vector3f b, float c) { return (1.0 - c) * a + c * b; }

// miltiplication
Vector3f operator*(const Vector3f &l, float r) { return Vector3f(l.x * r, l.y * r, l.z * r); }
Vector3f operator*(float r, const Vector3f &l) { return Vector3f(l.x * r, l.y * r, l.z * r); }
Vector3f operator*(const Vector3f &lhs, const Vector3f &rhs)
{
  return Vector3f(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}
// addition
Vector3f operator+(const Vector3f &l, const Vector3f &r)
{
  return Vector3f(l.x + r.x, l.y + r.y, l.z + r.z);
}
// subtraction
Vector3f operator-(const Vector3f &l, const Vector3f &r)
{
  return Vector3f(l.x - r.x, l.y - r.y, l.z - r.z);
}
// comparisons
bool operator==(const Vector3f &l, const Vector3f &r)
{
  return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
}
bool operator!=(const Vector3f &l, const Vector3f &r) { return !(l == r); }
