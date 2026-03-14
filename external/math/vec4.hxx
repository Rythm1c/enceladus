#ifndef VEC4_HPP
#define VEC4_HPP

#include "utils.hxx"

struct Vector4f
{
  union
  {
    struct
    {
      float x;
      float y;
      float z;
      float w;
    };

    struct
    {
      float r; // red
      float g; // green
      float b; // blue
      float a; // alpha
    };

    float v[4];
  };
  // default constuctor
  inline Vector4f()
      : x(0.0), y(0.0), z(0.0), w(0.0) {}
  // set all components to a singular value
  inline Vector4f(float _v)
      : x(_v), y(_v), z(_v), w(_v) {}
  // set each value individialy
  inline Vector4f(float _x, float _y, float _z, float _w)
      : x(_x), y(_y), z(_z), w(_w) {}
  // set the 4d vector using an array
  inline Vector4f(float *_v)
      : x(_v[0]), y(_v[1]), z(_v[2]), w(_v[3]) {}

  Vector4f unit() const;
  float mag() const;
};

typedef Vector4f color4f;

float dot(const Vector4f &l, const Vector4f &r);

Vector4f operator+(const Vector4f &l, const Vector4f &r);
Vector4f operator-(const Vector4f &l, const Vector4f &r);

Vector4f operator*(const Vector4f &l, const Vector4f &r);
Vector4f operator*(const Vector4f &l, float r);
Vector4f operator*(float l, const Vector4f &r);

#endif
