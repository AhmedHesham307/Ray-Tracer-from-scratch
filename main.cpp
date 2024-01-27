#include <iostream>
#include <omp.h>
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

#define LIGHT_POS point3(2, 0, 0)
#define GROUND_COLOR RGB(0.025, 0.05, 0.075)
#define SKYCOLOR_LOW RGB(0.36, 0.45, 0.57)
#define SKYCOLOR_HIGH RGB(0.14, 0.21, 0.49)
#define SUN_COLOR RGB(1.64, 1.27, 0.99)
#define SUN_DIRECTION vec3(.7, .4, .7)

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const bool performance_logging = true;
constexpr float aspect = 4 / 3;

/*
 * calculate a background color based on the ray direction
 */
RGB out_color(vec3 v)
{
    if (v.z < 0.0)
        return GROUND_COLOR;
    v = v.normalize();
    const float skyGradient = 1. / 4.;
    vec3 skyColor = vec3::lerp(SKYCOLOR_LOW, SKYCOLOR_HIGH, std::pow(v.z, skyGradient));
    return skyColor;
}

/*
 * diffuse lighting component: The fraction of light that is spread equally in all directions
 */
double diffuse_shading(vec3 pos, vec3 normal, vec3 light_pos)
{
    vec3 light_dir = (light_pos - pos).normalize();
    // This is a standard, physically based(tm) diffuse lighting calculation
    double lambertian = light_dir.dot(normal.normalize());
    return lambertian > 0 ? lambertian : 0;
}

/*
 * specular lighting component: The fraction of lights that forms highlights on glossy objects
 */
double specular(vec3 pos, vec3 normal, vec3 light_pos, vec3 view_dir)
{
    // Blinn-Phong specular
    view_dir = view_dir.normalize();
    normal = normal.normalize();
    vec3 light_dir = (light_pos - pos).normalize();

    vec3 halfway = (view_dir + light_dir).normalize();
    double result = halfway.dot(normal);
    return result > 0 ? result : 0;
}

/*
 * Find the intersection with scene geometry that is closest to the ray origin
 */
Collision find_closest_hit(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r)
{
    // create placeholder collision with highest possible distance and no intersection
    Collision col = Collision(DBL_MAX, vec3(0, 0, 0), false);
    col.hit_object_index = -1;
    // check all scene objects for a collision closer to the camera than the current closest collision
    for (int j = 0; j < scene.size(); j++)
    {
        Collision wall_col = scene.at(j)->intersect(r);
        if (wall_col.distance > 0 && wall_col.distance < col.distance)
        {
            col = wall_col;
            col.hit_object_index = j;
        }
    }
    return col;
}

RGB recursive_ray_tracing(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r, int remaining_iterations = 0)
{
    Collision col = find_closest_hit(scene, r);
    if (col.hit_object_index < 0)
    {
        return out_color(r.direction);
    }
    else
    {
        vec3 pos = r.origin + r.direction * col.distance;
        Material mat = scene.at(col.hit_object_index)->get_material();
        double diffuse_intensity = diffuse_shading(r.origin + r.direction * col.distance, col.normal, LIGHT_POS);
        double specular_intensity = std::pow(specular(pos, col.normal, LIGHT_POS, r.direction * (-1)), mat.specular_exponent);
        RGB local_color = mat.color * (diffuse_intensity * mat.diffuse + specular_intensity * mat.specular + mat.ambient);
        if (remaining_iterations <= 0)
        {
            return local_color;
        }

        // start new ray minimally offset from the surface so that the new ray can not hit the surface again
        point3 start_pos = pos + col.normal * .0001;
        vec3 reflected_dir = r.direction.reflect(col.normal);
        ray next_ray = ray(reflected_dir, start_pos);

        RGB rt_color = recursive_ray_tracing(scene, next_ray, remaining_iterations - 1);

        return vec3::lerp(local_color, rt_color, mat.metallic);
    }
}

void rt_scene(const std::vector<std::unique_ptr<SceneGeometry>> &scene, const Camera &cam, std::vector<std::vector<RGB>> &frame_buffer)
{
    double step_x = (1. / static_cast<double>(SCREEN_WIDTH));
    double step_y = (1. / static_cast<double>(SCREEN_HEIGHT));
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (int j = 0; j < SCREEN_WIDTH; j++)
        {
            // do not sample full image space range from 0 to 1, sample pixel centers instead
            double screen_space_x = step_x * j + .5 * step_x;
            double screen_space_y = step_y * i + .5 * step_y;
            vec3 sample_dir = cam.view_dir(screen_space_x, screen_space_y);
            ray camera_ray(sample_dir, cam.position);

            frame_buffer.at(i).at(j) = recursive_ray_tracing(scene, camera_ray);
        }
    }
    /*
    cam.view_dir(0, 0).print();
    cam.view_dir(0, 1).print();
    cam.view_dir(1, 0).print();
    cam.view_dir(1, 1).print();
    std::cout << std::endl;
    */
}

int main(int argc, char *args[])
{
    Camera cam(vec3(1, 0, 0), point3(0, 0, 0), vec3(0, 0, 1), 35, 35);

    // containers for the scene objects
    std::vector<std::unique_ptr<SceneGeometry>> scene = {};

    // scene definition
    scene.push_back(std::make_unique<Sphere>(point3(-3, 0, 0), 1, Material(RGB(1, 1, 0), .5)));

    // scene.push_back(std::make_unique<Wall>(vec2(2, -1), point2(4, 3), 1, Material(RGB(0, 0, 1), 0.5)));
    // scene.push_back(std::make_unique<Wall>(vec2(1, .5), point2(4, -3), 2, Material(RGB(0, 1, 0), 0.5)));

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
        window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            // create the surface. The surface is the buffer we write our color values to.
            SDL_Surface *surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
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
            for (int y = 0; y < SCREEN_HEIGHT; ++y)
            {
                for (int x = 0; x < SCREEN_WIDTH; ++x)
                {
                    Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + y * surface->pitch / 4 + x;
                    *pixel = SDL_MapRGBA(surface->format, 255, ((float)x) / SCREEN_WIDTH * 255, ((float)y) / SCREEN_HEIGHT * 255, 0);
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

            // screen buffer
            std::vector<std::vector<RGB>> frame_buffer(SCREEN_HEIGHT, std::vector<RGB>(SCREEN_WIDTH, RGB(0, 0, 0)));

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
                SDL_GetMouseState(&x, &y);
                float input = (x - SCREEN_WIDTH / 2) / static_cast<double>(SCREEN_WIDTH / 2);
                // cam.rotate(-input * .003);

                auto rt_start_time = std::chrono::high_resolution_clock::now();
                // std::cout << "start raytracing\n";
                //  Render and create the outpainted stencil
                rt_scene(scene, cam, frame_buffer);
                auto rt_end_time = std::chrono::high_resolution_clock::now();

                auto outpainting_end_time = std::chrono::high_resolution_clock::now();

                // std::cout << "shading and outpainting done\n";
                auto shading_end_time = std::chrono::high_resolution_clock::now();

#if defined(RENDER_SCENE)
                Uint8 *pixels = (Uint8 *)surface->pixels;
                for (int i = 0; i < SCREEN_HEIGHT; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {

                        RGB val = frame_buffer.at(i).at(j);
                        Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + i * surface->pitch / 4 + j;
                        *pixel = SDL_MapRGBA(surface->format, 255, val.x * 255, val.y * 255, val.z * 255);
                    }
                }
#endif
                auto surface_end_time = std::chrono::high_resolution_clock::now();

                // Clear before update
                SDL_RenderClear(renderer);
                // Render the texture
                SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
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