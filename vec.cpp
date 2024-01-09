#include "vec.h"

double vec2::length() const
{
    return std::sqrt(x * x + y * y);
}

vec2 vec2::normalize() const
{
    double length = std::sqrt(x * x + y * y);
    return vec2(x / length, y / length);
}

double vec2::dot(const vec2 other) const
{
    return x * other.x + y * other.y;
}

double vec2::cross(const vec2 other) const
{
    return x * other.y - y * other.x;
}

vec2 vec2::operator+(const vec2 other) const
{
    vec2 result = vec2(x + other.x, y + other.y);
    return result;
}

vec2 vec2::operator-(const vec2 other) const
{
    vec2 result = vec2(x - other.x, y - other.y);
    return result;
}

vec2 vec2::operator*(const vec2 other) const
{
    vec2 result = vec2(x * other.x, y * other.y);
    return result;
}

vec2 vec2::operator*(const double d) const
{
    vec2 result = vec2(x * d, y * d);
    return result;
}

double vec3::length() const
{
    return std::sqrt(dot(*this));
}

double vec3::dot(vec3 other) const
{
    return x * x + y * y + z * z;
}

vec3 vec3::normalize() const
{
    return *this * (1 / length());
}

vec3 vec3::operator+(const vec3 other) const{
    return vec3(x + other.x, y + other.y, z + other.z);
}
vec3 vec3::operator-(const vec3 other) const{
    return vec3(x - other.x, y - other.y, z - other.z);
}
vec3 vec3::operator*(const vec3 other) const{
    return vec3(x * other.x, y * other.y, z * other.z);
}
vec3 vec3::operator*(const double d) const{
    return vec3(x * d, y * d, z * d);
}
