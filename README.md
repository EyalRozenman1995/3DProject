# 3DProject
Topographic photo to 3D display app using OpenGL libary


The input of the program is a color-coded height image, in which low heights are represented by shades of blue and high heights by shades of red.
After translating the image into a network of triangles by traversing the image matrix with a constant step (determined by the StepSizeInPixels variable), where the step size determines the resolution (larger step = lower resolution), we sample the image at the vertex positions of each triangle to extract color and height information.
The color of the vertex is the color of the corresponding point in the image.
The height of the vertex is computed as height(v)=B+(R-B)*G, where R, G, and B are the color components of the original image.
The idea behind the formula is to assign high values to points with high R values and low values to points with high B values, assuming that the input image is composed in such a way.


Transformations/Scaling/Rotations:
Scaling (Zoom In/Out): Pressing the up/down arrows increases/decreases the scaleValue variable.
In each iteration of the main loop, the glScalef function is called with values dependent on this variable.

Transformations (Up/Down): Our implementation includes a transformation to the center of the map and translation on the y-axis with the 'W' and 'S' keys.

Rotations: Rotation around the vector (1,0,1) is done by pressing the 'Z' and 'X' keys.
Rotation around the y-axis is done by pressing the 'A' and 'D' keys.

Picking:
Our implementation uses color picking as described in the OpenGL documentation, by coloring each triangle with a unique id based on its index in the triangle vector.

Multi-resolution:
As mentioned earlier, the resolution is determined by the stepSizeInPixels variable.
Pressing the 'E' key calls a function that clears the triangle vector and initializes it again.

Fog:
Using built-in functions of OpenGL, Pressing 'F' increases the density of the fog.
When the fog density is greater than 0, before rendering the scene, GLclearColor is called with a gray color to match a more realistic appearance of fog even outside the map.

Rain:
Implementation of a simple particle system simulation for rain.
Each raindrop has random fields: position, velocity, and color (in shades of gray-blue).
In the main loop, an update is performed for each raindrop separately, based on its previous position and velocity. When a raindrop goes beyond the borders of the map, it is randomly initialized back to its starting position.
The number of raindrops can be increased up to 15,000 by pressing 'Q'.




