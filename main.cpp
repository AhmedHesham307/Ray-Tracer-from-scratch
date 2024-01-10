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
//#define TEXTURE_TEST

#define LIGHT_POS point2(2, 0)

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

RGB recursive_ray_tracing(const std::vector<std::unique_ptr<SceneGeometry>> &scene, ray r, double &depth){
    Collision col = find_closest_hit(scene, r);
    if(col.hit_object_index < 0){
        return RGB(0,0,0);
    }else{
        depth = col.distance;
        return scene.at(col.hit_object_index)->get_material().color * diffuse_shading(r.origin + r.direction * col.distance, col.normal, LIGHT_POS);
    }
}

void rt_scene(const std::vector<std::unique_ptr<SceneGeometry>> &scene, const Camera &cam, std::vector<double> &depth, std::vector<RGB> &frame_buffer)
{
    double step = (1. / static_cast<double>(SCREEN_WIDTH));
    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        // do not sample full image space range from 0 to 1, sample pixel centers instead
        double progress = step * i + .5 * step;

        vec2 sample_dir = cam.view_dir(progress);
        ray camera_ray(sample_dir, cam.position);

        frame_buffer.at(i) = recursive_ray_tracing(scene, camera_ray, depth.at(i));
    }
}

int main(int argc, char *args[])
{
    Camera cam(vec2(1, 0), point2(0, 0), 35, 35);

    // containers for the scene objects
    std::vector<std::unique_ptr<SceneGeometry>> scene = {};

    // scene definition
    scene.push_back(std::make_unique<Circle>(point2(5, 0), 1, Material(RGB(1, 0, 0),.5)));

    scene.push_back(std::make_unique<Wall>(vec2(2, -1), point2(4, 3), 1, Material(RGB(0, 0, 1), .5)));
    scene.push_back(std::make_unique<Wall>(vec2(1, .5), point2(4, -3), 2, Material(RGB(0, 1, 0), 0)));

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

            /*
            Define Buffers for holding raytracing output
            The per-pixel scene information is translated to an image in another step, shading
            Note that all scene buffers are only one dimensional
            The 2d image is created by stretching the 1d information dependant on the depth. See Camera::Outpainting for details

            We might switch to recursive ray tracing with nice reflections for sprint 3.
            */
            std::vector<double> depth(SCREEN_WIDTH, -1);
            // screen buffer
            std::vector<std::vector<bool>> hits2d(SCREEN_HEIGHT, std::vector<bool>(SCREEN_WIDTH, false));
            std::vector<RGB> frame_buffer(SCREEN_WIDTH, RGB(0, 0, 0));

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
                for (int i = 0; i < SCREEN_HEIGHT; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {
                        hits2d.at(i).at(j) = false;
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
                float input = (x - 400) / 400.;
                cam.rotate(-input * .003);

                for (int j = 0; j < SCREEN_WIDTH; j++)
                {
                    depth.at(j) = -1;
                }

                auto rt_start_time = std::chrono::high_resolution_clock::now();
                // std::cout << "start raytracing\n";
                //  Render and create the outpainted stencil
                rt_scene(scene, cam, depth, frame_buffer);
                auto rt_end_time = std::chrono::high_resolution_clock::now();
                // std::cout << "end raytracing\n";
                cam.outpainting(depth, hits2d);
                auto outpainting_end_time = std::chrono::high_resolution_clock::now();


                // std::cout << "shading and outpainting done\n";
                auto shading_end_time = std::chrono::high_resolution_clock::now();


                #if defined(RENDER_SCENE)
                Uint8 *pixels = (Uint8 *)surface->pixels;
                for (int i = 0; i < SCREEN_HEIGHT; i++)
                {
                    for (int j = 0; j < SCREEN_WIDTH; j++)
                    {

                        RGB val = frame_buffer.at(j);
                        if (!hits2d.at(i).at(j))
                            val = RGB(0, 0, 0);
                        Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + i * surface->pitch / 4 + j;
                        *pixel = SDL_MapRGBA(surface->format, 255, val.x *255, val.y * 255, val.z*255);
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