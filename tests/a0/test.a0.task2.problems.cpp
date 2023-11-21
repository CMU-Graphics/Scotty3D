
#include "test.h"

#include <iostream>
#include <numeric>
#include <vector>

// A0T2: Problem 1
// TODO: Uncomment the function below by removing the surrounding /* and */, and run in test mode.
//       You will encounter compilation errors. Fix them, make sure all 
//       compilation errors go away, and move onto the next problem.
// Hint: there are 3 parts that need to be fixed.

/*
Test test_a0_task2_problems_print("a0.task2.problems.print", []() {
    string str = "str";
    int integer = 0;
    float flt = 0.1f;

	// Most common ways of printing a line of text in Scotty3D are:
    printf("\n1. printf with format specifiers such as string %s, interger %d, and float %f.\n", str.c_str(), integer, flt)
    
    std::cour << "2. std::cout and std::endl with multiple insertion operators like " 
              << str + ", " << integer << ", and " << flt << "." << std::endl;
});
*/

// A0T2: Problem 2
// TODO: We want to pass our target 2D vector through a filter called helper, 
//       using the modifiers vector in the process. But for some reason, we 
//       are seeing many more rejections from the filter run, causing more 
//       zeroes to appear in our modified vector. Fix the problem to get 
//       the correct filtered vector.

Test test_a0_task2_problems_numerical("a0.task2.problems.numerical", []() {
    std::vector<std::vector<int>> target = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    std::vector<int> modifiers = {9, 8, 7, 15, 16, 17, 20, 25, 28};

    // This is a lambda function to compare if a third of the modifier is
    // greater than the target value. If so return true.
    // Ex) let x = 1, y = 4. y / 3 = 4 / 3 = 1.333 > 1, so x < y / 3. Return true.

    int factor = 3;
    auto helper = [&](int x, int y) { return x < (y / factor); };

    int j = 0;
    for (auto& v : target) {
        for (auto& i : v) {
            int y = j >= int(modifiers.size()) ? 0 : modifiers.at(j);
            i = helper(i, y) ? i : 0;
            j++;
        }
    }

    std::vector<std::vector<int>> expected = {{1, 2, 0}, {4, 5, 0}, {0, 8, 9}};

    if (Test::differs(target[0], expected[0]) || Test::differs(target[1], expected[1]) || Test::differs(target[2], expected[2]))
        throw Test::error("The vector does not match the expected result.");
});

// A0T2: Problem 3
// TODO: Vectors are variable length arrays of C++. What could possibly be going
//       wrong with this simple code? Fix the code while using iterators to 
//       retrieve the correct last element of the vector.
// Hint: Befriend C++ documentation websites

Test test_a0_task2_problems_vector("a0.task2.problems.vector", []() {
    std::vector<int> one_to_ten;

    for (int i = 0; i < 10; i++) {
        one_to_ten.emplace_back(i + 1);
    }

    // Use iterator to grab the last element of the vector
    int last_element = *one_to_ten.end();

    // The last element is surely a 10... right?
    int expected = 10;

    if (last_element != expected) {
		printf("Difference: %d", expected - last_element);
		throw Test::error("The last element was not 10.");
	}
});

// A0T2: Problem 4
// TODO: We want to count how many times a number appears in all three vectors.
//       Find the reason why the function below reports a count too high and fix it.

Test test_a0_task2_problems_boolean("a0.task2.problems.boolean", []() {
    std::vector<int> vec1, vec2, vec3;

    for (int i = 0; i < 20; i = i+1) vec1.emplace_back(i);
    for (int i = 0; i < 20; i = i+2) vec2.emplace_back(i);
    for (int i = 0; i < 20; i = i+3) vec3.emplace_back(i);

    int count = 0;
    for (size_t i = 0; i < vec1.size(); i++) {
        for (size_t j = 0; j < vec2.size(); j++) {
            for (size_t k = 0; k < vec3.size(); k++) {
                // Check if the numbers at indices i,j,k respectively are the same
                if ((vec1.at(i) == vec2.at(j)) == vec3.at(k)) count++;
            }
        }
    }

    int expected = 4; // 0, 6, 12, 18

    if (count != expected) {
        printf("Expected : %d, Got : %d", expected, count);
        throw Test::error("Wrong number of triple occurences was found.");
    }
});
