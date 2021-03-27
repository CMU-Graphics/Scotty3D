---
layout: default
title: "A2: MeshEdit"
permalink: /meshedit/
nav_order: 5
has_children: true
has_toc: false
---

# MeshEdit Overview

MeshEdit is the first major component of Scotty3D, which performs 3D modeling, subdivision, and mesh processing. When implementation of this tool is completed, it will enable the user to transform a simple cube model into beautiful, organic 3D surfaces described by high-quality polygon meshes. This tool can import, modify, and export industry-standard COLLADA files, allowing Scotty3D to interact with the broader ecosystem of computer graphics software.

The `media/` subdirectory of the project contains a variety of meshes and scenes on which the implementation may be tested. The simple `cube.dae` input should be treated as the primary test case -- when properly implemented MeshEdit contains all of the modeling tools to transform this starting mesh into a variety of functional and beautiful geometries. For further testing, a collection of other models are also included in this directory, but it is not necessarily reasonable to expect every algorithm to be effective on every input. The implementer must use judgement in selecting meaningful test inputs for the algorithms in MeshEdit.

The following sections contain guidelines for implementing the functionality of MeshEdit:

- [Halfedge Mesh](halfedge)
- [Local Mesh Operations](local)
  - [Tutorial: Edge Flip](local/edge_flip)
  - [Beveling](local/bevel)
- [Global Mesh Operations](global)
  - [Triangulation](global/triangulate)
  - [Linear Subdivision](global/linear)
  - [Catmull-Clark Subdivision](global/catmull)
  - [Loop Subdivision](global/loop)
  - [Isotropic Remeshing](global/remesh)
  - [Simplification](global/simplify)

As always, be mindful of the [project philosophy](..).
