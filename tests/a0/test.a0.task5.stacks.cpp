#include "test.h"

#include "lib/mathlib.h"

#include <iostream>
#include <vector>

struct Object {
    int index;
    int vertices;
    int center;

    Mat4 transformation;

    std::vector<Object *> child_objs;
};

Object *find_node(Object *root, int goal_index);

// ---- local helpers ----
static Object make_obj(int index) {
    Object o;
    o.index = index;
    o.vertices = index * 10;
    o.center = 0;
    o.transformation = Mat4::I;
    return o;
}

static void check_found(Object *got, Object *expected, const char* why) {
    if (got == nullptr) {
        printf("Expected index : %d, Got : nullptr\n", expected->index);
        throw Test::error(why);
    }
    if (got->index != expected->index) {
        printf("Expected index : %d, Got : %d\n", expected->index, got->index);
        throw Test::error(why);
    }
    if (got != expected) {
        printf("Expected pointer to the node in the graph, got a different address.\n");
        throw Test::error("Returned an object with the right index, but not the node from the graph. "
                          "Did you return a copy or a freshly allocated Object?");
    }
}

static std::vector<int> snapshot(Object *root) {
    std::vector<int> out;
    std::vector<Object *> stack = {root};
    while (!stack.empty() && out.size() < 128) {
        Object *curr = stack.back();
        stack.pop_back();
        out.emplace_back(curr->index);
        out.emplace_back(int(curr->child_objs.size()));
        for (Object *child : curr->child_objs) stack.emplace_back(child);
    }
    return out;
}

//          0
//        /   \
//       1     2
//
Test test_a0_basiccpp_stacks_find_common_shallow("a0.task5.basiccpp.stacks.find.common.shallow", []() {
    Object root = make_obj(0), a = make_obj(1), b = make_obj(2);
    root.child_objs = {&a, &b};

    check_found(find_node(&root, 2), &b, "Failed to find a direct child of the root.");
});

//          0
//        /   \
//       1     2
//      /     / \
//     3     4   5
//                \
//                 6
//
Test test_a0_basiccpp_stacks_find_common_deep("a0.task5.basiccpp.stacks.find.common.deep", []() {
    Object root = make_obj(0), n1 = make_obj(1), n2 = make_obj(2), n3 = make_obj(3),
           n4 = make_obj(4), n5 = make_obj(5), n6 = make_obj(6);

    n5.child_objs = {&n6};
    n2.child_objs = {&n4, &n5};
    n1.child_objs = {&n3};
    root.child_objs = {&n1, &n2};

    std::vector<int> before = snapshot(&root);

    check_found(find_node(&root, 6), &n6, "Failed to find a deeply nested node on a non-first branch.");

    std::vector<int> after = snapshot(&root);
    if (Test::differs(before, after)) {
        throw Test::error("The traversal modified the scene graph. It should be left unchanged.");
    }
});
