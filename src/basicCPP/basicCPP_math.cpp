#include "lib/mathlib.h"

/**
 * using the math libraries in Scotty3D, 
 * fill in this function so that the vector is rotated
 */
Vec4 rotate(Mat4 rot_matrix, Vec4 u) {
    return {};
}

/**
 * Using the math libraries given in Scotty3D, 
 * compute the area of a parallelogram given 2 sides
 * area should be nonnegative
 *
 * You may not create any extra functions here, 
 * instead try editing the vec2 header library 
 * so you have a new helper method for cross products of 2D vecs!
 * 
 * Hint: wikipedia is useful!
 */
float parallelogram_area(Vec2 side1, Vec2 side2) {
    return 0.0f;
}

/**
 * Using the math libraries given in Scotty3D, 
 * compute the nonnegative volume of a parallelopiped given 3 sides
 * 
 * Hint: wikipedia is useful!
 * 
   +--------+
  /        /|
 /        / |
+--------+  | side3
|        |  |
|        |  +
|        | /
|        |/ side2
+--------+
 side1
 * 
 */
float parallelopiped_volume(Vec3 side1, Vec3 side2, Vec3 side3) {
    return 0.0f;
}

/**
 * In graphics programming, finding the angles between 
 * two vectors is essential for various applications, such as 
 * collision detection. Given the blank function, 
 * derive the angle between vectors u and v 
 * 
 * Direction of the angle is from u to v, signed 
 * 
 * Hint: atan2 may prove to be useful! 
 * (and the helper library function you created above)
 */
float get_angles(Vec2 u, Vec2 v) {
    return 0.0f;
}








