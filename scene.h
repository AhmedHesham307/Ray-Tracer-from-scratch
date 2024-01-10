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

class SceneGeometry{
    public:
    virtual Collision intersect(ray r) const = 0;
    virtual ~SceneGeometry(){}
    virtual RGB get_color() const=0;
};

class Wall : public SceneGeometry
{
    vec2 direction;
    point2 origin;
    double upper_bound;
    RGB color;

    public:
    Wall(vec2 direction, point2 origin, double upper_bound, RGB color=RGB(1,1,1)) : direction{direction}, origin{origin}, upper_bound{upper_bound}, color{color} {}
    Collision intersect(ray r) const override;
    RGB get_color() const{return color;}
};


class Circle : public SceneGeometry
{
    point2 center;
    double radius;
    RGB color;
    public:
    Circle(point2 center, double radius, RGB color=RGB(1,1,1)) : center{center}, radius{radius}, color{color} {}
    Collision intersect(ray r) const override;
    RGB get_color() const{return color;}
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
    void outpainting(std::vector<double> depth, std::vector<bool> hits, std::vector<std::vector<bool>> &out_hits);

    void forward();
    void backward();
    void left();
    void right();

    void rotate(double angle);
};