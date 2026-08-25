#include <iostream>
#include <vector>
#include <algorithm>

struct Point {
    int x;
    int y;
};

struct Triangle {
    Point v1, v2, v3;
    int depth;
};

/**
 * In computer graphics, an important concept 
 * is being able to sort the shapes you see on your screen by their depth. 
 * This allows the user an impression of depth even though 
 * we are on displaying everything on a 2D screen. 
 * Here we give you a very simple triangle class, 
 * and your job is given a vector of randomly generated Triangle Primitives, 
 * use iterators to sort the array. 
 * 
 * hint: std::sort() is useful!
 */
void sort_prims(std::vector<Triangle> &prims) {
    
}






