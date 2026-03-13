#include "mat4.hxx"
#include "quaternion.hxx"
#include <cmath>

Mat4x4 identity()
{
  return Mat4x4(
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0);
}

Mat4x4 translate(const Vector3f t)
{
  Mat4x4 trans;
  trans.xw = t.x;
  trans.yw = t.y;
  trans.zw = t.z;
  return trans;
}
Mat4x4 scale(const Vector3f s)
{
  Mat4x4 trans;
  trans.xx = s.x;
  trans.yy = s.y;
  trans.zz = s.z;
  return trans;
}
Mat4x4 rotationX(float angle)
{
  Mat4x4 trans;
  float rads = to_radians(angle);
  trans.xx = 1.0f;
  trans.yy = cos(rads);
  trans.yz = -sin(rads);
  trans.zy = sin(rads);
  trans.zz = cos(rads);
  trans.ww = 1.0f;
  return trans;
}
Mat4x4 rotationY(float angle)
{
  Mat4x4 trans;
  float rads = to_radians(angle);
  trans.xx = cos(rads);
  trans.xz = sin(rads);
  trans.yy = 1.0f;
  trans.zx = -sin(rads);
  trans.zz = cos(rads);
  trans.ww = 1.0f;
  return trans;
}
Mat4x4 rotationZ(float angle)
{
  Mat4x4 trans;
  float rads = to_radians(angle);
  trans.xx = cos(rads);
  trans.xy = sin(rads);
  trans.yx = -sin(rads);
  trans.yy = cos(rads);
  trans.zz = 1.0f;
  trans.ww = 1.0f;
  return trans;
}

Mat4x4 Mat4x4::transpose() const
{
  return Mat4x4(
      this->xx, this->yx, this->zx, this->wx,
      this->xy, this->yy, this->zy, this->wy,
      this->xz, this->yz, this->zz, this->wz,
      this->xw, this->yw, this->zw, this->ww);
}

Mat4x4 look_at(const Vector3f &pos, const Vector3f &fr, const Vector3f &up)
{
  // cr = camera right vector
  Vector3f cd = (pos - fr).unit();
  Vector3f cr = (cross(up, cd)).unit();
  Vector3f cu = (cross(cd, cr)).unit();

  // rotation and translation matrix combined
  float xx = cr.x;
  float xy = cr.y;
  float xz = cr.z;
  float xw = -pos.x * cr.x - pos.y * cr.y - pos.z * cr.z;

  float yx = cu.x;
  float yy = cu.y;
  float yz = cu.z;
  float yw = -pos.x * cu.x - pos.y * cu.y - pos.z * cu.z;

  float zx = cd.x;
  float zy = cd.y;
  float zz = cd.z;
  float zw = -pos.x * cd.x - pos.y * cd.y - pos.z * cd.z;

  return Mat4x4(
      xx, xy, xz, xw,
      yx, yy, yz, yw,
      zx, zy, zz, zw,
      0.0, 0.0, 0.0, 1.0);
}

Quat Mat4x4::toQuat() const
{

  float x = 0.0;
  float y = 0.0;
  float z = 0.0;
  float s = 0.5 * std::sqrt(1.0 + this->xx + this->yy + this->zz);

  if (s > 0.0)
  {
    float coeff = 1.0 / (4.0 * s);
    x = coeff * (this->zy - this->yz);
    y = coeff * (this->xz - this->zx);
    z = coeff * (this->yx - this->xy);
    return Quat(x, y, z, s);
  }

  x = 0.5 * std::sqrt(1.0 + this->xx - this->yy - this->zz);
  if (x > 0.0)
  {
    float coeff = 1.0 / (4.0 * x);
    y = coeff * (this->xy + this->yx);
    z = coeff * (this->xz + this->zx);
    s = coeff * (this->zy - this->yz);
    return Quat(x, y, z, s);
  }

  y = 0.5 * std::sqrt(1.0 - this->xx + this->yy - this->zz);
  if (y > 0.0)
  {
    float coeff = 1.0 / (4.0 * y);
    x = coeff * (this->xy + this->yx);
    z = coeff * (this->yz + this->zy);
    s = coeff * (this->xz - this->zx);
    return Quat(x, y, z, s);
  }
  // if all else fails just use z
  z = 0.5 * std::sqrt(1.0 - this->xx - this->yy + this->zz);
  float coeff = 1.0 / (4.0 * z);
  x = coeff * (this->xz + this->zx);
  y = coeff * (this->yz + this->zy);
  s = coeff * (this->yx - this->xy);
  return Quat(x, y, z, s);
}

