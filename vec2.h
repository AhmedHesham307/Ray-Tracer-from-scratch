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

    double length();
    
    vec2 normalize();
    
    double dot(const vec2 other);
    
    double cross(const vec2 other);

    vec2 operator+(vec2 other);
    vec2 operator-(vec2 other);
    vec2 operator*(vec2 other);
    vec2 operator*(double d);
};



#endif