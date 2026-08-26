#include <memory> 
#include <iostream>
#include <string>
#include "lib/mathlib.h"

/**
 * 
 * Write a function that inserts an element anywhere in the singly linked list
 * given that place_index is from 0 to len(list) 
 * 
 * each node contains a unique integer 
 * 
 * e.g. nodes with vals:
 * 0 - 1 - 2 - 3 - 4
 * 
 * if adding node at index 2 and value 10, new layout is 
 * 0 - 1 - 10 - 2 - 3 - 4
 *        /^/ 
 * 
 * if adding node with value 10 at len(list) (5 in this case), 
 * new layout is 
 * 0 - 1 - 2 - 3 - 4 - 10
 *                    /^/
 * 
 * if adding node at 0, new layout is 
 *  10 - 0 - 1 - 2 - 3 - 4 
 * /^/
 * 
 * if the list is empty (start is NULL), then regardless of place_index, 
 * return a new node
 */

struct Node {
    Node *next;
    uint32_t val;
};

Node *insert_in_list(Node *start, uint32_t new_val, uint32_t place_index) {

    Node *out = new Node;
    return out;
    
}

/**
 * Write a new function that deletes a node at place_index
 * return a nullptr if deletion is not possible, else return the starting node
 * 
 * We will not give you empty lists, so start is *never* nullptr.
 */
Node *delete_from_list(Node *start, uint32_t delete_index) {
    Node *out = new Node;
    return out;
}

