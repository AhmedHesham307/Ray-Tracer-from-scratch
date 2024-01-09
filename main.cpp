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

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const bool performance_logging = true;
constexpr float aspect = 4 / 3;

double diffuse_shading(vec2 pos, vec2 normal, vec2 light_pos)
{
    vec2 light_dir = (light_pos - pos).normalize();
    // This is a standard, physically based(tm) diffuse lighting calculation
    double lambertian = light_dir.dot(normal.normalize());
    return lambertian > 0 ? lambertian : 0;
}

void shaded_frame_buffer(const std::vector<bool> &hits, const std::vector<vec2> &normals, const std::vector<vec2> &positions, std::vector<RGB> &shaded, std::vector<int> &hit_objects, std::vector<std::unique_ptr<SceneGeometry>> &scene)
{

    for (int i = 0; i < positions.size(); i++)
    {
        if (hit_objects.at(i) >= 0)
        {
            double val = diffuse_shading(positions[i], normals[i], vec2(0, 2));
            shaded[i] = scene.at(hit_objects.at(i))->get_color() * val;
        }
        else
        {
            shaded[i] = RGB(0, 0, 0);
        }
    }
}

Collision find_closest_hit(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r)
{
    // create placeholder collision with highest possible distance and no intersection
    Collision col = Collision(DBL_MAX, vec2(0, 0), false);

    // check all scene objects for a collision closer to the camera than the current closest collision
    for (int j = 0; j < scene.size(); j++)
    {
        Collision wall_col = scene.at(j)->intersect(r);

        if (wall_col.hit == true && wall_col.distance < col.distance)
        {
            col = wall_col;
            col.hit_object_index = j;
        }
    }
    return col;
}

void rt_scene(const std::vector<std::unique_ptr<SceneGeometry>> &scene, const Camera &cam, std::vector<bool> &hits, std::vector<int> &hit_object, std::vector<double> &depth, std::vector<vec2> &normals, std::vector<vec2> &positions)
{
    double step = (1. / static_cast<double>(SCREEN_WIDTH));
    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        // do not sample full image space range from 0 to 1, sample pixel centers instead
        double progress = step * i + .5 * step;

        vec2 sample_dir = cam.view_dir(progress);
        ray camera_ray(sample_dir, cam.position);

        // std::cout << camera_ray.direction.x << " " << camera_ray.direction.y << std::endl;
        Collision col = find_closest_hit(scene, camera_ray);
        // a collision was found. Col is the closest intersection.
        if (col.hit)
        {
            hits[i] = true;
            depth[i] = col.distance;
            normals[i] = col.normal;
            positions[i] = camera_ray.origin + camera_ray.direction * col.distance;
            hit_object[i] = col.hit_object_index;
        }
    }
}

int main(int argc, char *args[])
{
    Camera cam(vec2(1, 0), point2(0, 0), 35, 35);

    // containers for the scene objects
    std::vector<std::unique_ptr<SceneGeometry>> scene = {};

    // scene definition
    scene.push_back(std::make_unique<Circle>(point2(5, 0), 1, RGB(1, 0, 0)));

    scene.push_back(std::make_unique<Wall>(vec2(2, -1), point2(4, 3), 1, RGB(0, 0, 1)));
    scene.push_back(std::make_unique<Wall>(vec2(1, .5), point2(4, -3), 2, RGB(0, 1, 0)));

    int frame_number = 0;

    int mouse_previous_x, mouse_previous_y = 0;

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
            /* Texture modification test*/
            for (int y = 0; y < SCREEN_HEIGHT; ++y)
            {
                for (int x = 0; x < SCREEN_WIDTH; ++x)
                {
                    Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + y * surface->pitch / 4 + x;
                    *pixel = SDL_MapRGBA(surface->format, 255, ((float)x) / SCREEN_WIDTH * 255, ((float)y) / SCREEN_HEIGHT * 255, 0);
                }
            }

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
            */
            std::vector<double> depth(SCREEN_WIDTH, -1);
            std::vector<bool> hits(SCREEN_WIDTH, false);
            std::vector<vec2> normals(SCREEN_WIDTH, vec2(1, 0));
            std::vector<vec2> positions(SCREEN_WIDTH, vec2(0, 0));
            std::vector<int> hit_object(SCREEN_WIDTH, -1);

            // screen buffer
            std::vector<std::vector<bool>> hits2d(SCREEN_HEIGHT, std::vector<bool>(SCREEN_WIDTH, false));
            std::vector<RGB> shaded_buffer(SCREEN_WIDTH, RGB(0, 0, 0));

            SDL_Event e;
            bool quit = false;
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

                        default:
                            break;
                        }
                    }

                    else if (e.type == SDL_MOUSEMOTION)
                    {
                        cam.rotate(-e.motion.xrel * .001);
                    }
                }
                for (int i = 0; i < SCREEN_HEIGHT; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {
                        hits2d.at(i).at(j) = false;
                    }
                }

                for (int j = 0; j < SCREEN_WIDTH; j++)
                {
                    depth.at(j) = -1;
                    hit_object.at(j) = -1;
                }

                auto rt_start_time = std::chrono::high_resolution_clock::now();
                // std::cout << "start raytracing\n";
                //  Render and create the outpainted stencil
                rt_scene(scene, cam, hits, hit_object, depth, normals, positions);
                auto rt_end_time = std::chrono::high_resolution_clock::now();
                // std::cout << "end raytracing\n";
                cam.outpainting(depth, hits, hits2d);
                auto outpainting_end_time = std::chrono::high_resolution_clock::now();

                // Create the shaded 1d image strip
                shaded_frame_buffer(hits, normals, positions, shaded_buffer, hit_object, scene);
                // std::cout << "shading and outpainting done\n";
                auto shading_end_time = std::chrono::high_resolution_clock::now();

                Uint8 *pixels = (Uint8 *)surface->pixels;
                for (int i = 0; i < SCREEN_HEIGHT; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {

                        RGB val = shaded_buffer.at(j);
                        if (!hits2d.at(i).at(j))
                            val = RGB(0, 0, 0);

                        Uint8 *r_address = 4 * i * SCREEN_WIDTH + 4 * j + pixels;
                        r_address[0] = val.z * 255; // blue
                        r_address[1] = val.y * 255; // green
                        r_address[2] = val.x * 255; // red
                        r_address[3] = 255;         // alpha
                    }
                }
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