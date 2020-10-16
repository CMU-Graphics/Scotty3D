---
layout: default
title: "Render"
permalink: /guide/render/
---

# Render

Welcome! This is Scotty3D's realistic, globally illuminated pathtracing renderer, capable of creating images of complex scenes using path tracing.

## Moving Camera

## Create light

To create a lighting for your scene, simply go to the menu on the left side, click "New Light", and you will be able to choose from a variaty of light objects and environmental lights. (you will implement the support for environmental light in Task 7. See the corresponding documentation for more guide.)

![light](render_mode/light.png)

## Render Window

Click on "Open Render Window", and you will be able to set the parameters to render your model. Enjoy the excitement of seeing the images becoming clearer and clearer ;-)

![light](render_mode/window.png)

## Enable Logged Ray for Debugging

In Render mode, simply check the box for "Logged Rays", and you would be able to see the camera rays that you generated in task 1.

![ray](render_mode/log_ray.png)


## Visualize BVH

In Render mode, simply check the box for "Logged Rays", and you would be able to see the camera rays that you generated in task 3 when you start rendering. You can click on the horizontal bar to see each level of your BVH.

![ray](render_mode/bvh.png)

## Materials and Other Object Options

You can change the material and other property of your mesh by selecting the object and choose "Edit Pose", "Edit Mesh", and "Edit Material". For example, you can make a colored cow by "Edit Material" -> "Diffuse light", and pick a color that you like.

![material](render_mode/material.png)




