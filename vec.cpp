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

vec2 vec2::lerp(vec2 first, vec2 second, double d){
    return vec2(first.x + d * (second.x - first.x), first.y + d * (second.y - first.y));
}

vec2 vec2::reflect(const vec2 normal) const{
    vec2 normalized_normal = normal.normalize();
    vec2 normalized_vec = this->normalize();

    double normal_component = 2 * normalized_vec.dot(normalized_normal);
    return normalized_vec - normalized_normal * normal_component;
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

vec3 vec3::lerp(vec3 first, vec3 second, double d){
    return vec3(first.x + d * (second.x - first.x), first.y + d * (second.y - first.y), first.z + d * (second.z - first.z));
}


