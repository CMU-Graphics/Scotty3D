#include "test.h"

#include <iostream>
#include <vector>

struct Point {
    int x;
    int y;
};

struct Triangle {
    Point v1, v2, v3;
    int depth;
};

void sort_prims(std::vector<Triangle> &prims);

// ---- local helpers ----

static Triangle make_tri(int depth) {
    return Triangle{Point{0, 0}, Point{1, 0}, Point{0, 1}, depth};
}

static std::vector<Triangle> make_prims(const std::vector<int>& depths) {
    std::vector<Triangle> prims;
    for (int d : depths) prims.emplace_back(make_tri(d));
    return prims;
}

static std::vector<int> depths_of(const std::vector<Triangle>& prims) {
    std::vector<int> out;
    for (const Triangle& t : prims) out.emplace_back(t.depth);
    return out;
}

static void print_depths(const char* label, const std::vector<int>& v) {
    printf("%s: [", label);
    for (size_t i = 0; i < v.size(); i++) {
        printf("%d%s", v[i], i + 1 == v.size() ? "" : ", ");
    }
    printf("]\n");
}

Test test_a0_iterators_sort_common_unsorted("a0.task5.iterators.sort.common.unsorted", []() {
    std::vector<Triangle> prims = make_prims({5, 3, 9, 1, 7});

    sort_prims(prims);

    std::vector<int> got = depths_of(prims);
    std::vector<int> expected = {1, 3, 5, 7, 9};

    if (Test::differs(got, expected)) {
        print_depths("Expected", expected);
        print_depths("Got     ", got);
        throw Test::error("Primitives were not sorted by ascending depth.");
    }
});

Test test_a0_iterators_sort_common_sorted("a0.task5.iterators.sort.common.sorted", []() {
    std::vector<Triangle> prims = make_prims({-2, 0, 4, 11, 42});

    sort_prims(prims);

    std::vector<int> got = depths_of(prims);
    std::vector<int> expected = {-2, 0, 4, 11, 42};

    if (Test::differs(got, expected)) {
        print_depths("Expected", expected);
        print_depths("Got     ", got);
        throw Test::error("An already sorted list was modified. Check your comparator's direction.");
    }
});
