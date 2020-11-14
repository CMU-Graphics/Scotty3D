---
layout: default
title: "rig"
permalink: /guide/rig/
---

# Rig

### Rigging Setup

Select the `Rig` tab to create a skeletal rig for an object.

You can create new bone by first selecting a parent joint and pressing `New Bone`, then click anywhere else on the object to place the bone. From thereon, you can repeat this process to create a chain of bones connected along the selected joint.
If you want to branch off at a joint, simply click on the joint to branch off of, then start another chain by adding a new bone from there.

To view a rigged example, see `media/human.dae` example and select the object in the Rig tab to view its joints.
Once you've implemented **inverse kinematics** the skeleton should be setup like so:

[picture here](TODO)



### Editing Skinning Weight Threshold Radius

Each joint has an associated `Radius`  which controls the part of the mesh influenced by the selected bone during animaton. The radius is visualized by the blue capsule around each bone and can be edited using the menu. The position of the joint can also be edited using the `Extent` values in the menu.

After setting up the skeleton and assigning radii, the mesh can be posed in the [animate](../animate) mode.

<!--

### Toggling Linear Blend Skinning with Threshold

You can press `u` or `U` to toggle between linear blend skinning with or without threshold.

### Inverse Kinematics

Multiple targets can be specified for IK purposes. To specify an additional target, just select a new joint and right click. To clear all targets, press `i` or `I`.
-->



