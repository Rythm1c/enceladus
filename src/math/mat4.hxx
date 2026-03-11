#ifndef MATRIX4X4_H
#define MATRIX4X4_H

#include "utils.hxx"
#include "vec3.hxx"
#include "vec4.hxx"

// for multiplication of two 4x4 mats
#define M4D(aRow, bCol)               \
      l.rc[aRow][0] * r.rc[0][bCol] + \
      l.rc[aRow][1] * r.rc[1][bCol] + \
      l.rc[aRow][2] * r.rc[2][bCol] + \
      l.rc[aRow][3] * r.rc[3][bCol]

struct Mat4x4
{
  union
  {
    struct
    {
      //        column 1  column 2  column 3  column 4
      /*row 1*/ float xx; float xy; float xz; float xw;
      /*row 2*/ float yx; float yy; float yz; float yw;
      /*row 3*/ float zx; float zy; float zz; float zw;
      /*row 4*/ float wx; float wy; float wz; float ww;
    };
    Vector4f rows[4];
    float rc[4][4];
  };
  // default constructor
  // set identity matrix
  Mat4x4()
      : xx(0.0f), xy(0.0f), xz(0.0f), xw(0.0f),
        yx(0.0f), yy(0.0f), yz(0.0f), yw(0.0f),
        zx(0.0f), zy(0.0f), zz(0.0f), zw(0.0f),
        wx(0.0f), wy(0.0f), wz(0.0f), ww(0.0f) {}
  // construct matrix using an array
  Mat4x4(const float *fv)
      : xx(fv[0]), xy(fv[1]), xz(fv[2]), xw(fv[3]),
        yx(fv[4]), yy(fv[5]), yz(fv[6]), yw(fv[7]),
        zx(fv[8]), zy(fv[9]), zz(fv[10]), zw(fv[11]),
        wx(fv[12]), wy(fv[13]), wz(fv[14]), ww(fv[15]) {}

  Mat4x4(
      float _00, float _01, float _02, float _03,
      float _10, float _11, float _12, float _13,
      float _20, float _21, float _22, float _23,
      float _30, float _31, float _32, float _33)
      : xx(_00), xy(_01), xz(_02), xw(_03),
        yx(_10), yy(_11), yz(_12), yw(_13),
        zx(_20), zy(_21), zz(_22), zw(_23),
        wx(_30), wy(_31), wz(_32), ww(_33) {}

  Mat4x4(
      Vector4f row1,
      Vector4f row2,
      Vector4f row3,
      Vector4f row4)
      : rows{row1, row2, row3, row4} {}

  struct Quat toQuat() const;
  /// @brief from a row-major matrix to a column-major and vice versa
  /// @return 
  Mat4x4 transpose() const;
};


Mat4x4 identity();
/// @brief create a translation matrix out of a vec3
/// @param t translation vector
/// @return translation mat
Mat4x4 translate(const Vector3f t);

/// @brief create a scaling matrix out of a vec3
/// @param s scaling vector
/// @return scaling matrix
Mat4x4 scale(const Vector3f s);

/// @brief create a rotation matrix for the X axis
/// @param angle rotational angle
/// @return rotation mat for the x axis
Mat4x4 rotationX(float angle);

/// @brief create a rotation matrix for the Y axis
/// @param angle rotational angle
/// @return rotation mat for the y axis
Mat4x4 rotationY(float angle);

/// @brief create a rotation matrix for the Z axis
/// @param angle rotational angle
/// @return rotation mat for the z axis
Mat4x4 rotationZ(float angle);

/// @brief create a view matrix from camera rotation
/// @param pos camera pos
/// @param dir camera direction
/// @param up camera upwards direction
/// @return rotation matrix for the world relative to camera
Mat4x4 look_at(const Vector3f &pos, const Vector3f &dir, const Vector3f &up);

/// @brief for creating an orthogonal projection matrix using dimentions
/// @param l left
/// @param r right
/// @param b bottom
/// @param t top
/// @param n near
/// @param f far
/// @return projection mat
Mat4x4 orthogonal(float l, float r, float b, float t, float n = -1.0f, float f = 1.0f);

/// @brief for creating a perspective projection
/// @param fov field of view
/// @param aspectRation width/height
/// @param N near 
/// @param F far
/// @return perspective mat
Mat4x4 perspective(float fov, float aspectRation, float N, float F);
Mat4x4 frustrum(float l, float r, float t, float b, float n, float f);

// multiplication operations
Mat4x4 operator*(const Mat4x4 &l, float r);
Mat4x4 operator*(float l, const Mat4x4 &r);
Mat4x4 operator*(const Mat4x4 &l, const Mat4x4 &r);
Vector4f operator*(const Mat4x4 &m, const Vector4f &v);
// addition operations
Mat4x4 operator+(const Mat4x4 &l, const Mat4x4 &r);
// sutraction operations
Mat4x4 operator-(const Mat4x4 &l, const Mat4x4 &r);
// comparison operations
bool operator==(const Mat4x4 &m1, const Mat4x4 &m2);
bool operator!=(const Mat4x4 &m1, const Mat4x4 &m2);

#endif