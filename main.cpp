#include <iostream>
#include <vector>
#include <cmath>
#include <float.h>

#define ASPECT (16. / 9)
// ascii art color map from https://paulbourke.net/dataformats/asciiart/
const std::vector<char> cmap = {'$', '@', 'B', '%', '8', '&', 'W', 'M', '#', '*', 'o', 'a', 'h', 'k', 'b', 'd', 'p', 'q', 'w', 'm', 'Z', 'O', '0', 'Q', 'L', 'C', 'J', 'U', 'Y', 'X', 'z', 'c', 'v', 'u', 'n', 'x', 'r', 'j', 'f', 't', '/', '\\', '|', '(', ')', '1', '{', '}', '[', ']', '?', '-', '_', '+', '~', '<', '>', 'i', '!', 'l', 'I', ';', ':', ',', '"', '^', '`', '\'', '.', ' '};

char sample_cmap(double val)
{
    val = val <= 1 ? val : 1;
    val = val >= 0 ? val : 0;
    val = 1- val;
    int index = val * ((double)cmap.size());
    if (index == cmap.size())
        index--;
    return cmap.at(index);
}

enum DisplayMode
{
    simple,
    outpainted,
    shaded
};

struct vec2
{

    double x;
    double y;
    vec2(double x_val, double y_val) : x{x_val}, y{y_val} {}

    double length()
    {
        return std::sqrt(x * x + y * y);
    }

    vec2 normalize()
    {
        double length = std::sqrt(x * x + y * y);
        return vec2(x / length, y / length);
    }

    double dot(const vec2 other)
    {
        return x * other.x + y * other.y;
    }

    double vec2_cross(const vec2 other)
    {
        return x * other.y - y * other.x;
    }
};

vec2 operator+(vec2 v1, const vec2 v2)
{
    v1.x = v1.x + v2.x;
    v1.y = v1.y + v2.y;
    return v1;
}

vec2 operator-(vec2 v1, const vec2 v2)
{
    v1.x = v1.x - v2.x;
    v1.y = v1.y - v2.y;
    return v1;
}

vec2 operator*(vec2 v1, const vec2 v2)
{
    v1.x = v1.x * v2.x;
    v1.y = v1.y * v2.y;
    return v1;
}

vec2 operator*(vec2 v1, double t)
{
    v1.x = v1.x * t;
    v1.y = v1.y * t;
    return v1;
}

using point2 = vec2;

struct ray
{
    vec2 direction;
    point2 origin;

    ray(vec2 direction, point2 origin) : direction{direction}, origin{origin} {}
};

struct Wall
{
    vec2 direction;
    point2 origin;
    double upper_bound;

    Wall(vec2 direction, point2 origin, double upper_bound) : direction{direction}, origin{origin}, upper_bound{upper_bound} {}
};

struct Collision
{
    double distance;
    vec2 normal;
    bool hit;
    Collision(double distance, vec2 normal, bool hit) : distance{distance}, normal{normal}, hit{hit} {}
};

struct Circle
{
    point2 center;
    double radius;
    Circle(point2 center, double radius) : center{center}, radius{radius} {}
};

Collision ray_wall_intersect(ray r, Wall w)
{

    // check determinant so we do not attempt to solve an unsolvable system (otherwise divide by zero will occur)
    double det = (r.direction.x * (-w.direction.y) - (-w.direction.x * r.direction.y));
    if (det == 0)
        return Collision(-1, vec2(0, 0), false);

    vec2 rhs = r.origin * (-1.) + w.origin;

    double first_step_fraction = r.direction.y / r.direction.x;

    double p2 = (rhs.y - first_step_fraction * rhs.x) / (-w.direction.y + first_step_fraction * w.direction.x);

    double p1 = (rhs.x + p2 * w.direction.x) / r.direction.x;

    bool hit = p1 > 0 && p2 >= 0 && p2 <= w.upper_bound;

    vec2 normal(w.direction.y, w.direction.x);
    if (normal.dot(r.direction) > 0)
        normal = normal * (-1.);

    return Collision(r.direction.length() * p1, normal, hit);
}

