
#pragma once

#include <vector>

#include "../lib/spectrum.h"
#include "../platform/gl.h"

class HDR_Image {
public:
    HDR_Image();
    HDR_Image(size_t w, size_t h);
    HDR_Image(const HDR_Image& src) = delete;
    HDR_Image(HDR_Image&& src) = default;
    ~HDR_Image() = default;

    HDR_Image copy() const;

    HDR_Image& operator=(const HDR_Image& src) = delete;
    HDR_Image& operator=(HDR_Image&& src) = default;

    Spectrum& at(size_t x, size_t y);
    Spectrum at(size_t x, size_t y) const;
    Spectrum& at(size_t i);
    Spectrum at(size_t i) const;

    void clear(Spectrum color);
    void resize(size_t w, size_t h);
    std::pair<size_t, size_t> dimension() const;

    std::string load_from(std::string file);
    std::string loaded_from() const;

    void tonemap_to(std::vector<unsigned char>& data, float exposure = 0.0f) const;
    const GL::Tex2D& get_texture(float exposure = 0.0f) const;

private:
    void tonemap(float exposure = 0.0f) const;

    size_t w, h;
    std::string last_path;
    std::vector<Spectrum> pixels;

    mutable GL::Tex2D render_tex;
    mutable float exposure = 1.0f;
    mutable bool dirty = true;
};
