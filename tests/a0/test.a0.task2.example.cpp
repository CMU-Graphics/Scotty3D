
#include "test.h"

Test test_a0_task2_example_always_fails("a0.task2.example.always_fails", []() {
	throw Test::error("This test always fails.");
});

Test test_a0_task2_example_always_passes("a0.task2.example.always_passes", []() {
	// Does not throw error, hence passes
});

Test test_a0_task2_example_always_ignored("a0.task2.example.always_ignored", []() {
	throw Test::ignored("This test is always ignored.");
});