Collision ray_circle_intersect(ray r, Circle geo)
{
    vec2 z = r.origin + geo.center * (-1);

    double a = r.direction.x * r.direction.x + r.direction.y * r.direction.y;

    double b = 2 * (r.direction.x * z.x + r.direction.y * z.y);

    double c = z.x * z.x + z.y * z.y - geo.radius * geo.radius;

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

    return Collision(p * r.direction.length(), intersection_point + geo.center * (-1), p > 0);
}

struct Camera
{
    vec2 direction;
    point2 position;
    double focal_length;
    double sensor_size;

    Camera(vec2 dir, point2 pos, double focal_length, double sensor_size) : direction{dir}, position{pos}, focal_length{focal_length}, sensor_size{sensor_size} {}
    vec2 view_dir(double image_space_x)
    {
        double dir_angle = std::atan(direction.y / direction.x);
        double y = -image_space_x * sensor_size + .5 * sensor_size;
        vec2 local = vec2(focal_length, y).normalize();

        vec2 world_space_view_dir(std::cos(dir_angle) * local.x - std::sin(dir_angle) * local.y,
                                  std::sin(dir_angle) * local.x + std::cos(dir_angle) * local.y);

        return world_space_view_dir;
    }

    void outpainting(std::vector<double> depth, std::vector<bool> hits, std::vector<std::vector<bool>> &out_hits)
    {
        uint width = out_hits.at(0).size();
        uint height = out_hits.size();

        double object_height = .5;
        double image_plane_width = sensor_size / focal_length;
        double image_plane_height = image_plane_width / ASPECT;
        std::cout << image_plane_height << std::endl;

        for (int i = 0; i < width; i++)
        {
            if (hits.at(i))
            {
                double image_space_x = i / (double)width + .5 / width;
                double image_plane_vector_y = -image_space_x * sensor_size + .5 * sensor_size;
                double image_plane_distance = vec2(focal_length, image_plane_vector_y).length() / focal_length;

                double apparent_height = object_height / depth.at(i) * image_plane_distance;

                for (int j = 0; j < height; j++)
                {
                    // pixel position in 0 to 1 image space
                    double image_space_y = j / (double)height + .5 / height;
                    double off_center = std::abs(.5 - image_space_y) * image_plane_height;
                    if (off_center < apparent_height)
                        out_hits.at(j).at(i) = true;
                }
            }
        }
    }
};

double diffuse_shading(vec2 pos, vec2 normal, vec2 light_pos)
{
    vec2 light_dir = (light_pos - pos).normalize();
    return light_dir.dot(normal.normalize());
}

void create_shaded_frame_buffer(const std::vector<vec2>& positions, const std::vector<vec2>& normals, const std::vector<bool>& hits, std::vector<double>& shaded){
    
    for(int i = 0; i < positions.size(); i++){
        if(hits[i]){
            shaded[i] = diffuse_shading(positions[i], normals[i], vec2(0, 2));
        }else{
            shaded[i] = -1;
        }
    }
}

