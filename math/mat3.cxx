#include "headers/mat3.hxx"

Mat3x3 Mat3x3::transpose() const
{
  return Mat3x3(
      this->xx, this->yx, this->zx,
      this->xy, this->yy, this->zy,
      this->xz, this->yz, this->zz);
}

float Mat3x3::determinant() const
{
  const float i = this->xx * (this->yy * this->zz - this->yz * this->zy);
  const float j = this->xy * (this->yx * this->zz - this->yz * this->zx);
  const float k = this->xz * (this->yx * this->zy - this->yy * this->zx);

  return (i - j + k);
}

Mat2x2 Mat3x3::minor(int a, int b) const
{
  Mat2x2 _minor;
  int _yy = 0;
  for (int y = 0; y < 3; y++)
  {
    if (y == b)
      continue;

    int _xx = 0;
    for (int x = 0; x < 3; x++)
    {
      if (x == a)
        continue;

      _minor.rc[_xx][_yy] = this->rc[x][y];
      _xx++;
    }
    _yy++;
  }

  return _minor;
}

float Mat3x3::cofactor(int a, int b) const
{
  const Mat2x2 minor = this->minor(a, b);
  const float c = float(pow(-1, a + 1 + b + 1)) * minor.determinant();

  return c;
}

Mat3x3 Mat3x3::inverse() const
{
  Mat3x3 result;

  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      result.rc[i][j] = this->cofactor(i, j);
    }
  }

  float det = this->determinant();
  float invDet = 1.0 / det;
  result *= invDet;
  result = result.transpose();

  return result;
}

Mat3x3 operator+(const Mat3x3 &l, float r)
{

  return Mat3x3(
      l.rows[0] + r,
      l.rows[1] + r,
      l.rows[2] + r);
}
const Mat3x3 Mat3x3::operator+=(float r)
{
  *this = *this + r;
  return *this;
}

Mat3x3 operator-(const Mat3x3 &l, float r)
{

  return Mat3x3(
      l.rows[0] - r,
      l.rows[1] - r,
      l.rows[2] - r);
}
const Mat3x3 Mat3x3::operator-=(float r)
{
  *this = *this - r;
  return *this;
}

Mat3x3 operator*(const Mat3x3 &l, float r)
{
  return Mat3x3(
      l.rows[0] * r,
      l.rows[1] * r,
      l.rows[2] * r);
}
Mat3x3 operator*(float l, const Mat3x3 &r) { return r * l; }
const Mat3x3 Mat3x3::operator*=(float r)
{
  *this = *this * r;
  return *this;
}

Vector3f operator*(const Mat3x3 &l, const Vector3f &r)
{
  return Vector3f(
      dot(l.rows[0], r),
      dot(l.rows[1], r),
      dot(l.rows[2], r));
}

Mat3x3 operator+(const Mat3x3 &l, const Mat3x3 &r)
{
  return Mat3x3(
      l.rows[0] + r.rows[0],
      l.rows[1] + r.rows[1],
      l.rows[2] + r.rows[2]);
}

Mat3x3 operator-(const Mat3x3 &l, const Mat3x3 &r)
{
  return Mat3x3(
      l.rows[0] - r.rows[0],
      l.rows[1] - r.rows[1],
      l.rows[2] - r.rows[2]);
}

Mat3x3 operator*(const Mat3x3 &l, const Mat3x3 &r)
{

  return Mat3x3(
      M3D(0, 0), M3D(0, 1), M3D(0, 2), //
      M3D(1, 0), M3D(1, 1), M3D(1, 2), //
      M3D(2, 0), M3D(2, 1), M3D(2, 2));
}