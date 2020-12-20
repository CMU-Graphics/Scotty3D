---
layout: default
title: "Animation Overview"
permalink: /animation/
---

# Animation Overview

There are four primary components that must be implemented to support Animation functionality.

**A4.0**

- [(Task 1) Spline Interpolation](splines.md)
- [(Task 2) Skeleton Kinematics](skeleton_kinematics.md)

**A4.5**
- [(Task 3) Linear Blend Skinning](skinning.md)
- [(Task 4) Particle Simulation]() (coming soon)

Each task is described at the linked page.

## Converting Frames to Video

Additionally, we will ask you to create your own animation. Once you've rendered out each frame of your animation, you can combine them into a video by using:

`ffmpeg -r 30 -f image2 -s 640x360 -pix_fmt yuv420p -i ./%4d.png -vcodec libx264 out.mp4`

You may want to change the default `30` and `640x360` to the frame rate and resolution you chose to render at.

If you don't have ffmpeg installed on your system, you can get it through most package managers, or you can [download it directly](https://ffmpeg.org/download.html). Alternatively, you may use your preferred video editing tool.



