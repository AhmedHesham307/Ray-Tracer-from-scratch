#include "vec.h"
#include <vector>
#include <iostream>

#define DEFAULT_MAT Material(RGB(1, 1, 1), .9, .9, .3, 30)

struct ray
{
    vec3 direction;
    point3 origin;

    ray(vec3 direction, point3 origin) : direction{direction}, origin{origin} {}
};

struct Collision
{
    double distance;
    vec3 normal;
    bool hit;
    Collision(double distance, vec3 normal, bool hit) : distance{distance}, normal{normal}, hit{hit} {}
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

class Sphere : public SceneGeometry
{
    point3 center;
    double radius;

public:
    Sphere(point3 center = point3(0,0,0), double radius = 1.0, Material mat = DEFAULT_MAT) 
    : SceneGeometry{mat}, center{center}, radius{radius}{}
    Collision intersect(ray r) const override;
};

/*
class Wall : public SceneGeometry
{
    vec2 direction;
    point2 origin;
    double upper_bound;


    public:
    Wall(vec2 direction, point2 origin, double upper_bound, Material mat=DEFAULT_MAT) : SceneGeometry(mat), direction{direction}, origin{origin}, upper_bound{upper_bound} {}
    Collision intersect(ray r) const override;

};

class Circle : public SceneGeometry
{
    point2 center;
    double radius;

public:
    Circle(point2 center, double radius, Material mat = DEFAULT_MAT) : SceneGeometry(mat), center{center}, radius{radius} {}
    Collision intersect(ray r) const override;
};*/

struct Camera
{

    point3 position;
    double focal_length;
    double sensor_size;
    const float movement_speed = .1;

    /*
    * construct an orthogonal basis in a right handed world coordinate system
    */
    vec3 forward_vec() const;
    /*
    * constructs an orthogonal basis in a right handed world coordinate system
    */
    vec3 right_vec() const;
    /*
    * constructs an orthogonal basis in a right handed world coordinate system
    */
    vec3 up_vec() const;

    /*
    * transform a vector from the left-handed camera space to the right-handed world space
    * Please complain at the people who invented the conventions for 3d graphics
    */
    vec3 local_to_world(const vec3 local) const;

    Camera(vec3 dir, point3 pos, vec3 up, double focal_length, double sensor_size) : direction{dir}, position{pos}, up{up}, focal_length{focal_length}, sensor_size{sensor_size}, ortho_f_cache{forward_vec()}, ortho_r_cache{right_vec()}, ortho_u_cache{up_vec()} {
        ortho_r_cache.print();
        ortho_f_cache.print();
        ortho_u_cache.print();
    }

    /*
    * map a position in the image space to a ray originating from the camera
    */
    vec3 view_dir(double image_space_x, double image_space_y) const;

    /*
    create a stencil for outpainting the one dimensional raytracer depth output
    to a two dimensional image. Objects that are closer to the camera should
    appear larger in the 2nd image dimension. The booleans of the 2d stencil
    indicate, whether this pixel should be filled with the stretched output from
    the shading of the object or with background.
    */

    void forward();
    void backward();
    void left();
    void right();

    void rotate(double angle);

private:
    // please note that these vectors are not necessarily orthogonal!
    vec3 direction;
    vec3 up;

    // these are cached values for the orthogonal basis of the camera in the world.
    // Without caching, for every ray generated, those values would need to be calculated again
    vec3 ortho_f_cache;
    vec3 ortho_r_cache;
    vec3 ortho_u_cache;
};