
#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "../lib/mathlib.h"
#include "../scene/scene.h"
#include "../util/hdr_image.h"
#include "../util/thread_pool.h"

#include "bsdf.h"
#include "env_light.h"
#include "light.h"
#include "object.h"

namespace Gui {
class Widget_Render;
}

namespace PT {

class Pathtracer {
public:
    Pathtracer(Gui::Widget_Render& gui, Vec2 screen_dim);
    ~Pathtracer();

    void set_sizes(size_t w, size_t h, size_t pixel_samples, size_t area_samples, size_t depth);

    const HDR_Image& get_output();
    const GL::Tex2D& get_output_texture(float exposure);
    size_t visualize_bvh(GL::Lines& lines, GL::Lines& active, size_t level);

    void begin_render(Scene& scene, const Camera& camera, bool add_samples = false);
    void cancel();
    bool in_progress() const;
    float progress() const;
    std::pair<float, float> completion_time() const;

private:
    // Internal
    void build_scene(Scene& scene);
    void build_lights(Scene& scene, std::vector<Object>& objs);
    void do_trace(size_t samples);
    void accumulate(const HDR_Image& sample);
    bool tonemap();

    Gui::Widget_Render& gui;
    unsigned long long render_time, build_time;
    Thread_Pool thread_pool;
    bool cancel_flag = false;

    HDR_Image accumulator;
    std::mutex accumulator_mut;
    size_t total_epochs, accumulator_samples;
    std::atomic<size_t> completed_epochs;

    /// Relevant to student
    Spectrum trace_pixel(size_t x, size_t y);
    Spectrum trace_ray(const Ray& ray);
    void log_ray(const Ray& ray, float t, Spectrum color = Spectrum{1.0f});

    BVH<Object> scene;
    std::vector<Light> lights;
    std::vector<BSDF> materials;
    std::optional<Env_Light> env_light; // only one of these per scene
    std::unordered_map<Scene_ID, size_t> mat_cache;

    Camera camera;
    size_t out_w, out_h, n_samples, n_area_samples, max_depth;
};

} // namespace PT
