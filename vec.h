#ifndef VEC3
#define VEC3
#include <cmath>
#include <iostream>


/*
using struct here because of ISO Cpp C.2 guideline:
The components of a vector can vary independently without producing something
like an "invalid vector".
*/
class vec3
{
    public :
    double x,y,z;

    vec3(): x{0},y{0},z{0}{}
    vec3(double x_val, double y_val, double z_val) : x{x_val}, y{y_val}, z{z_val}{}

    double length() const;
    double length_squared() const;
    
    vec3 normalize() const;
    
    vec3 operator+(const vec3 other) const;
    vec3 operator-() const;   
    vec3 operator-(const vec3 other) const;
    vec3 operator*(const vec3 other) const;
    vec3 operator*(const double d) const;
    vec3 operator/(const double t) const;
    void print()const;

    static vec3 linear_interp(const vec3 first, const vec3 second, double d);
    static vec3 reflect(const vec3 v , const vec3 normal) ;
    static double dot(const vec3 v1,const vec3 v2);
    static vec3 cross(const vec3 &u, const vec3 &v);
};

using point3 = vec3;
using RGB = vec3;
#endif