#ifndef MATRIX3x3_H
#define MATRIX3x3_H

#include "vec3.hxx"
#include "mat2.hxx"

#define M3D(aRow, bCol)                 \
        l.rc[aRow][0] * r.rc[0][bCol] + \
        l.rc[aRow][1] * r.rc[1][bCol] + \
        l.rc[aRow][2] * r.rc[2][bCol]   


struct Mat3x3
{
  union
  {

    struct
    {
      float xx; float xy; float xz;
      float yx; float yy; float yz; 
      float zx; float zy; float zz;
    };
    Vector3f rows[3];
    float rc[3][3];

  };

  /// @brief identity matrix
  Mat3x3()
      : xx(0.0), xy(0.0), xz(0.0),
        yx(0.0), yy(0.0), yz(0.0),
        zx(0.0), zy(0.0), zz(0.0) {}

  Mat3x3(float *fv)
      : xx(fv[0]), xy(fv[1]), xz(fv[2]),
        yx(fv[3]), yy(fv[4]), yz(fv[5]),
        zx(fv[6]), zy(fv[7]), zz(fv[8]) {}

  Mat3x3(Vector3f _row1,
         Vector3f _row2,
         Vector3f _row3)
      : rows{_row1, _row2, _row3} {}

  Mat3x3(float _00, float _01, float _02,
         float _10, float _11, float _12,
         float _20, float _21, float _22)
      : xx(_00), xy(_01), xz(_02),
        yx(_10), yy(_11), yz(_12),
        zx(_20), zy(_21), zz(_22) {}

  const Mat3x3 operator+=(float r);
  const Mat3x3 operator-=(float r);
  const Mat3x3 operator*=(float r);

  float determinant() const;
  Mat3x3 inverse() const;
  float cofactor(int a, int b) const;
  Mat2x2 minor(int a, int b) const;
  

  /// @brief from a row-major matrix to a column-major and vice versa
  /// @param m matrix to transpose
  /// @return 
  Mat3x3 transpose() const;
};



Mat3x3 operator+(const Mat3x3 &l, float r);
Mat3x3 operator-(const Mat3x3 &l, float r);
Mat3x3 operator*(const Mat3x3 &l, float r);
Mat3x3 operator*(float l, const Mat3x3 &r);

Vector3f operator*(const Mat3x3 &l, const Vector3f &r);

Mat3x3 operator+(const Mat3x3 &l, const Mat3x3 &r);
Mat3x3 operator-(const Mat3x3 &l, const Mat3x3 &r);
Mat3x3 operator*(const Mat3x3 &l, const Mat3x3 &r);

#endif