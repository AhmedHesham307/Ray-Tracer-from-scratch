#include "scene.h"

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
        return Collision(-1, vec2(0, 0), false);
    vec2 intersection_point(0, 0);
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

vec2 Camera::view_dir(double image_space_x) const
{
    double dir_angle = std::atan(direction.y / direction.x);
    double y = -image_space_x * sensor_size + .5 * sensor_size;
    vec2 local = vec2(focal_length, y).normalize();

    vec2 world_space_view_dir(std::cos(dir_angle) * local.x - std::sin(dir_angle) * local.y,
                              std::sin(dir_angle) * local.x + std::cos(dir_angle) * local.y);

    return world_space_view_dir;
}

void Camera::outpainting(std::vector<double> depth, std::vector<bool> hits, std::vector<std::vector<bool>> &out_hits)
{
    uint width = out_hits.at(0).size();
    uint height = out_hits.size();

    double object_height = 1;
    double image_plane_width = sensor_size / focal_length;
    double image_plane_height = image_plane_width / (4. / 3);
    // std::cout << image_plane_height << std::endl;

    for (int i = 0; i < width; i++)
    {
        if (hits.at(i))
        {
            double image_space_x = i / static_cast<double>(width) + .5 / width;
            double image_plane_vector_y = -image_space_x * sensor_size + .5 * sensor_size;
            double image_plane_distance = vec2(focal_length, image_plane_vector_y).length() / focal_length;

            double apparent_height = object_height / depth.at(i) * image_plane_distance;

            for (int j = 0; j < height; j++)
            {
                // pixel position in 0 to 1 image space
                double image_space_y = j / static_cast<double>(height) + .5 / height;
                double off_center = std::abs(.5 - image_space_y) * image_plane_height;
                if (off_center < apparent_height)
                    out_hits.at(j).at(i) = true;
            }
        }
    }
}

vec2 Camera::forward_vec(){
    return direction;
}

vec2 Camera::right_vec(){
    return vec2(direction.y, -direction.x);
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
    double current_angle = std::atan(direction.y / direction.x);
    double new_angle = current_angle + angle;
    direction = vec2(std::cos(new_angle), std::sin(new_angle));
}