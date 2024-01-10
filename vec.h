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

    static vec2 lerp(const vec2 first, const vec2 second, double d);

    vec2 reflect(const vec2 normal) const;
};

using point2 = vec2;

struct vec3
{
    double x;
    double y;
    double z;
    public:
    vec3(double x_val, double y_val, double z_val) : x{x_val}, y{y_val}, z{z_val}{}

    double length() const;
    
    vec3 normalize() const;
    
    double dot(const vec3 other) const;

    vec3 operator+(const vec3 other) const;
    vec3 operator-(const vec3 other) const;
    vec3 operator*(const vec3 other) const;
    vec3 operator*(const double d) const;

    static vec3 lerp(const vec3 first, const vec3 second, double d);
};

using RGB = vec3;
#endif