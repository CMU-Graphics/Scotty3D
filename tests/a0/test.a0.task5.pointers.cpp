#include "test.h"

#include <cstdint>
#include <iostream>
#include <vector>

struct Node {
    Node *next;
    uint32_t val;
};

Node *insert_in_list(Node *start, uint32_t new_val, uint32_t place_index);
Node *delete_from_list(Node *start, uint32_t delete_index);

// ---- local helpers ----
static Node *build_list(const std::vector<int>& vals) {
    Node *head = nullptr;
    Node *tail = nullptr;
    for (int v : vals) {
        Node *n = new Node();
        n->val = uint32_t(v);
        n->next = nullptr;
        if (tail == nullptr) head = n;
        else tail->next = n;
        tail = n;
    }
    return head;
}

static bool has_no_loop(Node *head) {
    Node *slow = head;
    Node *fast = head;

    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;       // moves 1 step
        fast = fast->next->next; // moves 2 steps

        if (slow == fast) return false;
    }
    return true;
}

static std::vector<int> to_vector(Node *start, size_t limit = 64) {
    std::vector<int> out;
    for (Node *n = start; n != nullptr && out.size() < limit; n = n->next) {
        out.emplace_back(int(n->val));
    }
    return out;
}

static void free_list(Node *start, size_t limit = 64) {
    size_t freed = 0;
    while (start != nullptr && freed < limit) {
        Node *next = start->next;
        delete start;
        start = next;
        freed++;
    }
}

static void print_list(const char* label, const std::vector<int>& v) {
    printf("%s: [", label);
    for (size_t i = 0; i < v.size(); i++) {
        printf("%d%s", v[i], i + 1 == v.size() ? "" : " - ");
    }
    printf("]\n");
}

// ========================= insert_in_list() =========================

// insert into the middle of the list. 0-1-2-3-4, value 10 at index 2, gives
// 0-1-10-2-3-4.
Test test_a0_task5_pointers_insert_common_middle("a0.task5.pointers.insert.common.middle", []() {
    Node *head = build_list({0, 1, 2, 3, 4});

    head = insert_in_list(head, 10, 2);

    bool looped = !has_no_loop(head);
    std::vector<int> got = to_vector(head);
    if (!looped) free_list(head);

    if (looped) {
        throw Test::error("The resulting list has a loop.");
    }

    std::vector<int> expected = {0, 1, 10, 2, 3, 4};
    if (Test::differs(got, expected)) {
        print_list("Expected", expected);
        print_list("Got     ", got);
        throw Test::error("Inserting into the middle of the list produced the wrong layout.");
    }
});

// insert at index 0
Test test_a0_task5_pointers_insert_common_front("a0.task5.pointers.insert.common.front", []() {
    Node *head = build_list({0, 1, 2, 3, 4});

    head = insert_in_list(head, 10, 0);

    bool looped = !has_no_loop(head);
    std::vector<int> got = to_vector(head);
    if (!looped) free_list(head);

    if (looped) {
        throw Test::error("The resulting list has a loop.");
    }

    std::vector<int> expected = {10, 0, 1, 2, 3, 4};
    if (Test::differs(got, expected)) {
        print_list("Expected", expected);
        print_list("Got     ", got);
        throw Test::error("Inserting at index 0 did not return the new head.");
    }
});

// ======================== delete_from_list() ========================

// delete from the middle
// 0-1-2-3-4 without index 2 is 0-1-3-4.
Test test_a0_basiccpp_pointers_delete_common_middle("a0.basiccpp.pointers.delete.common.middle", []() {
    Node *head = delete_from_list(build_list({0, 1, 2, 3, 4}), 2);
 
    std::vector<int> got = to_vector(head);
    free_list(head);
 
    std::vector<int> expected = {0, 1, 3, 4};
    if (Test::differs(got, expected)) {
        print_list("Expected", expected);
        print_list("Got     ", got);
        throw Test::error("Deleting from the middle produced the wrong layout.");
    }
});

// delete front
Test test_a0_basiccpp_pointers_delete_common_front("a0.basiccpp.pointers.delete.common.front", []() {
    Node *head = delete_from_list(build_list({0, 1, 2, 3, 4}), 0);
 
    std::vector<int> got = to_vector(head);
    free_list(head);
 
    std::vector<int> expected = {1, 2, 3, 4};
    if (Test::differs(got, expected)) {
        print_list("Expected", expected);
        print_list("Got     ", got);
        throw Test::error("Deleting index 0 did not return the new head.");
    }
});
