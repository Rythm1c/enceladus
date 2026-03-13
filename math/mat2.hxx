#ifndef MATRIX2x2_H
#define MATRIX2x2_H

#include "vec2.hxx"

struct Mat2x2
{
  union
  {
    struct
    {
      float xx; float xy;
      float yx; float yy;
    };

    float rc[2][2];

    Vector2f rows[2];
  };

  Mat2x2()
      : xx(0.0), xy(0.0),
        yx(0.0), yy(0.0) {}

  Mat2x2(float _00, float _01,
         float _10, float _11)
      : xx(_00), xy(_01),
        yx(_10), yy(_11) {}

  Mat2x2(Vector2f row1,
         Vector2f row2)
      : rows{row1, row2} {}

  float determinant() const;
};

#endif