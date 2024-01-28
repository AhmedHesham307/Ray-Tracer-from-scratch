#include <iostream>
#include <cmath>
#include <float.h>
#include <SDL.h>
#include <memory>
#include <algorithm>

#include "scene.h"
#include <numeric>
#include <chrono>

#define RENDER_SCENE
// #define TEXTURE_TEST
#define LIGHT_POS point3(0, 0, 0)
#define GROUND_COLOR RGB(0.025, 0.05, 0.075)
#define SKYCOLOR_LOW RGB(0.36, 0.45, 0.57)
#define SKYCOLOR_HIGH RGB(0.14, 0.21, 0.49)
#define SUN_COLOR RGB(1.64, 1.27, 0.99)
#define SUN_DIRECTION vec3(.7, .4, .7)


const int SCREEN_WIDTH = 640;
// const int cam.image_height = 480;
const bool performance_logging = true;
constexpr float ASPECT_RATIO = 4/ 3;
int i = 0;

RGB out_color(vec3 v)
{
    if (v.z < 0.0){
        return GROUND_COLOR;
    }
    v = v.normalize();
    const float skyGradient = 1. / 4.;
    vec3 skyColor = vec3::linear_interp(SKYCOLOR_LOW, SKYCOLOR_HIGH, std::pow(v.z, skyGradient));
    return skyColor;
}

/*
* diffuse light intensity
*/
double diffuse_shading(vec3 pos, vec3 normal, vec3 light_pos)
{
    vec3 light_dir = (light_pos - pos).normalize();
    // This is a standard, physically based(tm) diffuse lighting calculation
    double lambertian = vec3::dot(light_dir , normal.normalize());
    return lambertian > 0 ? lambertian : 0;
}

/* 
* specular light intensity
*/
double specular(vec3 pos, vec3 normal, vec3 light_pos, vec3 view_dir){
    //Blinn-Phong specular
    view_dir = view_dir.normalize();
    normal = normal.normalize();
    vec3 light_dir = (light_pos - pos).normalize();

    vec3 halfway = (view_dir + light_dir).normalize();
    double result = vec3::dot(halfway , normal);
    return result > 0 ? result : 0;
}

/*
* Find the intersection of a ray with the scene that is closest to the ray origin
*/
Collision find_closest_hit(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r)
{
    // create placeholder collision with highest possible distance and no intersection
    Collision col = Collision(DBL_MAX, vec3(0, 0, 0), false , -1);

    // check all scene objects for a collision closer to the camera than the current closest collision
    for (int j = 0; j < scene.size(); j++)
    {   
        Collision object_col = scene.at(j)->intersect(r);
        
        if (object_col.distance > 0 && object_col.distance < col.distance)
        {
            col = object_col;
            col.hit_object_index = j;
        }
    }
    return col;
}

/*
* Send out a ray into the scene from a given position. Returns the color of light transported along that ray. Recursively factors in reflections.
*/
RGB recursive_ray_tracing(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r,  int remaining_iterations = 10)
{
    Collision col = find_closest_hit(scene, r);
    
    if (col.hit_object_index < 0)
    {
        return out_color(r.get_direction());
    }
    else
    {
        vec3 pos = r.get_origin() + r.get_direction() * col.distance;
        Material mat = scene.at(col.hit_object_index)->get_material();
        
        double diffuse_intensity = diffuse_shading(r.get_origin() + r.get_direction() * col.distance, col.normal, LIGHT_POS);
        double specular_intensity = std::pow(specular(pos, col.normal, LIGHT_POS, -(r.get_direction())), mat.specular_exponent);
        RGB local_color = mat.color * (diffuse_intensity * mat.diffuse + specular_intensity * mat.specular + mat.ambient);
        if (remaining_iterations <= 0)
        {
            return local_color;
        }
        
        //start new ray minimally offset from the surface so that the new ray can not hit the surface again
        point3 start_pos = pos + col.normal * .0001;
        vec3 reflected_dir = vec3::reflect(r.get_direction() ,col.normal);
        ray next_ray = ray(reflected_dir, start_pos);
    
        RGB rt_color = recursive_ray_tracing(scene, next_ray, remaining_iterations - 1);
        RGB interp_color;
        return interp_color.linear_interp(local_color, rt_color, mat.metallic);
    }
}

