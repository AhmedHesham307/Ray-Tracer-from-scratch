#include "vec.h"
#include <vector>
#define DEFAULT_MAT Material(RGB(1, 1, 1), .9, .9, .3, 30)

class ray
{
    private:
    vec3 direction;
    point3 origin;

    public:
    
    ray() {}
    ray(const vec3 direction, const point3 origin) : direction{direction}, origin{origin} {}
    
    point3 at (double t) const {
        return origin + direction * t;
    }
    vec3 get_direction(){
        return direction;
    }
    vec3 get_origin(){
        return origin;
    }
};

struct Collision{
    double distance;
    vec3 normal;
    bool hit;
    Collision(double distance, vec3 normal, bool hit) : distance{distance}, normal{normal}, hit{hit}{}
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
    SceneGeometry(Material mat) : mat(mat){}
    virtual Collision intersect(ray r) const = 0;
    virtual ~SceneGeometry() {}
    Material get_material() { return mat; }
};

class Wall : public SceneGeometry
{
    point3 position;  // Center or starting point of the wall
    vec3 normal;      // Normal vector representing the orientation of the wall
    double length;     // Length of the wall
    double width;      // Width of the wall (optional)

public:
    Wall(Material mat = DEFAULT_MAT, point3 position = point3(0,0,0), vec3 normal = vec3(0,0,0), double length= 1.0, double width = 1.0)
        : SceneGeometry{mat}, position{position}, normal{normal.normalize()}, length{length}, width{width} {}
    Collision intersect(ray r) const override;
};

class Sphere : public SceneGeometry
{
    point3 center;
    double radius;

public:
    Sphere(Material mat = DEFAULT_MAT, point3 center = point3(0,0,0), double radius = 1.0) 
    : SceneGeometry{mat}, center{center}, radius{radius}{}
    Collision intersect(ray r) const override;
};

class Camera
{
private:
    vec3 forward_vec();
    vec3 right_vec();
public:
    vec3 direction, fov_top_left, image_top_left, pixel_delta_x , pixel_delta_y;
    double movement_speed, aspect_ratio , image_width, image_height, focal_length , vfov;

    point3 position = point3(0,0,-1);  // Point camera is looking from
    point3 lookat   = point3(0,0,0);   // Point camera is looking at
    vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction

    Camera (){}
    std::vector<vec3> init ();
    // Camera(vec3 dir, point3 pos, double focal_length, double aspect_ratio, double image_width, double speed = .1) : direction{dir}, position{pos}, focal_length{focal_length}, aspect_ratio{aspect_ratio}, image_width{image_width}, movement_speed{speed} {}
    
    void forward();
    void backward();
    void left();
    void right();

    void rotate(double angle);
    void rotate(double angle, const vec3& axis) ;
};