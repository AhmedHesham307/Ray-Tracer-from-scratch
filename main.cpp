#include <iostream>
#include <fstream>
#include <omp.h>
#include <cmath>
#include <float.h>
#include <SDL.h>
#include <memory>
#include <algorithm>

#include "scene.h"
#include <numeric>
#include <chrono>

#include <random>

#define RENDER_SCENE
// #define TEXTURE_TEST

#define SUN_POS point3(1000, 0, 10)
#define GROUND_COLOR RGB(0.025, 0.05, 0.075)
#define SKYCOLOR_LOW RGB(0.36, 0.45, 0.57)
#define SKYCOLOR_HIGH RGB(0.14, 0.21, 0.49)
#define SUN_COLOR RGB(.9, .9, .9)

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
    double sun_amount = std::pow((v.dot(SUN_POS.normalize()) + 1) / 2, 100);
    sun_amount = std::min(std::max(sun_amount, 0.), 1.);
    return vec3::lerp(skyColor, SUN_COLOR, sun_amount);
}

Material random_material(std::uniform_int_distribution<> &distribution, std::mt19937 &gen)
{
    return Material(RGB(distribution(gen) / 255., distribution(gen) / 255., distribution(gen) / 255.), distribution(gen) / 255., distribution(gen) / 255., distribution(gen) / 255., distribution(gen) / 255.);
}

/*
 * diffuse lighting component: The fraction of light that is spread equally in all directions
 */
RGB diffuse_shading(vec3 pos, vec3 normal, Light l)
{
    vec3 light_vec = l.position - pos;
    vec3 light_dir = light_vec.normalize();
    // This is a standard, physically based(tm) diffuse lighting calculation
    double lambertian = light_dir.dot(normal.normalize());
    // scale by inverse square distance from the light source
    return lambertian > 0 ? l.color * lambertian : RGB(0, 0, 0);
}

/*
 * map linear RGB (the color space used for shading) to an approximation of the ACES tone mapping curve (color that looks good on screen but is a pain to perform shading calculations with)
 * adapted from an assignment from course IN0039
 */
RGB tone_mapped(RGB x)
{
    return ((x * ((x * 2.51) + vec3(0.03, 0.03, 0.03)) / (x * ((x * 2.43) + vec3(0.59, .59, .59)) + vec3(0.14, .14, .14))).clamp()).pow(1.0 / 2.2);
}

/*
 * specular lighting component: The fraction of lights that forms highlights on glossy objects
 */
RGB specular(vec3 pos, vec3 normal, Light l, vec3 view_dir)
{
    // Blinn-Phong specular
    view_dir = view_dir.normalize();
    normal = normal.normalize();
    vec3 light_vec = l.position - pos;
    vec3 light_dir = light_vec.normalize();

    vec3 halfway = (view_dir + light_dir).normalize();
    double result = halfway.dot(normal);
    // scale by inverse square distance from the light source
    return result > 0 ? l.color * result : RGB(0, 0, 0);
}

/*
 * Find the intersection with scene geometry that is closest to the ray origin
 */
Collision find_closest_hit(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r)
{
    // create placeholder collision with highest possible distance and no intersection
    Collision col = Collision(-1, vec3(0, 0, 0), false);
    col.hit_object_index = -1;

    // check all scene objects for a collision closer to the camera than the current closest collision
    for (int j = 0; j < scene.size(); j++)
    {
        Collision wall_col = scene.at(j)->intersect(r);
        if (wall_col.distance > 0 && (wall_col.distance < col.distance || col.distance < 0))
        {

            col = wall_col;
            col.hit_object_index = j;
        }
    }
    return col;
}

