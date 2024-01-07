#ifndef VEC2
#define VEC2
#include <cmath>

/*
using struct here because of ISO Cpp C.2 guideline:
The components of a vector can vary independently without producing something
like an "invalid vector".
*/
struct vec2
{
    double x;
    double y;
    public:
    vec2(double x_val, double y_val) : x{x_val}, y{y_val} {}

    double length() const;
    
    vec2 normalize() const;
    
    double dot(const vec2 other) const;
    
    double cross(const vec2 other) const;

    vec2 operator+(const vec2 other) const;
    vec2 operator-(const vec2 other) const;
    vec2 operator*(const vec2 other) const;
    vec2 operator*(const double d) const;
};

using point2 = vec2;
#endif