
#pragma once

/* Debugging Tips:

    You may use this file, as well as debug.cpp, to add any debugging features and
    UI options that you find useful. To do so, you can add fields to the global
    Debug_Data type here and access them in any other student/ files via debug_data.field
    You can use your fields to enable/disable features or otherwise change how your
    implementation behaves in the other files.

    You can also connect your debug fields to specific UI options by adding
    ImGui calls in debug.cpp. This creates a special UI panel that can be enabled
    by the Edit -> Edit Debug Data menu item or by pressing Ctrl+D.
    This panel will contain your specific debug controls.

    For example, we have already implemented an option to color pathtracer objects
    based on their surface normal. This involved adding the following field to
    Debug_Data:

        struct Debug_Data {
            bool normal_colors = false;
        };

    This ImGui option to student_debug_ui in debug.cpp:

        void student_debug_ui() {

            // ...
            Checkbox("Use Normal Colors", &debugger.normal_colors);
            // ...
        }

    And we finally used the option in pathtracer.cpp:

        Spectrum Pathtracer::trace_ray(const Ray& ray) {

            // ...
            Spectrum radiance_out = debug_data.normal_colors ? Spectrum(0.5f) :
    Spectrum::direction(hit.normal);
            // ...
        }
*/

struct Debug_Data {
    // Setting it here makes it default to false.
    bool normal_colors = false;
};

// This tells other code about a global variable of type Debug_Data, allowing
// us to access it in any code that includes this header.
extern Debug_Data debug_data;

void student_debug_ui();
