# Octtree based space-skipping

I originally built this as a proof of concept, the concept being that it wouldnt be that good

spatial subdivision for rendering sdfs is good, generally, and improves performance by a lot, just not this method

## the idea

start with a bounding box, then subdivide if the bounding box contains the sdf volume, and if the box is not at the minimum size.

continue doing this for the smaller bounding boxes

after doing this, you can be sure that if a box is not the minimum size, then it does not contain any of the sdf volume, and can use a box-intersecion in order to skip to the next box

you can traverse this octtree by using a depth first search:

1. if current box doesnt contain ray, (if possible) go back 1 level of depth , go back to 1.
2. if current box does contain ray, check if it is subdivided, if it is, use math to go to the correct subdivision, go back to 1.
3. current box is not subdivided, if it is a minimum-size box, then it contains the volume, start raymarching
4. if not a minimum-size box, use box intersection formula to skip the current box and go back 1 level of depth
5. go back to 1.

## the problem

while this is fairly easy to implement recursively, shaders arent good with recursion, and much prefer brute computation where every pixel does the same computations

GPUs have a hierarchy of control, with the lowest group being warps, which are groups of 32 individual hardware threads

the GPU then uses SIMT (single instruction multiple threads) on this group of 32 threads/pixels

if some pixels have to branch, based on a conditional statement, then all the other threads basically just have to wait for them to finish doing their thing

<img width="630" height="252" alt="image" src="https://github.com/user-attachments/assets/f9e1fc16-bf5c-4415-bc22-3fe2667667c1" />

as a result, divergent code, or code where different pixels have very different control flow, is bad for GPUs, and your pixels spend most of their time waiting for other pixels

while the GPU will try to put pixels that are close to each other in the same warp, the octtree is too detailed and chaotic, so even nearby pixels follow different traversal patterns


## other solutions

while this method can work better than plain raymarching for large numbers of sdf volumes, for single volumes like the mandelbulb i used for testing, its better to use bounding spheres around sdf volumes

by having a bounding sphere, you can compute the ray intersection with the sphere, and jump the ray to the sphere instantly if it is going in that direction, otherwise, render the background

after doing that ray intersection, you can get the most out of your raymarch iterations, since you are now close to the volume

you can combine this with a bounding volume hierarchy, in order to optimise this for larger numbers of objects in the scene