void print_shaded_buffer2d_stencil(const std::vector<double>& shaded, const std::vector<std::vector<bool>> stencil){
    for(int i = 0; i < stencil.size(); i++){
        for(int j = 0; j < shaded.size(); j++){
            if(stencil.at(i).at(j)){
                std::cout << sample_cmap(shaded.at(j));
            }else{
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }
}

void print_buffer_normalized(const std::vector<double> &depth, const std::vector<bool> &hit)
{
    std::cout << std::endl;

    double min_depth = DBL_MAX;
    double max_depth = -DBL_MAX;
    for (int i = 0; i < depth.size(); i++)
    {
        if (hit.at(i))
        {
            double d = depth.at(i);
            if (d < min_depth)
                min_depth = d;
            if (d > max_depth)
                max_depth = d;
        }
    }

    for (int i = 0; i < depth.size(); i++)
    {
        if (hit.at(i))
        {
            double normalized_depth = (depth.at(i) - min_depth) / (max_depth - min_depth);
            std::cout << sample_cmap(normalized_depth);
        }
        else
            std::cout << '.';
    }
    std::cout << std::endl;
}

void print_bool_buffer2d(const std::vector<std::vector<bool>> buffer)
{
    for (int i = 0; i < buffer.size(); i++)
    {
        for (int j = 0; j < buffer.at(0).size(); j++)
        {
            if (buffer.at(i).at(j))
                std::cout << 'X';
            else
                std::cout << '+';
        }
        std::cout << "\n";
    }
}

int main()
{
    Camera cam(vec2(1, 0), point2(0, 0), 35, 35);

    std::vector<Wall> walls = {};
    std::vector<Circle> circles = {};
    // walls.push_back(Wall(vec2(0, -1), vec2(4., 1.), 1));
    // walls.push_back(Wall(vec2(0, -1), vec2(2.5, 0), 1));
    circles.push_back(Circle(point2(5, 0), 1));

    double pixel_aspect = 2.3;
    int resolution = 50;

    DisplayMode mode = simple;

    while (true)
    {
        // initialize buffers. Do not put in front of the loop because resolution might be subject to change
        std::vector<double> depth(resolution, -1);
        std::vector<bool> hits(resolution, false);
        std::vector<vec2> normals(resolution, vec2(1, 0));
        std::vector<vec2> positions(resolution, vec2(0, 0));
        double step = (1. / (double)resolution);

        for (int i = 0; i < resolution; i++)
        {
            // do not sample full image space range from 0 to 1, sample pixel centers instead
            double progress = step * i + .5 * step;

            vec2 sample_dir = cam.view_dir(progress);
            ray camera_ray(sample_dir, cam.position);

            // std::cout << camera_ray.direction.x << " " << camera_ray.direction.y << std::endl;

            // create placeholder collision with highest possible distance and no intersection
            Collision col = Collision(DBL_MAX, vec2(0, 0), false);

            // check all walls for collision. update col if collision is closer than current closest collision
            for (int i = 0; i < walls.size(); i++)
            {
                Collision wall_col = ray_wall_intersect(camera_ray, walls.at(i));

                if (wall_col.hit == true && wall_col.distance < col.distance)
                    col = wall_col;
            }

            for (int i = 0; i < circles.size(); i++)
            {
                Collision circle_col = ray_circle_intersect(camera_ray, circles.at(i));

                if (circle_col.hit == true && circle_col.distance < col.distance)
                    col = circle_col;
            }

            if (col.hit)
            {
                hits[i] = true;
                depth[i] = col.distance;
                normals[i] = col.normal;
                positions[i] = camera_ray.origin + camera_ray.direction * col.distance;
            }
        }
        std::vector<std::vector<bool>> hits2d(resolution / (pixel_aspect * ASPECT), std::vector<bool>(resolution, false));
        std::vector<double> shaded_buffer(resolution, 0);

        switch (mode)
        {
        case simple:
            print_buffer_normalized(depth, hits);
            break;

        case outpainted:
            cam.outpainting(depth, hits, hits2d);
            print_bool_buffer2d(hits2d);
            break;

        case shaded:
            create_shaded_frame_buffer(positions, normals, hits, shaded_buffer);
            cam.outpainting(depth, hits, hits2d);
            print_shaded_buffer2d_stencil(shaded_buffer, hits2d);
            break;

        default:
            break;
        }

        std::cout << "Commands:\n\"simple\" \"outpainted\" \"shaded\": set rendering mode.   <integer>: set screen resolution    \"q\": quit\n";

        std::string input = "";
        std::cin >> input;

        if (input == "simple")
            mode = simple;
        if (input == "outpainted")
            mode = outpainted;
        if (input == "shaded")
            mode = shaded;
        if (input == "quit" || input == "q" || input == "Quit" || input == "Q")
            return 0;

        size_t pos_of_int = 0;

        try
        {
            int input_int = std::stoi(input);
            resolution = input_int;
        }
        catch (const std::exception &e)
        {
            // ignore invalid argument exception from stoi if input was not a number
        }
    }
}