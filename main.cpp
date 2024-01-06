#include <iostream>

#include <cmath>
#include <float.h>
#include <SDL.h>
#include <memory>
#include "vec2.h"
#include "scene.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
constexpr float aspect = 4 / 3;
// ascii art color map from https://paulbourke.net/dataformats/asciiart/
const std::vector<char> cmap = {'$', '@', 'B', '%', '8', '&', 'W', 'M', '#', '*', 'o', 'a', 'h', 'k', 'b', 'd', 'p', 'q', 'w', 'm', 'Z', 'O', '0', 'Q', 'L', 'C', 'J', 'U', 'Y', 'X', 'z', 'c', 'v', 'u', 'n', 'x', 'r', 'j', 'f', 't', '/', '\\', '|', '(', ')', '1', '{', '}', '[', ']', '?', '-', '_', '+', '~', '<', '>', 'i', '!', 'l', 'I', ';', ':', ',', '"', '^', '`', '\'', '.'};

char sample_cmap(double val)
{
    val = val <= 1 ? val : 1;
    val = val >= 0 ? val : 0;
    val = 1 - val;
    int index = val * ((double)cmap.size());
    if (index == cmap.size())
        index--;
    return cmap.at(index);
}

enum DisplayMode
{
    simple,
    outpainted,
    shaded
};

double diffuse_shading(vec2 pos, vec2 normal, vec2 light_pos)
{
    vec2 light_dir = (light_pos - pos).normalize();
    return light_dir.dot(normal.normalize());
}

void create_shaded_frame_buffer(const std::vector<vec2> &positions, const std::vector<vec2> &normals, const std::vector<bool> &hits, std::vector<double> &shaded)
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

void print_shaded_buffer2d_stencil(const std::vector<double> &shaded, const std::vector<std::vector<bool>> stencil)
{
    for (int i = 0; i < stencil.size(); i++)
    {
        for (int j = 0; j < shaded.size(); j++)
        {
            if (stencil.at(i).at(j))
            {
                std::cout << sample_cmap(shaded.at(j));
            }
            else
            {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }
}

void print_buffer_normalized(const std::vector<double> &depth, const std::vector<bool> &hit)
{
    std::cout << std::endl;

    double min_depth = DBL_MAX;
    double max_depth = -DBL_MAX;
    for (int i = 0; i < depth.size(); i++)
    {
        if (hit.at(i))
        {
            double d = depth.at(i);
            if (d < min_depth)
                min_depth = d;
            if (d > max_depth)
                max_depth = d;
        }
    }

    for (int i = 0; i < depth.size(); i++)
    {
        if (hit.at(i))
        {
            double normalized_depth = (depth.at(i) - min_depth) / (max_depth - min_depth);
            std::cout << sample_cmap(normalized_depth);
        }
        else
            std::cout << ' ';
    }
    std::cout << std::endl;
}

void print_bool_buffer2d(const std::vector<std::vector<bool>> buffer)
{
    for (int i = 0; i < buffer.size(); i++)
    {
        for (int j = 0; j < buffer.at(0).size(); j++)
        {
            if (buffer.at(i).at(j))
                std::cout << 'X';
            else
                std::cout << ' ';
        }
        std::cout << "\n";
    }
}

int main(int argc, char *args[])
{
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

            for (int y = 0; y < SCREEN_HEIGHT; ++y)
            {
                for (int x = 0; x < SCREEN_WIDTH; ++x)
                {
                    Uint32 *pixel = static_cast<Uint32 *>(surface->pixels) + y * surface->pitch / 4 + x;
                    *pixel = SDL_MapRGB(surface->format, ((float)x) / SCREEN_WIDTH * 255, ((float)y) / SCREEN_HEIGHT * 255, 0); // Set pixel to red
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
            // Hack to get window to stay up
            SDL_Event e;
            bool quit = false;
            while (quit == false)
            {
                SDL_RenderClear(renderer);

                // Render the texture
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
        }
    }

    Camera cam(vec2(1, 0), point2(0, 0), 35, 35);

    // containers for the scene objects
    std::vector<std::unique_ptr<SceneGeometry>> scene = {};

    // scene definition
    scene.push_back(std::make_unique<Circle>(point2(5, 0), 1));

    scene.push_back(std::make_unique<Wall>(vec2(2, -1), point2(4, 3), 1));
    scene.push_back(std::make_unique<Wall>(vec2(1, .5), point2(4, -3), 2));

    double pixel_aspect = 2.3;
    int resolution = 50;

    DisplayMode mode = simple;

    while (true)
    {
        // initialize buffers. Do not put in front of the loop because resolution might be subject to change
        std::vector<double> depth(resolution, -1);
        std::vector<bool> hits(resolution, false);
        std::vector<vec2> normals(resolution, vec2(1, 0));
        std::vector<vec2> positions(resolution, vec2(0, 0));
        double step = (1. / static_cast<double>(resolution));

        for (int i = 0; i < resolution; i++)
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
        std::vector<std::vector<bool>> hits2d(resolution / (pixel_aspect * aspect), std::vector<bool>(resolution, false));
        std::vector<double> shaded_buffer(resolution, 0);

        switch (mode)
        {
        case simple:
            print_buffer_normalized(depth, hits);
            break;

        case outpainted:
            cam.outpainting(depth, hits, hits2d);
            print_bool_buffer2d(hits2d);
            break;

        case shaded:
            create_shaded_frame_buffer(positions, normals, hits, shaded_buffer);
            cam.outpainting(depth, hits, hits2d);
            print_shaded_buffer2d_stencil(shaded_buffer, hits2d);
            break;

        default:
            break;
        }

        std::cout << "Commands:\n\"simple\" \"outpainted\" \"shaded\": set rendering mode.   <integer>: set screen resolution    \"q\": quit\n";

        std::string input = "";
        std::cin >> input;

        if (input == "simple")
            mode = simple;
        if (input == "outpainted")
            mode = outpainted;
        if (input == "shaded")
            mode = shaded;
        if (input == "quit" || input == "q" || input == "Quit" || input == "Q")
            return 0;

        size_t pos_of_int = 0;

        try
        {
            int input_int = std::stoi(input);
            resolution = input_int;
        }
        catch (const std::exception &e)
        {
            // ignore invalid argument exception from stoi if input was not a number
        }
    }
}