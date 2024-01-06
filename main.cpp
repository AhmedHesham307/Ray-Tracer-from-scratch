#include <iostream>

#include <cmath>
#include <float.h>
#include <SDL.h>
#include <memory>
#include <algorithm>
#include "vec2.h"
#include "scene.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
constexpr float aspect = 4 / 3;

enum DisplayMode
{
    simple,
    outpainted,
    shaded
};

double diffuse_shading(vec2 pos, vec2 normal, vec2 light_pos)
{
    vec2 light_dir = (light_pos - pos).normalize();
    double lambertian = light_dir.dot(normal.normalize());
    return lambertian > 0 ? lambertian : 0;
}

void shaded_frame_buffer(const std::vector<bool> &hits, const std::vector<vec2> &normals, const std::vector<vec2> &positions, std::vector<double> &shaded)
{

    for (int i = 0; i < positions.size(); i++)
    {
        if (hits[i])
        {
            shaded[i] = diffuse_shading(positions[i], normals[i], vec2(0, 2));
        }
        else
        {
            shaded[i] = -1;
        }
    }
}

void rt_scene(const std::vector<std::unique_ptr<SceneGeometry>> &scene, const Camera &cam, std::vector<bool> &hits, std::vector<double> &depth, std::vector<vec2> &normals, std::vector<vec2> &positions)
{
    double step = (1. / static_cast<double>(SCREEN_WIDTH));
    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        // do not sample full image space range from 0 to 1, sample pixel centers instead
        double progress = step * i + .5 * step;

        vec2 sample_dir = cam.view_dir(progress);
        ray camera_ray(sample_dir, cam.position);

        // std::cout << camera_ray.direction.x << " " << camera_ray.direction.y << std::endl;

        // create placeholder collision with highest possible distance and no intersection
        Collision col = Collision(DBL_MAX, vec2(0, 0), false);

        // check all scene objects for a collision closer to the camera than the current closest collision
        for (int i = 0; i < scene.size(); i++)
        {
            Collision wall_col = scene.at(i)->intersect(camera_ray);

            if (wall_col.hit == true && wall_col.distance < col.distance)
                col = wall_col;
        }

        // a collision was found. Col is the closest intersection.
        if (col.hit)
        {
            hits[i] = true;
            depth[i] = col.distance;
            normals[i] = col.normal;
            positions[i] = camera_ray.origin + camera_ray.direction * col.distance;
            
        }
    }

}

int main(int argc, char *args[])
{
    Camera cam(vec2(1, 0), point2(0, 0), 35, 35);

    // containers for the scene objects
    std::vector<std::unique_ptr<SceneGeometry>> scene = {};

    // scene definition
    scene.push_back(std::make_unique<Circle>(point2(5, 0), 1));

    scene.push_back(std::make_unique<Wall>(vec2(2, -1), point2(4, 3), 1));
    scene.push_back(std::make_unique<Wall>(vec2(1, .5), point2(4, -3), 2));

    // SDL setup adapted from the resource linked in the task description
    // https://lazyfoo.net/tutorials/SDL/01_hello_SDL/index2.php
    SDL_Window *window = NULL;
    SDL_Surface *screenSurface = NULL;
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
            SDL_Surface *surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
            if (!surface)
            {
                std::cerr << "Surface creation failed: " << SDL_GetError() << std::endl;
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }
            // Get window surface
            screenSurface = SDL_GetWindowSurface(window);
            SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (!renderer)
            {
                std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }
            /* Texture modification test
            for (int y = 0; y < SCREEN_HEIGHT; ++y)
            {
                for (int x = 0; x < SCREEN_WIDTH; ++x)
                {
                    Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + y * surface->pitch / 4 + x;
                    *pixel = SDL_MapRGB(surface->format, ((float)x) / SCREEN_WIDTH * 255, ((float)y) / SCREEN_HEIGHT * 255, 0); // Set pixel to red
                }
            }
            */

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
            The 2d image is created by stretching the 1d information dependant on the depth
            */
            std::vector<double> depth(SCREEN_WIDTH, -1);
            std::vector<bool> hits(SCREEN_WIDTH, false);
            std::vector<vec2> normals(SCREEN_WIDTH, vec2(1, 0));
            std::vector<vec2> positions(SCREEN_WIDTH, vec2(0, 0));

            //screen buffer
            std::vector<std::vector<bool>> hits2d(SCREEN_HEIGHT, std::vector<bool>(SCREEN_WIDTH, false));
            std::vector<double> shaded_buffer(SCREEN_WIDTH, 0);

            SDL_Event e;
            bool quit = false;
            while (quit == false)
            {
                // Render and create the outpainted stencil
                rt_scene(scene, cam, hits, depth, normals, positions);
                cam.outpainting(depth, hits, hits2d);

                // Create the shaded 1d image strip
                shaded_frame_buffer(hits, normals, positions, shaded_buffer);

                for(int i = 0; i < SCREEN_HEIGHT; i++){
                    for(int j = 0; j < SCREEN_WIDTH; j++){
                        Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + i * surface->pitch / 4 + j;

                        double val = 0;
                        if(hits2d.at(i).at(j)){
                            val = shaded_buffer.at(j);
                        }
                        *pixel = SDL_MapRGB(surface->format, val * 255, val * 255, val * 255); 
                    }
                }

                // Clear before update
                SDL_RenderClear(renderer);
                // Render the texture
                SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
                SDL_RenderCopy(renderer, texture, NULL, NULL);

                // Update the window surface
                SDL_RenderPresent(renderer);
                while (SDL_PollEvent(&e))
                {
                    if (e.type == SDL_QUIT)
                        quit = true;
                }
            }
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_FreeSurface(surface);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 0;
        }
    }

    
}