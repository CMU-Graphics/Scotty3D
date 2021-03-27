---
layout: default
title: "Layout"
permalink: /guide/layout_mode/
parent: User Guide
---

# Layout

This is the main scene editing mode in Scotty3D, and does not contain tasks for the student to implement.
This mode allows you to load full scenes from disk, create or load new objects, export your scene (COLLADA format), and edit transformations that place each object into your scene.

## Creating Objects

There are three ways to add objects to your scene:
- `Import New Scene`: clears the current scene (!) and replaces it with objects loaded from a file on disk.
- `Import Objects`: loads objects from a file, adding them to the current scene.
- `New Object`: creates a new object from a choice of various platonic solids.

To save your scene to disk (including all meshes and their transformations) use the `Export Scene` option.

Scotty3D supports loading objects from the following file formats:
- dae (COLLADA)
- obj
- fbx
- gltf / glb
- 3ds
- stl
- blend
- ply

Scotty3D only supports exporting scenes to COLLADA.

## Managing Objects

Left clicking on or enabling the check box of your object under `Select an Object` will select it. Information about that object's transformation will appear under `Edit Object` beneath the "Select an Object" options.

Under `Edit Object`, you may directly edit the values of the object's position, rotation (X->Y->Z Euler angles), and scale. Note that clicking and dragging on the values will smoothly scale them, and Control/Command-clicking on the value will let you edit it as text.

You can also edit the transformation using the `Move`, `Rotate`, and `Scale` tools. One of these options is always active. This determines the transformation widgets that appear at the origin of the object model.
- `Move`: click and drag on the red (X), green (Y), or blue (Z) arrow to move the object along the X/Y/Z axis. Click and drag on the red (YZ), green (XZ), or blue (XY) squares to move the object in the YZ/XZ/XY plane.
- `Rotate`: click and drag on the red (X), green (Y), or blue (Z) loop to rotate the object about the X/Y/Z axis. Note that these rotations are applied relative to the current pose, so they do not necessarily correspond to smooth transformations of the X/Y/Z Euler angles.
- `Scale`: click and drag on the red (X), green (Y), or blue(Z) block to scale the object about the X/Y/Z axis. Again note that this scale is applied relative to the current pose.

Finally, you may remove the object from the scene by pressing `Delete` or hitting the Delete key. You may swap to `Model` mode with this mesh selected by pressing `Edit Mesh`. Note that if this mesh is non-manifold, this option will not appear.

## Key Bindings

| Key                   | Command                                            |
| :-------------------: | :--------------------------------------------:     |
| `m` | Use the `Move` tool. |
| `r` | Use the `Rotate` tool. |
| `s` | Use the `Scale` tool. |
| `delete` | Delete the currently selected object. |

## Demo

<video src="{{ site.baseurl }}/guide/layout_mode/layout.mp4" controls preload muted loop style="max-width: 100%; margin: 0 auto;"></video>
