#include "scene.h"
/*
Collision Wall::intersect(ray r) const
{
    

    // check determinant so we do not attempt to solve an unsolvable system (otherwise divide by zero will occur)
    double det = (r.direction.x * (-direction.y) - (-direction.x * r.direction.y));
    if (det == 0)
        return Collision(-1, vec2(0, 0), false);

    vec2 rhs = r.origin * (-1.) + origin;

    double first_step_fraction = r.direction.y / r.direction.x;

    double p2 = (rhs.y - first_step_fraction * rhs.x) / (-direction.y + first_step_fraction * direction.x);

    double p1 = (rhs.x + p2 * direction.x) / r.direction.x;

    bool hit = p1 > 0 && p2 >= 0 && p2 <= upper_bound;

    vec2 normal(direction.y, -direction.x);
    if (normal.dot(r.direction) > 0)
        normal = normal * (-1.);

    return Collision(r.direction.length() * p1, normal, hit);
}

Collision Circle::intersect(ray r) const
{
    vec2 z = r.origin + center * (-1);

    double a = r.direction.x * r.direction.x + r.direction.y * r.direction.y;

    double b = 2 * (r.direction.x * z.x + r.direction.y * z.y);

    double c = z.dot(z) - radius * radius;

    double det = b * b - 4 * a * c;

    double p = -1;

    if (det < 0)
        return Collision(-1, vec3(0, 0, 0), false);
    vec3 intersection_point(0, 0, 0);
    if (det == 0)
        intersection_point = r.origin + r.direction * (-b / (2 * a));
    else
    {
        double p1 = (-b + sqrt(det)) / (2 * a);
        double p2 = (-b - sqrt(det)) / (2 * a);

        p = p1 < p2 ? p1 : p2;
        intersection_point = r.origin + r.direction * p;
    }

    return Collision(p * r.direction.length(), intersection_point + center * (-1), p > 0);
}
*/

vec3 Camera::view_dir(double image_space_x, double image_space_y) const
{
    double dir_angle = std::atan2(direction.y, direction.x);
    double x = image_space_x * sensor_size - .5 * sensor_size;
    double y = -image_space_y * sensor_size + .5 * sensor_size;
    vec3 local = vec3(x, y, focal_length).normalize();

    vec3 world_space_view_dir = local_to_world(local);
    return world_space_view_dir;
}

vec3 Camera::forward_vec() const{
    return direction.normalize();
}

vec3 Camera::right_vec() const{
    return direction.cross(up).normalize();
}

vec3 Camera::up_vec() const{
    return right_vec().cross(direction).normalize();
}

vec3 Camera::local_to_world(const vec3 v) const{
    // just a basis transformation
    return vec3(ortho_r_cache.x * v.x + ortho_u_cache.x * v.y + ortho_f_cache.x * v.z,
                ortho_r_cache.y * v.x + ortho_u_cache.y * v.y + ortho_f_cache.y * v.z,
                ortho_r_cache.z * v.x + ortho_u_cache.z * v.y + ortho_f_cache.z * v.z);
}


void Camera::forward(){
    position = position + forward_vec() * movement_speed;
}

void Camera::backward(){
    position = position - forward_vec() * movement_speed;
}

void Camera::right(){
    position = position + right_vec() * movement_speed;
}

void Camera::left(){
    position = position - right_vec() * movement_speed;
}

void Camera::rotate(double angle){
    double current_angle = std::atan2(direction.y, direction.x);
    double new_angle = current_angle + angle;
    direction = vec3(std::cos(new_angle), std::sin(new_angle), 0);
}