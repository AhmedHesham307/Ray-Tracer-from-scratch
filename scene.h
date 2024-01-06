#include "vec2.h"
#include <vector>

struct ray
{
    vec2 direction;
    point2 origin;

    ray(vec2 direction, point2 origin) : direction{direction}, origin{origin} {}
};


struct Collision
{
    double distance;
    vec2 normal;
    bool hit;
    Collision(double distance, vec2 normal, bool hit) : distance{distance}, normal{normal}, hit{hit} {}
};

class SceneGeometry{
    public:
    virtual Collision intersect(ray r) = 0;

    virtual ~SceneGeometry(){}
};

class Wall : public SceneGeometry
{
    vec2 direction;
    point2 origin;
    double upper_bound;

    public:
    Wall(vec2 direction, point2 origin, double upper_bound) : direction{direction}, origin{origin}, upper_bound{upper_bound} {}
    Collision intersect(ray r) override;
};


class Circle : public SceneGeometry
{
    point2 center;
    double radius;
    public:
    Circle(point2 center, double radius) : center{center}, radius{radius} {}
    Collision intersect(ray r) override;
};





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
        double image_plane_height = image_plane_width / (16. / 9);
        //std::cout << image_plane_height << std::endl;

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
};