/*
* fill a buffer of colors with the colors seen by a camera in the scene
*/
void rt_scene(std::vector<vec3> u,const std::vector<std::unique_ptr<SceneGeometry>> &scene,const Camera &cam,
              std::vector<std::vector<RGB>> &frame_buffer)
{
    double step = (1. / static_cast<double>(cam.image_width));
    
    for (int i = 0; i < cam.image_height; i++){
        for(int j = 0; j < cam.image_width; j++){
        // do not sample full image space range from 0 to 1, sample pixel centers instead
        auto pixel_center = cam.image_top_left + u[0] *j + u[1] *i ;
        auto cam_pixel = cam.position - pixel_center ;
        ray cam_pixel_ray(cam_pixel, cam.position);
    
        frame_buffer.at(i).at(j) = recursive_ray_tracing(scene, cam_pixel_ray);
        }
    }
}

/*
* The main loop lives here
*/
int main(int argc, char *args[])
{
    Camera cam;
    cam.aspect_ratio = ASPECT_RATIO;
    cam.image_width = SCREEN_WIDTH;
    cam.movement_speed = 0.1;
    cam.vfov = 90;
    cam.position = point3(0,0,0);
    cam.lookat   = point3(-1,0,0);
    cam.vup      = vec3(0,0,-1);
    auto u = cam.init();
    // containers for the scene objects
    std::vector<std::unique_ptr<SceneGeometry>> scene = {};

    // scene definition
    // scene.push_back(std::make_unique<Sphere>(Material(RGB(1, 0, 0),0.5), point3( 3, 2, 0), .5));
    scene.push_back(std::make_unique<Sphere>(Material(RGB(0, 1, 0),0.5), point3( 1.5, 0, 0), .5));

    scene.push_back(std::make_unique<Wall>(Material(RGB(0, 0, 1)), point3(3.0, 2, 0) , vec3(0,-1,0), 1, 1));
    scene.push_back(std::make_unique<Wall>(Material(RGB(0, 1, 0)), point3(3.0, -3, 0), vec3(0,1,0), 2,  2));


    int frame_number = 0;

    std::vector<int64_t> total_times = {};
    std::vector<int64_t> rt_times = {};
    std::vector<int64_t> outpainting_times = {};
    std::vector<int64_t> shading_times = {};
    std::vector<int64_t> surface_update_times = {};
    std::vector<int64_t> sdl_rendering_times = {};

    // SDL setup adapted from the resource linked in the task description
    // https://lazyfoo.net/tutorials/SDL/01_hello_SDL/index2.php
    SDL_Window *window = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        // Create window
        window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, cam.image_height, SDL_WINDOW_SHOWN);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            // create the surface. The surface is the buffer we write our color values to.
            SDL_Surface *surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, cam.image_height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
            if (!surface)
            {
                std::cerr << "Surface creation failed: " << SDL_GetError() << std::endl;
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }

            SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (!renderer)
            {
                std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }
// Texture modification test
#if defined(TEXTURE_TEST)
            for (int y = 0; y < cam.image_height; ++y)
            {
                for (int x = 0; x < SCREEN_WIDTH; ++x)
                {
                    Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + y * surface->pitch / 4 + x;
                    *pixel = SDL_MapRGBA(surface->format, 255, ((float)x) / SCREEN_WIDTH * 255, ((float)y) / cam.image_height * 255, 0);
                }
            }
