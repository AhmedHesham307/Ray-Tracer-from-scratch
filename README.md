# Raytracing Project
### by Advanced Programming Team 85

This is the result of the first sprint. Our implementation supports all the features from the requirements:
1. Definition of a camera, rays and scene objects. A camera is a struct that has a member function to create rays originating at the camera. The most important members of a camera are the position and the orientation (as an angle in radians). A ray is generated first in local space of the camera based on the lens properties and then transformed to world space by applying the rotation and the translation of the camera to it.
2. There are multiple one-dimensional vector containers for storing the scene depth, whether an object was hit by the camera ray, and the normal vector of the object that was hit.
3. There are functions for calculating the intersection points of a wall (represented as a ray with a specified maximum length) or a circle (specified by a center and a radius).
4. There are multiple modes for rendering the output. 
    1. simple mode displays the normalized depth (normalizing the depth range to the 0 to 1 range) with an ASCII art palette by https://paulbourke.net/dataformats/asciiart/
    2. outpainted mode displays a 2d boolean buffer where the second image dimension is calculated based only on the depth in the 1d depth buffer created by raytracing.
    3. shaded display presents a 2d ASCII art image shaded with lambertian shading that is created using the data from the 1d buffers and outpainted using the 2d stencil created from the 1d depth.