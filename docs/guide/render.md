---
layout: default
title: "Render"
permalink: /guide/render_mode/
parent: User Guide
---

# Render

Welcome! This is Scotty3D's realistic, globally illuminated renderer, capable of creating images of complex scenes using path tracing.

## Render Window

In render mode, click on "Open Render Window", and you will be able to set the parameters to render your model. Enjoy the excitement of seeing the images becoming clearer and clearer ;-)

![light](window.png)

## Moving Camera

The render mode comes with its own camera, representing the position, view direction, field of view, and aspect ratio with which to render the scene. These parameters are visually represented by the camera control cage, which shows up as a black wire-frame pyramid that traces out the unit-distance view plane. Note that changing camera settings (e.g. field of view) will adjust the geometry of the camera cage.

To move the render camera to the current view, click "Move to View." This will reposition the camera so that it has exactly what you have on screen in view.

To freely move the camera without updating its field of view/aspect ratio to match the current viewport, click "Free Move," and use the normal 3D navigation tools to position the camera. When you are done, click "Confirm Move" or "Cancel Move" to apply or cancel the updated camera position. Feel free to play around with the field of view (FOV) and aspect ratio (AR) sliders while in the free move mode - they will adjust your current view to use these values, so you can see how exactly the effect the visible region.

## Create light

To create a lighting for your scene, simply go to the menu on the left side, click "New Light", and you will be able to choose from a variaty of light objects and environmental lights. (you will implement the support for environmental light in Task 7. See the corresponding documentation for more guide.)

![light](light.png)


## Enable Ray Logging for Debugging

In Render mode, simply check the box for "Logged Rays", and you would be able to see the camera rays that you generated in task 1 when you start render.

![ray](log_ray.png)

## Visualize BVH

In Render mode, simply check the box for "BVH", and you would be able to see the BVH you generated in task 3 when you start rendering. You can click on the horizontal bar to see each level of your BVH.

![ray](bvh.png)

## Materials and Other Object Options

You can change the material and other property of your mesh by selecting the object and choose "Edit Pose", "Edit Mesh", and "Edit Material". For example, you can make a colored cow by "Edit Material" -> "Diffuse light", and pick a color that you like.

![material](material.png)