RGB recursive_ray_tracing(const std::vector<std::unique_ptr<SceneGeometry>> &scene, const std::vector<Light> lights, ray r, int remaining_iterations = 10)
{
    Collision col = find_closest_hit(scene, r);
    if (col.distance < 0)
    {
        return out_color(r.direction);
    }
    else
    {
        vec3 pos = r.origin + r.direction * col.distance;
        Material mat = scene.at(col.hit_object_index)->get_material();

        RGB diffuse_light = RGB(0, 0, 0);
        RGB specular_light = RGB(0, 0, 0);

        point3 start_pos = pos + col.normal * .0001;

        // sun sample

        ray sun_ray = ray((SUN_POS - start_pos).normalize(), start_pos);
        Collision light_sample = find_closest_hit(scene, sun_ray);
        if (light_sample.distance < 0 || light_sample.distance > (SUN_POS - start_pos).length())
        {
            diffuse_light = diffuse_light + diffuse_shading(start_pos, col.normal, Light(SUN_POS, SUN_COLOR));
            specular_light = specular_light + specular(pos, col.normal, Light(SUN_POS, SUN_COLOR), r.direction * (-1)).pow(mat.specular_exponent);
        }

        // sample the lights in the scene
        for (int i = 0; i < lights.size(); i++)
        {
            ray light_ray = ray((lights.at(i).position - start_pos).normalize(), start_pos);
            Collision light_sample = find_closest_hit(scene, light_ray);
            if (light_sample.distance < 0 || light_sample.distance > (SUN_POS - start_pos).length())
            {
                diffuse_light = diffuse_light + diffuse_shading(start_pos, col.normal, lights.at(i));
                specular_light = specular_light + specular(pos, col.normal, lights.at(i), r.direction * (-1)).pow(mat.specular_exponent);
            }
        }

        RGB local_color = mat.color * (diffuse_light * mat.diffuse) + specular_light * mat.specular + mat.color * mat.ambient;
        if (remaining_iterations <= 0)
        {
            return local_color;
        }

        // start new ray minimally offset from the surface so that the new ray can not hit the surface again

        vec3 reflected_dir = r.direction.reflect(col.normal);
        ray next_ray = ray(reflected_dir, start_pos);

        RGB rt_color = recursive_ray_tracing(scene, lights, next_ray, remaining_iterations - 1);

        return vec3::lerp(local_color, rt_color, mat.metallic);
    }
}

