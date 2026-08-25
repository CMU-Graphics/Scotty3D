#include <stack> 
#include <iostream>
#include <vector>
#include "lib/mathlib.h"

struct Object {
    int index;
    int vertices;
    int center;

    Mat4 transformation;

    std::vector<Object *> child_objs;
};

/**
 * Scene Graphs are an important concept that you will learn 
 * in the next couple lectures, and they help create a graph structure
 * for various scenes comprised of objects. They establish a 
 * parent-child relationship, which is essential for relating 
 * different spaces in graphics. 
 * 
 * Given an acyclic (no loops) scene graph, 
 * return the Object with index goal_index with an iterative depth-first traversal. 
 *                                               (hint: stacks)
 * If a node cannot be found, return a nullptr.
 * The graph should remain unchanged.
 */
Object *find_node(Object *root, int goal_index) {
    Object obj = new Object;
    return obj;
}