#include "scene.h"
Collision Wall::intersect(ray r) const
{
    // Calculate the denominator of the parametric equation
    double denominator = normal.dot(r.direction);

    // Calculate the parameter t for the intersection point
    double t = (position - r.origin).dot(normal) / denominator;
    
    // Check if the intersection point is in front of the ray
    if (t > 0) {
        // Calculate the intersection point
        vec3 intersection_point = r.origin + r.direction * t;

        // Calculate the basis vectors for the local coordinate system of the wall
        vec3 wallRight = (normal.cross(vec3(0, 0, 1))).normalize();
        vec3 wallUp = (wallRight.cross(normal)).normalize();

        // Calculate the vector from the wall position to the intersection point
        vec3 wallToIntersection = intersection_point - position;

        // Project the wallToIntersection vector onto the wall's local coordinate system
        double projectionX = wallToIntersection.dot(wallRight);
        double projectionY = wallToIntersection.dot(wallUp);

        // Check if the intersection point is within the bounds of the wall
        if (projectionX >= 0 && projectionX <= length && projectionY >= 0 && projectionY <= width) {
            return Collision(t, normal, true);
        }
    }

    return Collision(-1, vec3(0, 0, 0), false);
}


// Ray sphere intersection function adapted from this blog https://iquilezles.org/articles/intersectors/
Collision Sphere::intersect(ray r) const
{
    vec3 oc = r.origin - center;
    double b = oc.dot(r.direction);
    double c = oc.dot(oc) - radius * radius;
    double h = b * b - c;
    if (h < 0.0)
        return Collision(-1, vec3(0, 0, 0), false); // no intersection
    h = sqrt(h);
    // ray touches the sphere with only one intersection point
    double p1 = -b - h;
    double p2 = -b + h;
    double distance = -1;

    if (p1 < 0.0)
    {
        distance = p2;
    }
    else
        distance = p1;

    vec3 intersection_point = r.origin + r.direction * distance;

    return Collision(distance, intersection_point - center, true);
}

vec3 Camera::view_dir(double image_space_x, double image_space_y) const
{
    double x = image_space_x * sensor_size - .5 * sensor_size;
    double y = (-image_space_y * sensor_size + .5 * sensor_size) * 3. / 4.;
    vec3 local = vec3(x, y, focal_length).normalize();

    vec3 world_space_view_dir = local_to_world(local);
    return world_space_view_dir.normalize();
}

vec3 Camera::forward_vec() const
{
    return direction.normalize();
}

vec3 Camera::right_vec() const
{
    return direction.cross(up).normalize();
}

vec3 Camera::up_vec() const
{
    return right_vec().cross(direction).normalize();
}

vec3 Camera::local_to_world(const vec3 v) const
{
    // just a basis transformation
    return vec3(ortho_r_cache.x * v.x + ortho_u_cache.x * v.y + ortho_f_cache.x * v.z,
                ortho_r_cache.y * v.x + ortho_u_cache.y * v.y + ortho_f_cache.y * v.z,
                ortho_r_cache.z * v.x + ortho_u_cache.z * v.y + ortho_f_cache.z * v.z);
}

void Camera::forward()
{
    position = position + forward_vec() * movement_speed;
}

void Camera::backward()
{
    position = position - forward_vec() * movement_speed;
}

void Camera::right()
{
    position = position + right_vec() * movement_speed;
}

void Camera::left()
{
    position = position - right_vec() * movement_speed;
}

void Camera::rotate_left_right(double angle)
{
    double current_angle = std::atan2(direction.y, direction.x);
    double new_angle = current_angle + angle;

    double base_length = vec2(direction.x, direction.y).length();
    direction = vec3(std::cos(new_angle) * base_length, std::sin(new_angle) * base_length, direction.z);

    update_ortho_cache();
}

void Camera::update_ortho_cache()
{
    ortho_f_cache = forward_vec();
    ortho_r_cache = right_vec();
    ortho_u_cache = up_vec();
}

void Camera::rotate_up_down(double angle)
{

    double base_length = vec2(direction.x, direction.y).length();
    double pitch_angle = std::atan2(direction.z, base_length);

    double new_pitch_angle = pitch_angle + angle;

    new_pitch_angle = new_pitch_angle > M_PI / 2 ? pitch_angle : new_pitch_angle;
    new_pitch_angle = new_pitch_angle < -M_PI / 2 ? -pitch_angle : new_pitch_angle;

    double new_z = std::sin(new_pitch_angle);

    double new_base_length = std::cos(new_pitch_angle);

    vec2 new_base_vector = vec2(direction.x, direction.y).normalize() * new_base_length;
    direction = vec3(new_base_vector.x, new_base_vector.y, new_z);

    update_ortho_cache();
}