#include "vec.h"
#include <vector>
#include <iostream>

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

struct Material{
    RGB color;
    // how much reflections contribute to the color of the material
    double metallic;
    Material(RGB color, double metallic) : color{color}, metallic{metallic}{}
};

class SceneGeometry{
    Material mat;
    public:
    virtual Collision intersect(ray r) const = 0;
    SceneGeometry(Material mat) : mat(mat){};
    virtual ~SceneGeometry(){}
    Material get_material() {return mat;}
};

class Wall : public SceneGeometry
{
    vec2 direction;
    point2 origin;
    double upper_bound;


    public:
    Wall(vec2 direction, point2 origin, double upper_bound, Material mat=Material(RGB(1,1,1), .1)) : SceneGeometry(mat), direction{direction}, origin{origin}, upper_bound{upper_bound} {}
    Collision intersect(ray r) const override;

};


class Circle : public SceneGeometry
{
    point2 center;
    double radius;

    public:
    Circle(point2 center, double radius, Material mat=Material(RGB(1,1,1), .1)) : SceneGeometry(mat), center{center}, radius{radius} {}
    Collision intersect(ray r) const override;

};


struct Camera{


    vec2 direction;
    point2 position;
    double focal_length;
    double sensor_size;
    const float movement_speed = .1;

    vec2 forward_vec();
    vec2 right_vec();

    Camera(vec2 dir, point2 pos, double focal_length, double sensor_size) : direction{dir}, position{pos}, focal_length{focal_length}, sensor_size{sensor_size} {}

    /*
    map a position in the image space to a ray originating from the camera
    */
    vec2 view_dir(double image_space_x) const;

    /*
    create a stencil for outpainting the one dimensional raytracer depth output
    to a two dimensional image. Objects that are closer to the camera should
    appear larger in the 2nd image dimension. The booleans of the 2d stencil
    indicate, whether this pixel should be filled with the stretched output from
    the shading of the object or with background.
    */
    void outpainting(std::vector<double> depth, std::vector<std::vector<bool>> &out_hits);

    void forward();
    void backward();
    void left();
    void right();

    void rotate(double angle);
};