#include "test.h"

#include "lib/mathlib.h"

#include <cmath>
#include <iostream>

Vec4 rotate(Mat4 rot_matrix, Vec4 u);
float parallelogram_area(Vec2 side1, Vec2 side2);
float parallelopiped_volume(Vec3 side1, Vec3 side2, Vec3 side3);
float get_angles(Vec2 u, Vec2 v);

// ============================ rotate() ============================

Test test_a0_math_rotate_common_z90("a0.task5.math.rotate.common.z90", []() {
    Mat4 rot_z90 = Mat4(Vec4(0.0f, 1.0f, 0.0f, 0.0f),
                        Vec4(-1.0f, 0.0f, 0.0f, 0.0f),
                        Vec4(0.0f, 0.0f, 1.0f, 0.0f),
                        Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    Vec4 got = rotate(rot_z90, Vec4(1.0f, 0.0f, 0.0f, 0.0f));
    Vec4 expected = Vec4(0.0f, 1.0f, 0.0f, 0.0f);

    if (Test::differs(got, expected)) {
        printf("Expected : (%f, %f, %f, %f), Got : (%f, %f, %f, %f)\n",
               expected.x, expected.y, expected.z, expected.w, got.x, got.y, got.z, got.w);
        throw Test::error("Rotating +X by 90 degrees about +Z did not produce +Y.");
    }
});

Test test_a0_math_rotate_common_x90("a0.task5.math.rotate.common.x90", []() {
    Mat4 rot_x90 = Mat4(Vec4(1.0f, 0.0f, 0.0f, 0.0f),
                        Vec4(0.0f, 0.0f, 1.0f, 0.0f),
                        Vec4(0.0f, -1.0f, 0.0f, 0.0f),
                        Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    Vec4 got = rotate(rot_x90, Vec4(0.0f, 1.0f, 0.0f, 0.0f));
    Vec4 expected = Vec4(0.0f, 0.0f, 1.0f, 0.0f);

    if (Test::differs(got, expected)) {
        printf("Expected : (%f, %f, %f, %f), Got : (%f, %f, %f, %f)\n",
               expected.x, expected.y, expected.z, expected.w, got.x, got.y, got.z, got.w);
        throw Test::error("Rotating +Y by 90 degrees about +X did not produce +Z. ");
    }
});

// ====================== parallelogram_area() ======================

// A0: parallelogram_area -- Common Case 1
// unit square
Test test_a0_math_pgram_common_unit("a0.task5.math.parallelogram.common.unit", []() {
    float got = parallelogram_area(Vec2(1.0f, 0.0f), Vec2(0.0f, 1.0f));
    float expected = 1.0f;

    if (Test::differs(got, expected)) {
        printf("Expected : %f, Got : %f\n", expected, got);
        throw Test::error("Area of the unit square was not 1.");
    }
});

// slanted parallelogram
Test test_a0_math_pgram_common_slanted("a0.task5.math.parallelogram.common.slanted", []() {
    float got = parallelogram_area(Vec2(3.0f, 1.0f), Vec2(1.0f, 4.0f));
    float expected = 11.0f;

    if (Test::differs(got, expected)) {
        printf("Expected : %f, Got : %f\n", expected, got);
        throw Test::error("Slanted parallelogram area was wrong. Did you use dot instead of cross2D?");
    }
});

// ===================== parallelopiped_volume() =====================

// unit cube
Test test_a0_math_ppiped_common_unit("a0.task5.math.parallelopiped.common.unit", []() {
    float got = parallelopiped_volume(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f),
                                      Vec3(0.0f, 0.0f, 1.0f));
    float expected = 1.0f;

    if (Test::differs(got, expected)) {
        printf("Expected : %f, Got : %f\n", expected, got);
        throw Test::error("Volume of the unit cube was not 1.");
    }
});

// sheared box
Test test_a0_math_ppiped_common_sheared("a0.task5.math.parallelopiped.common.sheared", []() {
    float got = parallelopiped_volume(Vec3(2.0f, 0.0f, 0.0f), Vec3(1.0f, 3.0f, 0.0f),
                                      Vec3(0.0f, 0.0f, 4.0f));
    float expected = 24.0f;

    if (Test::differs(got, expected)) {
        printf("Expected : %f, Got : %f\n", expected, got);
        throw Test::error("Sheared parallelopiped volume was wrong.");
    }
});

// ============================ get_angles() ============================

Test test_a0_math_angles_common_perpendicular("a0.task5.math.get_angles.common.perpendicular", []() {
    float got = get_angles(Vec2(1.0f, 0.0f), Vec2(0.0f, 1.0f));
    float expected = PI_F / 2.0f;

    if (Test::differs(got, expected)) {
        printf("Expected : %f, Got : %f\n", expected, got);
        throw Test::error("+X to +Y was not +PI/2.");
    }
});

Test test_a0_math_angles_common_diagonal("a0.task5.math.get_angles.common.diagonal", []() {
    float got = get_angles(Vec2(4.0f, 0.0f), Vec2(1.0f, 1.0f));
    float expected = PI_F / 4.0f;

    if (Test::differs(got, expected)) {
        printf("Expected : %f, Got : %f\n", expected, got);
        throw Test::error("Angle from +X to the diagonal was not +PI/4.");
    }
});

//-------------------- DEV CASES ---------------------------