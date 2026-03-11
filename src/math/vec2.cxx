#include "vec2.hxx"

float Vector2f::length() const
{
  float x2 = this->x * this->x;
  float y2 = this->y * this->y;
  return sqrt(x2 + y2);
}

Vector2f Vector2f::unit() const
{
  float invLen = 1.0 / this->length();
  return Vector2f(
      invLen * this->x,
      invLen * this->y);
}

float dot(const Vector2f &a, const Vector2f &b)
{
  return a.x * b.x + a.y * b.y;
}

Vector2f operator+(const Vector2f &l, const Vector2f &r)
{
  return Vector2f(l.x + r.x, l.y + r.y);
}

Vector2f operator-(const Vector2f &l, const Vector2f &r)
{
  return Vector2f(l.x - r.x, l.y - r.y);
}

Vector2f operator*(const Vector2f &l, float r)
{
  return Vector2f(l.x * r, l.y * r);
}
Vector2f operator*(float l, const Vector2f &r) { return r * l; }

bool operator==(const Vector2f &l, const Vector2f &r)
{
  return (l.x == r.x) && (l.y == r.y);
}
