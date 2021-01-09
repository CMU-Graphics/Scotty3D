
#include "debug.h"
#include <imgui/imgui.h>

#include "../lib/log.h"
#include "../lib/spectrum.h"

// Actual storage for the debug data
Debug_Data debug_data;

/* Debugging Tips:

    Based on your Debug_Data fields in debug.h, you can add ImGui calls
    to this function to make them editable in the debug UI panel.
    The UI panel may be shown using the Edit > Edit Debug Data menu option
    or by pressing Ctrl+D.

    ImGui is an immediate-mode GUI library, which means UI control flow
    is expressed just like normal code. For example, to create a button,
    all you have to do is:

        if(Button("My Button")) {
            // This runs when the button is clicked
        }

    Similarly, you can directly connect UI elements to data values by
    passing in the address of your storage variable:

        Checkbox("My Checkbox", &bool_variable);

    Then, bool_variable will always reflect the state of the checkbox.

    These constructs are composable to make pretty advanced UI elements!
    The whole Scotty3D UI is implemented in this way.

    Some useful functions are documented below, and you can refer to
    deps/imgui/imgui.h for many more.
*/
void student_debug_ui() {
    using namespace ImGui;

    // Debug option example
    Checkbox("Pathtracer: use normal colors", &debug_data.normal_colors);

    // ImGui examples
    if(Button("Press Me")) {
        info("Debug button pressed!");
    }

    // We need to store values somewhere, or else they will get reset every time
    // we run this function (which is every frame). For convenience, we make them
    // static, which gives them the same storage class as global variables.

    static int int_value = 0;
    InputInt("Int Input", &int_value);

    static float float_value = 0.0f;
    InputFloat("Float Input", &float_value);

    static Spectrum color = Spectrum(1.0f);
    ColorEdit3("Color Input", color.data);
}
