#include "vec.h"
#include <vector>
#include <iostream>

#define DEFAULT_MAT Material(RGB(1, 1, 1), .9, .9, .3, 30)

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
    int hit_object_index;
};

struct Material
{
    RGB color;
    // how much the object is lit without a light
    double ambient;
    // how much reflections contribute to the color of the material
    double metallic;
    // strength of the diffuse component
    double diffuse;
    // strength of the specular component
    double specular;
    // controls specular highlight shape
    double specular_exponent;
    Material(RGB color, double metallic = .5, double ambient = .1, double diffuse = .9, double specular = .4, double specular_exponent = 50) : color{color}, diffuse{diffuse}, ambient{ambient}, metallic{metallic}, specular{specular}, specular_exponent{specular_exponent} {}
};

class SceneGeometry
{
    Material mat;

public:
    virtual Collision intersect(ray r) const = 0;
    SceneGeometry(Material mat) : mat(mat){};
    virtual ~SceneGeometry() {}
    Material get_material() { return mat; }
};

class Wall : public SceneGeometry
{
    vec2 direction;
    point2 origin;
    double upper_bound;

public:
    Wall(vec2 direction, point2 origin, double upper_bound, Material mat = DEFAULT_MAT) : SceneGeometry(mat), direction{direction}, origin{origin}, upper_bound{upper_bound} {}
    Collision intersect(ray r) const override;
};

class Circle : public SceneGeometry
{
    point2 center;
    double radius;

public:
    Circle(point2 center, double radius, Material mat = DEFAULT_MAT) : SceneGeometry(mat), center{center}, radius{radius} {}
    Collision intersect(ray r) const override;
};

struct Camera
{

    vec2 direction;
    point2 position;
    double focal_length;
    double sensor_size;
    double movement_speed;

    vec2 forward_vec();
    vec2 right_vec();

    Camera(vec2 dir, point2 pos, double focal_length, double sensor_size, double speed = .1) : direction{dir}, position{pos}, focal_length{focal_length}, sensor_size{sensor_size}, movement_speed{speed} {}

    /*
     * returns a vector originating from the camera given a screen space coordinate. Screen space maps the leftmost pixel to 0 and the rightmost pixel to 1.
     */
    vec2 view_dir(double image_space_x) const;

    /*
     * given a 1d depth buffer, create a 2d stencil that gives the information, to how many pixels the 1d images has to be stretched in the second dimension, depending on the depth
     */
    void outpainting(std::vector<double> depth, std::vector<std::vector<bool>> &out_hits);

    void forward();
    void backward();
    void left();
    void right();

    void rotate(double angle);
};