#include "scene.h"


Collision Wall::intersect(ray r) const
{
    // Calculate the denominator of the parametric equation
    double denominator = vec3::dot(normal , r.get_direction());

    // Calculate the parameter t for the intersection point
    double t = vec3::dot(position - r.get_origin() , normal) / denominator;
    std::cout << t << std::endl;
    // Check if the intersection point is in front of the ray
    if (t > 0) {
        // Calculate the intersection point
        vec3 intersection_point = r.at(t);

        // Calculate the basis vectors for the local coordinate system of the wall
        vec3 wallRight = (vec3::cross(normal, vec3(0, 0, 1))).normalize();
        vec3 wallUp = (vec3::cross(wallRight , normal)).normalize();

        // Calculate the vector from the wall position to the intersection point
        vec3 wallToIntersection = intersection_point - position;

        // Project the wallToIntersection vector onto the wall's local coordinate system
        double projectionX = vec3::dot(wallToIntersection, wallRight);
        double projectionY = vec3::dot(wallToIntersection, wallUp);

        // Check if the intersection point is within the bounds of the wall
        if (projectionX >= 0 && projectionX <= length && projectionY >= 0 && projectionY <= width) {
            return Collision(t, normal, true, 0);
        }
    }

    return Collision(-1, vec3(0, 0, 0), false , 0);
}

/*
* ray Sphere intersection implementation
*/
Collision Sphere::intersect(ray r) const
{
    auto ray_direction = r.get_direction();
    
    auto ray_origin = r.get_origin();
    vec3 ray_sphere_vec = ray_origin - center ;

    double a = ray_direction.length_squared();

    double b = 2 * vec3::dot(ray_direction , ray_sphere_vec);

    double c = ray_sphere_vec.length_squared() - radius * radius;

    double det = b * b - 4 * a * c;
    // std::cout<<det<<std::endl;

    double projection = -1; 
    // ray doesn't collide with the sphere
    if(det < 0) {
        return Collision(-1, vec3(0,0,0), false , 0);
    }
    // ray touches the sphere with only one intersection point
    vec3 intersection_point(0,0,0);
    if(det == 0){
        intersection_point = r.get_origin() + r.get_direction() * (-b / (2 * a));
        projection = (-b - sqrt(det))/ a;
    }
    // ray touches the sphere with 2 intersection points
    else{
        // std:: cout << b << std::endl;
        double p1 = (-b + sqrt(det)) / (2 * a);
        double p2 = (-b - sqrt(det)) / (2 * a);
        projection = p1 < p2 ? p1 : p2;
        // std::cout << a << std::endl;

        intersection_point = ray_origin + ray_direction * projection;
    }
    return Collision(projection * ray_direction.length(), intersection_point - center, true , 0);
}

std::vector<vec3> Camera::init(){
    vec3   u, v, w;        // Camera frame basis vectors
    image_height = static_cast<int>(image_width / aspect_ratio);
    focal_length = (position - lookat).length();
    auto theta = vfov * 3.14 / 180.0;
    auto h = tan(theta/2);
    auto fov_height = 2 * h * focal_length;
    double fov_width = fov_height * (static_cast<double>(image_width)/image_height);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    w = (position - lookat).normalize();
    u = (vec3::cross(vup, w)).normalize();
    v = vec3::cross(w,u);
    direction = w;
    // the FOV of the camera will be in the z direction so (x , -y , 0)
    vec3 fov_x = u * fov_width;
    // fov_x.print();
    vec3 fov_y = v * (-fov_height);
    // these are the space between pixels in our fov
    auto pixel_delta_x = fov_x / image_width;
    auto pixel_delta_y = fov_y / image_height;
    
    fov_top_left = position - (w*focal_length) - fov_x/2 - fov_y/2;
    // the location of the first top left pixel according to our camera view in world space
    image_top_left = fov_top_left + (pixel_delta_x + pixel_delta_y) * 0.5;     
    return std::vector<vec3> {pixel_delta_x ,pixel_delta_y };
    }

vec3 Camera::forward_vec(){
    return direction;
}

vec3 Camera::right_vec(){
    return (vec3::cross(vup,  ((position - lookat).normalize()))).normalize();
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

void Camera::rotate(double angle)
{
    double current_angle = std::atan2(direction.y, direction.x);
    double new_angle = current_angle + angle;
    direction = vec3(std::cos(new_angle), std::sin(new_angle), 0);
}


