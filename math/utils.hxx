#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <math.h>

#define PIE 3.141592f
#define VEC3_EPSILON 0.000001f

// convert degrees to radians
float to_radians(float degs);
// convert radians to degrees
float to_degrees(float rads);
float random_float();
float random_float(int a, int b);
int random_int(int a, int b);
float max(float a, float b);
float min(float a, float b);
int step(float edge, float b);
float fract(float value);
// limits a value to the range min - max
template <class T>
T clamp(T v, T min, T max);

#endif