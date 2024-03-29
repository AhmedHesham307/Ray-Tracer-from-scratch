# Raytracing Project
### by Advanced Programming Team 85
## Third Sprint
![Screenshot from the current version](images/Sprint3.png)
1. New cmake based build procedure:
    1. Make sure you have installed SDL for rendering to screen. On Debian-derived systems, i.e. Ubuntu or Mint, you can install this library with the command `sudo apt install libsdl2-dev`. You also have to install OpenMP for parallel execution on multicore CPUs.
    2. `mkdir build && cd build` to create the build directory and switch to that directory.
    3. `cmake ..` to configure the project. The path to the SDL headers should be detected automatically.
    4. `make` to build the project.
    5. `./RaytracerADP` to run the executable.
2. Created a pure 3d scene in contrast to second sprint, we replaced the `Circle` with a `Sphere` class and `Wall` is now 3d.
3. Calculated the average time per frame and and log frame times using `ofstream`. `.log` files will be created in your main directory "outside of build folder".
4. used `OpenMP` to speed up raytracing (lines of the image are distributed across hardware threads) and tone mapping (a new addition with sprint 3).
5. Added a sky to the scene and a `sun` represented as a global lighting in the space.
6. You can now move and rotate the camera in 3d using the mouse and arrow keys. Unfortunately we are still restricted by the limitations of WSL2 GUIs which do not allow for constraining the cursor to the window. The camera still rotates based on the cursor being away from the center, not based on mouse movement like in any first person game.
7. Performance optimization:
To increase performance we implemented parallelization on multicore CPUs using OpenMP. On a 20 thread Intel 13900H using all cores gives an almost linear speedup on raytracing and about 8x speedup for tonemapping. With higher numbers of geometry primitives, acceleration structures would greatly enhance raytracing performance, as would vectorized checking for collision with multiple primitives/bounding boxes. We did not implement an acceleration structure because that would be way out of the scope of an advanced programming bonus project. 
## Second Sprint
![Screenshot from the previous version](images/Sprint1.png)
1. New cmake based build procedure: 
2. Refactor the scene geometry description to utilize object oriented programming features. Both `Circle` and `Box` now inherit from `SceneGeometry`, a purely virtual class providing an interface for intersection with rays. 
3. Implement movement using the arrow keys/WASD and camera rotation using the mouse. To rotate the camera to the left, move the cursor to the left half of the screen. To rotate right, move the cursor to the right half of the screen. If you do not want to rotate the camera, move the mouse to the center of the screen. We understand that this is janky and kinda suboptimal, but it is the only mouse input that consistently works with WSL2 GUI windows. Warping the cursor to the center of the screen does not work on WSL2, so with standard First-Person-Game-style camera rotation based on the movement (not the position) of the mouse, rotation would be limited by the cursor reaching the edges of the GUI window. The current implementation at least allows for unlimited rotation in every direction.
4. Color was invented.
5. The average time taken by every step of the image generation is printed to the console after the program ends.
6. Implemented the Blinn-Phong local illumination model https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model
7. Switched from deferred rendering to recursive raytracing. Enjoy the reflections!

## First Sprint
This is the result of the first sprint. Our implementation supports all the features from the requirements:
1. Definition of a camera, rays and scene objects. A camera is a struct that has a member function to create rays originating at the camera. The most important members of a camera are the position and the orientation (as an angle in radians). A ray is generated first in local space of the camera based on the lens properties and then transformed to world space by applying the rotation and the translation of the camera to it.
2. There are multiple one-dimensional vector containers for storing the scene depth, whether an object was hit by the camera ray, and the normal vector of the object that was hit.
3. There are functions for calculating the intersection points of a wall (represented as a ray with a specified maximum length) or a circle (specified by a center and a radius).
4. There are multiple modes for rendering the output. 
    1. simple mode displays the normalized depth (normalizing the depth range to the 0 to 1 range) with an ASCII art palette by https://paulbourke.net/dataformats/asciiart/
    Dots represent the closest geometry, dollar signs are used for the pixels furthest away from the camera. Pixels for which no geometry was hit are indicated by blanks. The test scene consists of a circle right in front of the camera and two angled walls on the sides of the frame. The camera is pointed in the positive x direction.
    2. outpainted mode displays a 2d boolean buffer where the second image dimension is calculated based only on the depth in the 1d depth buffer created by raytracing. This 2d buffer can be used i.e. by shaded mode as a stencil for determining how far the 1d image band must be stretched to achieve a primitive 3d effect.
    3. shaded display presents a 2d ASCII art image shaded with lambertian (diffuse) shading that is created using the data from the 1d buffers and outpainted using the 2d stencil created from the 1d depth. The light source is positioned at (0, 2). We can observe that the left (light-facing) side of the circle is shaded brighter than the right side. The right wall is shaded brighter because it is hit by the light at a steeper angle.