void rt_scene(const std::vector<std::unique_ptr<SceneGeometry>> &scene, const std::vector<Light> lights, const Camera &cam, std::vector<std::vector<RGB>> &frame_buffer)
{
    double step_x = (1. / static_cast<double>(SCREEN_WIDTH));
    double step_y = (1. / static_cast<double>(SCREEN_HEIGHT));
#pragma omp parallel for firstprivate(step_x, step_y) schedule(dynamic)
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (int j = 0; j < SCREEN_WIDTH; j++)
        {
            // do not sample full image space range from 0 to 1, sample pixel centers instead
            double screen_space_x = step_x * j + .5 * step_x;
            double screen_space_y = step_y * i + .5 * step_y;
            vec3 sample_dir = cam.view_dir(screen_space_x, screen_space_y);
            ray camera_ray(sample_dir, cam.position);

            frame_buffer.at(i).at(j) = recursive_ray_tracing(scene, lights, camera_ray);
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

    std::vector<Light> lights = {};

    lights.push_back(Light(point3(0, 10, 10), RGB(.3, .3, 0)));
    lights.push_back(Light(point3(0, -10, 10), RGB(0, .3, .3)));

    // scene definition

    // tum blue
    Material tum_mat = Material(RGB(0, 20. / 255., 50. / 255.), .2, 1, .05, 1, 10);
    Material floor_mat = Material(RGB(1, 1, 1), .2, .2, .2, .2, 10);
    double tum_distance = 40;

    // tum logo
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 9, 4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 7, 4), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 7, 2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 7, 0), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 7, -2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 7, -4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 5, 4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 3, 4), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 3, 2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 3, 0), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 3, -2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 3, -4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, 1, -4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -1, 4), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -1, 2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -1, 0), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -1, -2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -1, -4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -3, 4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -5, 4), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -5, 2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -5, 0), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -5, -2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -5, -4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -7, 4), 1, tum_mat));

    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -9, 4), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -9, 2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -9, 0), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -9, -2), 1, tum_mat));
    scene.push_back(std::make_unique<Sphere>(point3(tum_distance, -9, -4), 1, tum_mat));

    std::random_device rd;                                // obtain a random number from hardware
    std::mt19937 gen(rd());                               // seed the generator
    std::uniform_int_distribution<> distribution(0, 255); // define the range

    double winding = .2;
    double x_step = .8;
    double radius = 15;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 30; j++)
        {
            scene.push_back(std::make_unique<Sphere>(point3(25 + j * x_step, radius * std::sin(winding * j + i * M_PI), radius * std::cos(winding * j + i * M_PI)), 1, random_material(distribution, gen)));
        }
    }

    int frame_number = 0;

    std::vector<int64_t> total_times = {};
    std::vector<int64_t> rt_times = {};
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
        window = SDL_CreateWindow("ADP raytracer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
                The code for rotating the view. We would have liked to implement camera movement like in first person games,
                but unfortunately, the functionality necessary to use relative mouse movements without movements being blocked
                by the window borders is not available for WSL2 GUI windows. We tried both capturing the cursor and warping the
                cursor to the center of the window. Neither works in WSL2, but they break the close window button.
                The final product is kinda janky, but it allows for unlimited rotation, unlike the other implementations, which
                would limit how far the camera can be rotated.
                Also, this solution does not mess with the close window button, like the other implementations did.
                */
                int x, y = 0;
                SDL_GetMouseState(&x, &y);
                float x_input = (x - SCREEN_WIDTH / 2) / static_cast<double>(SCREEN_WIDTH / 2);
                float y_input = (y - SCREEN_HEIGHT / 2) / static_cast<double>(SCREEN_HEIGHT / 2);
                cam.rotate_left_right(-x_input * .05);
                cam.rotate_up_down(-y_input * .05);
                auto rt_start_time = std::chrono::high_resolution_clock::now();
                // std::cout << "start raytracing\n";
                //  Render and create the outpainted stencil
                rt_scene(scene, lights, cam, frame_buffer);
                auto rt_end_time = std::chrono::high_resolution_clock::now();

                auto outpainting_end_time = std::chrono::high_resolution_clock::now();

                // std::cout << "shading and outpainting done\n";
                auto shading_end_time = std::chrono::high_resolution_clock::now();

#if defined(RENDER_SCENE)
                Uint8 *pixels = (Uint8 *)surface->pixels;
#pragma omp parallel for firstprivate(pixels)
                for (int i = 0; i < SCREEN_HEIGHT; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {

                        RGB val = tone_mapped(frame_buffer.at(i).at(j));
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
                auto rt_time = std::chrono::duration_cast<std::chrono::milliseconds>(rt_end_time - rt_start_time);
                rt_times.push_back(rt_time.count());
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
                double avg_frame_time = (std::accumulate(total_times.begin(), total_times.end(), 0) / total_times.size());
                double max_fps = 1000.0 / avg_frame_time;
                std::cout << "Number of frames: " << frame_number << " : " << avg_frame_time << " ms average frame time"
                          << " --> " << max_fps << " max fps\n";
                std::cout << "   " << (std::accumulate(rt_times.begin(), rt_times.end(), 0) / rt_times.size()) << " milliseconds for average raytracing\n";
                std::cout << "   " << (std::accumulate(surface_update_times.begin(), surface_update_times.end(), 0) / surface_update_times.size()) << " milliseconds for average tone mapping and surface update\n";
                std::cout << "   " << ((std::accumulate(sdl_rendering_times.begin(), sdl_rendering_times.end(), 0) / sdl_rendering_times.size())) << " milliseconds for average SDL rendering\n";

                // producing log files
                std::ofstream outFile1("../frametime.log");
                if (outFile1.is_open())
                {
                    for(int i = 0; i < total_times.size(); i++){
                        outFile1 << total_times.at(i) << std::endl;
                    }
                    outFile1.close();
                    std::cout << "frame times saved to 'frametime.log' file." << std::endl;
                }

            }

            return 0;
        }
    }
}
