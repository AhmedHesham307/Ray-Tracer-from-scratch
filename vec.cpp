#include "vec.h"
// boring vector operator implementations
double vec3::length() const
{
    return std::sqrt(dot(*this));
}

double vec3::dot(const vec3 other) const
{
    return x * other.x + y * other.y + z * other.z;
}
vec3 vec3::cross(const vec3 other) const
{
    return vec3(y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
}

vec3 vec3::normalize() const
{
    return *this / length();
}

vec3 vec3::operator+(const vec3 other) const{
    return vec3(x + other.x, y + other.y, z + other.z);
}
vec3 vec3::operator-() const{
    return vec3(-x, -y, -z);
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
vec3 vec3::operator/(const double t) const{
    return vec3(x / t, y / t, z / t);
}

vec3 vec3::linear_interp(vec3 first, vec3 second, double d){
    return vec3(first.x + d * (second.x - first.x),
                first.y + d * (second.y - first.y),
                first.z + d * (second.z - first.z));
}

vec3 vec3::reflect(const vec3 normal) const{
    vec3 normalized_normal = normal.normalize();
    vec3 normalized_vec = this->normalize();

    double normal_component = 2 * normalized_vec.dot(normalized_normal);
    return normalized_vec - normalized_normal * normal_component;
}

void vec3::print()const{
        std::cout << x << " ";
        std::cout << y << " ";
        std::cout << z << " ";
}