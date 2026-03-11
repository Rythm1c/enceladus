#ifndef VEC2_HPP
#define VEC2_HPP

#include "utils.hxx"

struct Vector2f
{
  union
  {
    struct
    {
      float x;
      float y;
    };

    float v[2];
  };
  // default constructor
  Vector2f()
      : x(0.0), y(0.0) {}
  // set all components to a single value
  Vector2f(float _v)
      : x(_v), y(_v) {}
  // set values individually
  Vector2f(float _x, float _y)
      : x(_x), y(_y) {}
  // set values using array
  Vector2f(float *_v)
      : x(_v[0]), y(_v[1]) {}

  float length() const;
  Vector2f unit() const;
};

typedef Vector2f Point2f;

float dot(const Vector2f &a, const Vector2f &b);

Vector2f operator+(const Vector2f &l, const Vector2f &r);
Vector2f operator-(const Vector2f &l, const Vector2f &r);

Vector2f operator*(const Vector2f &l, float r);
Vector2f operator*(float l, const Vector2f &r);

bool operator==(const Vector2f &l, const Vector2f &r);

#endif
