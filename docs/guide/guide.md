---
layout: default
title: User Guide
permalink: /guide/
nav_order: 4
has_children: true
has_toc: false
---

# User Guide

## Modes and Actions

The basic paradigm in Scotty3D is that there are six different _modes_, each
of which lets you perform certain class of actions. For instance, in `Model` mode, you
can perform actions associated with modeling, such as moving mesh elements and performing global mesh operations.
When in `Animate` mode, you can perform actions associated with animation. Etc.
Within a given mode, you can
switch between actions by hitting the appropriate key; keyboard commands are
listed below for each mode. Note that the input scheme may change depending on
the mode. For instance, key commands in `Model` mode may result in
different actions in `Render` mode.

The current mode is displayed as the "pressed" button in the menu bar, and available actions
are are detailed in the left sidebar. Note that some actions are only available when a model/element/etc. is selected.

## Global Navigation

In all modes, you can move the camera around and select scene elements. Information about your selection will be shown in the left sidebar.

The camera can be manipulated in three ways:
- Rotate: holding shift, left-clicking, and dragging will orbit the camera about the scene. Holding middle click and dragging has the same effect.
- Zoom: using the scroll wheel or scrolling on your trackpad will move the camera towards or away from its center.
- Translate: right-clicking (or using multi-touch on a trackpad, e.g., two-finger click-and-drag) and dragging will move the camera around the scene.

## Global Preferences

You can open the preferences window from the edit option in the menu bar.
- Multisampling: this controls how many samples are used for MSAA when rendering scene objects in the Scotty3D interface. If your computer struggles to render complex scenes, try changing this to `1`.

## Global Undo

As is typical, all operations on scene objects, meshes, etc. are un and re-doable using Control/Command-Z to undo and Control/Command-Y to redo. These actions are also available from the `Edit` option in the menu bar.
