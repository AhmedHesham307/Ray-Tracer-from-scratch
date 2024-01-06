#include "vec2.h"

double vec2::length()
{
    return std::sqrt(x * x + y * y);
}

vec2 vec2::normalize()
{
    double length = std::sqrt(x * x + y * y);
    return vec2(x / length, y / length);
}

double vec2::dot(const vec2 other)
{
    return x * other.x + y * other.y;
}

double vec2::cross(const vec2 other)
{
    return x * other.y - y * other.x;
}

vec2 vec2::operator+(const vec2 other)
{
    vec2 result = vec2(x + other.x, y + other.y);
    return result;
}

vec2 vec2::operator-(const vec2 other)
{
    vec2 result = vec2(x - other.x, y - other.y);
    return result;
}

vec2 vec2::operator*(const vec2 other)
{
    vec2 result = vec2(x  * other.x, y * other.y);
    return result;
}

vec2 vec2::operator*(double d)
{
    vec2 result = vec2(x * d, y * d);
    return result;
}