Mat4x4 orthogonal(float l, float r, float b, float t, float n, float f)
{
  Mat4x4 proj;
  proj.xx = 2.0f / (r - l);
  proj.xw = -((r + l) / (r - l));
  proj.yy = 2.0f / (t - b);
  proj.yw = -((t + b) / (t - b));
  proj.zz = -2.0f / (f - n);
  proj.zw = -((f + n) / (f - n));
  proj.ww = 1.0f;

  return proj;
}

Mat4x4 frustrum(float l, float r, float b, float t, float n, float f)
{
  Mat4x4 proj;
  proj.xx = (2.0f * n) / (r - l);
  proj.xz = (r + l) / (r - l);
  proj.yy = (2.0f * n) / (t - b);
  proj.yz = (t + b) / (t - b);
  proj.zz = -(f + n) / (f - n);
  proj.zw = (-2.0f * f * n) / (f - n);
  proj.wz = -1.0f;
  proj.ww = 0.0f;
  return proj;
}

Mat4x4 perspective(float fov, float aspectRatio, float N, float F)
{

  float ymax = N * tan((to_radians(fov / 2.0f)));
  float xmax = ymax * aspectRatio;

  return frustrum(-xmax, xmax, -ymax, ymax, N, F);
}
Mat4x4 operator*(const Mat4x4 &l, float r)
{
  return Mat4x4(
      l.rows[0] * r,
      l.rows[1] * r,
      l.rows[2] * r,
      l.rows[3] * r);
}
Mat4x4 operator*(float l, const Mat4x4 &r)
{
  return Mat4x4(
      r.rows[0] * l,
      r.rows[1] * l,
      r.rows[2] * l,
      r.rows[3] * l);
}

Vector4f operator*(const Mat4x4 &m, const Vector4f &v)
{
  return Vector4f(
      dot(m.rows[0], v),
      dot(m.rows[1], v),
      dot(m.rows[2], v),
      dot(m.rows[3], v));
}
Mat4x4 operator*(const Mat4x4 &l, const Mat4x4 &r)
{
  return Mat4x4(
      M4D(0, 0), M4D(0, 1), M4D(0, 2), M4D(0, 3),
      M4D(1, 0), M4D(1, 1), M4D(1, 2), M4D(1, 3),
      M4D(2, 0), M4D(2, 1), M4D(2, 2), M4D(2, 3),
      M4D(3, 0), M4D(3, 1), M4D(3, 2), M4D(3, 3));
}

Mat4x4 operator+(const Mat4x4 &l, const Mat4x4 &r)
{
  return Mat4x4(
      l.rows[0] + r.rows[0],
      l.rows[1] + r.rows[1],
      l.rows[2] + r.rows[2],
      l.rows[3] + r.rows[3]);
}
Mat4x4 operator-(const Mat4x4 &l, const Mat4x4 &r)
{
  return Mat4x4(
      l.rows[0] - r.rows[0],
      l.rows[1] - r.rows[1],
      l.rows[2] - r.rows[2],
      l.rows[3] - r.rows[3]);
}
bool operator==(const Mat4x4 &l, const Mat4x4 &r)
{
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
    {
      if (l.rc[i][j] != r.rc[i][j])
      {
        return false;
      }
    }

  return true;
}
bool operator!=(const Mat4x4 &l, const Mat4x4 &r) { return !(l == r); }
