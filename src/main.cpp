
#include "platform/platform.h"
#include "util/rand.h"
#include <sf_libs/CLI11.hpp>

int main(int argc, char** argv) {

    RNG::seed();

    App::Settings settings;
    CLI::App args{"Scotty3D - 15-462"};

    args.add_option("-s,--scene", settings.scene_file, "Scene file to load");
    args.add_option("--env_map", settings.env_map_file, "Override scene environment map");
    args.add_flag("--headless", settings.headless, "Path-trace scene without opening the GUI");
    args.add_option("-o,--output", settings.output_file, "Image file to write (if headless)");
    args.add_flag("--animate", settings.animate, "Output animation frames (if headless)");
    args.add_option("--width", settings.w, "Output image width (if headless)");
    args.add_option("--height", settings.h, "Output image height (if headless)");
    args.add_flag("--use_ar", settings.w_from_ar,
                  "Compute output image width based on camera AR (if headless)");
    args.add_option("--depth", settings.d, "Maximum ray depth (if headless)");
    args.add_option("--samples", settings.s, "Pixel samples (if headless)");
    args.add_option("--exposure", settings.exp, "Output exposure (if headless)");
    args.add_option("--area_samples", settings.ls, "Area light samples (if headless)");

    CLI11_PARSE(args, argc, argv);

    if(!settings.headless) {
        Platform plt;
        App app(settings, &plt);
        plt.loop(app);
    } else {
        App app(settings);
    }
    return 0;
}