#endif

            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!texture)
            {
                std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
                SDL_DestroyRenderer(renderer);
                SDL_FreeSurface(surface);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }

            /*
            Define Buffers for holding raytracing output
            The per-pixel scene information is translated to an image in another step, shading
            Note that all scene buffers are only one dimensional
            The 2d image is created by stretching the 1d information dependant on the depth. See Camera::Outpainting for details

            We might switch to recursive ray tracing with nice reflections for sprint 3.
            */
            // screen buffer
            std::vector<std::vector<RGB>> frame_buffer(SCREEN_WIDTH,std::vector<RGB>(cam.image_height, RGB(0, 0, 0)));

            SDL_Event e;
            bool quit = false;

            int mouse_x, mouse_y = 0;

            while (quit == false)
            {

                while (SDL_PollEvent(&e))
                {
                    if (e.type == SDL_QUIT)
                        quit = true;

                    // handle key presses
                    else if (e.type == SDL_KEYDOWN)
                    {

                        switch (e.key.keysym.sym)
                        {
                        case SDLK_UP:
                            cam.forward();
                            break;

                        case SDLK_DOWN:
                            cam.backward();
                            break;

                        case SDLK_LEFT:
                            cam.left();
                            break;

                        case SDLK_RIGHT:
                            cam.right();
                            break;

                        case SDLK_a:
                            cam.left();
                            break;

                        case SDLK_s:
                            cam.backward();
                            break;

                        case SDLK_d:
                            cam.right();
                            break;

                        case SDLK_w:
                            cam.forward();
                            break;

                        case SDLK_q:
                            quit = true;
                            break;

                        case SDLK_r:
                            // cam = Camera(vec3(1,0,0), point3(0,0,0), 35.0, ASPECT_RATIO, SCREEN_WIDTH,0.1);
                            break;

                        default:
                            break;
                        }
                    }
                }
                /*
                The code for rotating the mouse. We would have liked to implement camera movement like in first person games,
                but unfortunately, the functionality necessary to relative use mouse movements without movements being blocked
                by the window borders is not available for WSL2 GUI windows. We tried both capturing the cursor and warping the
                cursor to the center of the window. Neither works in WSL2, but they break the close window button.
                The final product is kinda janky, but it allows for unlimited
                rotation, unlike the other implementations, which would have been limited in how far the camera can be rotated.
                Also, this solution does not mess with the close window button, like the other implementations did.
                */
                int x, y = 0;
                // SDL_GetMouseState(&x, &y);
                // float x_input = (x - cam.image_width / 2) / static_cast<double>(cam.image_width / 2);
                // float y_input = (y - cam.image_height / 2) / static_cast<double>(cam.image_height / 2);
                // cam.rotate_left_right(-x_input * .05);
                // cam.rotate_up_down(-y_input * .05);


                auto rt_start_time = std::chrono::high_resolution_clock::now();
                // std::cout << "start raytracing\n";
                //  Render and create the outpainted stencil
                rt_scene(u, scene, cam, frame_buffer);
                auto rt_end_time = std::chrono::high_resolution_clock::now();
                // std::cout << "end raytracing\n";
                auto outpainting_end_time = std::chrono::high_resolution_clock::now();

                // std::cout << "shading and outpainting done\n";
                auto shading_end_time = std::chrono::high_resolution_clock::now();

#if defined(RENDER_SCENE)
                Uint8 *pixels = (Uint8 *)surface->pixels;
                for (int i = 0; i < cam.image_height; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {
                        RGB val = frame_buffer.at(i).at(j);
                        Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + i * surface->pitch / 4 + j;
                        *pixel = SDL_MapRGB(surface->format, val.x * 255, val.y * 255, val.z * 255);
                    }
                }
#endif
                auto surface_end_time = std::chrono::high_resolution_clock::now();

                // Clear before update
                SDL_RenderClear(renderer);
                // Render the texture
                texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_RenderCopy(renderer, texture, NULL, NULL);

                // Update the window surface
                SDL_RenderPresent(renderer);
                auto render_end_time = std::chrono::high_resolution_clock::now();

                // append the measured times
                auto rt_time = std::chrono::duration_cast<std::chrono::microseconds>(rt_end_time - rt_start_time);
                rt_times.push_back(rt_time.count());
                auto outpainting_time = std::chrono::duration_cast<std::chrono::microseconds>(outpainting_end_time - rt_end_time);
                outpainting_times.push_back(outpainting_time.count());
                auto shading_time = std::chrono::duration_cast<std::chrono::microseconds>(shading_end_time - outpainting_end_time);
                shading_times.push_back(shading_time.count());
                auto surface_time = std::chrono::duration_cast<std::chrono::milliseconds>(surface_end_time - shading_end_time);
                surface_update_times.push_back(surface_time.count());
                auto render_time = std::chrono::duration_cast<std::chrono::milliseconds>(render_end_time - surface_end_time);
                sdl_rendering_times.push_back(render_time.count());
                auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(render_end_time - rt_start_time);
                total_times.push_back(total_time.count());
                frame_number++;
            }
            // properly dispose of the resources allocated by the SDL backend
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_FreeSurface(surface);
            SDL_DestroyWindow(window);
            SDL_Quit();

            // log the average time taken by every step
            if (performance_logging)
            {
                std::cout << "Number of frames: " << frame_number << " : " << (std::accumulate(total_times.begin(), total_times.end(), 0) / total_times.size()) << " ms average frame time\n";
                std::cout << "   " << (std::accumulate(rt_times.begin(), rt_times.end(), 0) / rt_times.size()) << " microseconds for average raytracing\n";
                std::cout << "   " << ((std::accumulate(outpainting_times.begin(), outpainting_times.end(), 0) / outpainting_times.size())) << " microseconds for average outpainting\n";
                std::cout << "   " << (std::accumulate(shading_times.begin(), shading_times.end(), 0) / shading_times.size()) << " microseconds for average shading\n";
                std::cout << "   " << (std::accumulate(surface_update_times.begin(), surface_update_times.end(), 0) / surface_update_times.size()) << " milliseconds for surface average update\n";
                std::cout << "   " << ((std::accumulate(sdl_rendering_times.begin(), sdl_rendering_times.end(), 0) / sdl_rendering_times.size())) << " milliseconds for average SDL rendering\n";
            }

            return 0;
        }
    }
}