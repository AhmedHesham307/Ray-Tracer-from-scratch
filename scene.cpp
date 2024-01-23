#include "scene.h"


Collision Wall::intersect(ray r) const
{
    // Calculate the denominator of the parametric equation
    double denominator = normal.dot(r.get_direction());

    // Calculate the parameter t for the intersection point
    double t = (position - r.get_origin()).dot(normal) / denominator;

    // Check if the intersection point is in front of the ray
    if (t > 0) {
        // Calculate the intersection point
        vec3 intersection_point = r.at(t);

        // Calculate the basis vectors for the local coordinate system of the wall
        vec3 wallRight = normal.cross(vec3(0, 0, 1)).normalize();
        vec3 wallUp = wallRight.cross(normal).normalize();

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

/*
* ray Sphere intersection implementation
*/
Collision Sphere::intersect(ray r) const
{
    auto ray_direction = r.get_direction();
    
    auto ray_origin = r.get_origin();
    vec3 ray_sphere_vec = ray_origin - center ;
    
    double a = ray_direction.dot(ray_direction);
    
    double b = 2 * ray_direction.dot(ray_sphere_vec);

    double c = ray_sphere_vec.dot(ray_sphere_vec) - radius * radius;

    double det = b * b - 4 * a * c;
    
    double projection = -1; 
    // ray doesn't collide with the sphere
    if(det < 0) {
        return Collision(-1, vec3(0,0,0), false);
    }
    // ray touches the sphere with only one intersection point
    vec3 intersection_point(0,0,0);
    if(det == 0){
        intersection_point = r.get_origin() + r.get_direction() * (-b / (2 * a));
        projection = (-b - sqrt(det))/ a;
    }
    // ray touches the sphere with 2 intersection points
    else{
        double p1 = (-b + sqrt(det)) / (2 * a);
        double p2 = (-b - sqrt(det)) / (2 * a);
        
        projection = p1 < p2 ? p1 : p2;
        intersection_point = ray_origin + ray_direction * projection;
    }
    return Collision(projection * ray_direction.length(), intersection_point - center, true);
}

double degrees_to_radians(double degrees) {
    return degrees * 3.14 / 180.0;
}

std::vector<vec3> Camera::init(){
    vec3   u, v, w;        // Camera frame basis vectors
    position = lookfrom;
    image_height = static_cast<int>(image_width / aspect_ratio);
    focal_length = (lookfrom - lookat).length();
    auto theta = degrees_to_radians(vfov);
    auto h = tan(theta/2);
    auto fov_height = 2 * h * focal_length;
    double fov_width = fov_height * (static_cast<double>(image_width)/image_height);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    w = (lookfrom - lookat).normalize();
    u = (vup.cross(w)).normalize();
    v = w.cross(u);

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

void Camera::outpainting(std::vector<std::vector<double>> depth, std::vector<std::vector<bool>> &out_hits)
{
    uint image_width = depth[0].size();
    uint image_height = depth.size();

    double object_height = 1.0; // Assuming a default object height

    // Assuming camera's direction is along the negative z-axis
    vec3 camera_direction(0, 0, -1);

    for (int i = 0; i < image_width; i++)
    {
        for (int j = 0; j < image_height; j++)
        {
            double image_space_x = i / static_cast<double>(image_width) + 0.5 / image_width;
            double image_space_y = j / static_cast<double>(image_height) + 0.5 / image_height;

            // Convert normalized image space coordinates to world coordinates
            double world_x = image_space_x * image_width;
            double world_y = image_space_y * image_height;

            // Find the corresponding depth value in the scene
            double world_z = depth[j][i];
            // std::cout<< world_z ;

            // Check if the point is in front of the camera
            if (camera_direction.dot(vec3(world_x, world_y, world_z)) > 0)
            {
                // // Calculate the apparent height of the object
                // double apparent_height = object_height / world_z * focal_length;

                // Check if the current point in the scene is within the apparent height
                // if (world_z < apparent_height)
                // {
                    // Set the corresponding element in out_hits to true
                    out_hits[j][i] = true;
                // }
            }
        }
    }
}

vec3 Camera::forward_vec(){
    return direction;
}

vec3 Camera::right_vec(){
    return vec3(direction.y, -direction.x, 0);
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
    direction = vec3(std::cos(new_angle), std::sin(new_angle),0);
}


