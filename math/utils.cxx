#include "utils.hxx"
#include <cstdlib>
#include <cmath>

float to_radians(float degs)
{
  float radian = PIE / 180.0f;
  return degs * radian;
}
float to_degrees(float rads)
{
  float scale = 180.0f / PIE;
  return rads * scale;
}
// random float number generator
float random_float() { return (float)(rand()) / (float)(RAND_MAX); }
// random integer generator
// integers within range a-b
int random_int(int a, int b)
{
  if (a > b)
    return random_int(b, a);
  if (a == b)
    return a;
  return a + (rand() % (b - a));
}
// random float generator
// floats within range a-b
float random_float(int a, int b)
{
  if (a > b)
    return random_float(b, a);
  if (a == b)
    return b;
  return (float)(random_int(a, b)) + random_float();
}
// return the the largest of the two floats
float max(float a, float b) { return a < b ? b : a; }
// return the smallest of the two floats
float min(float a, float b) { return a < b ? a : b; }

int step(float edge, float b) { return b > edge ? 1 : 0; }

float fract(float value) { return value - floor(value); }

template <class T>
T clamp(T v, T min, T max)
{
  if (v < min)
  {
    return min;
  }
  if (v > max)
  {
    return max;
  }
  return v;
  // return std::max(min, std::min(max, v));
}
template float clamp<float>(float v, float min, float max);
template int clamp<int>(int v, int min, int max);