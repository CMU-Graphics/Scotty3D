
#include "hdr_image.h"
#include "../lib/log.h"

#include <sf_libs/stb_image.h>
#include <sf_libs/tinyexr.h>

HDR_Image::HDR_Image() : w(0), h(0) {
}

HDR_Image::HDR_Image(size_t w, size_t h) : w(w), h(h) {
    assert(w > 0 && h > 0);
    pixels.resize(w * h);
}

HDR_Image HDR_Image::copy() const {
    HDR_Image ret;
    ret.resize(w, h);
    ret.pixels.insert(ret.pixels.begin(), pixels.begin(), pixels.end());
    ret.last_path = last_path;
    ret.dirty = true;
    ret.exposure = exposure;
    return ret;
}

std::pair<size_t, size_t> HDR_Image::dimension() const {
    return {w, h};
}

void HDR_Image::resize(size_t _w, size_t _h) {
    w = _w;
    h = _h;
    pixels.clear();
    pixels.resize(w * h);
    dirty = true;
}

void HDR_Image::clear(Spectrum color) {
    for(auto& s : pixels) s = color;
    dirty = true;
}

Spectrum& HDR_Image::at(size_t i) {
    assert(i < w * h);
    dirty = true;
    return pixels[i];
}

Spectrum HDR_Image::at(size_t i) const {
    assert(i < w * h);
    return pixels[i];
}

Spectrum& HDR_Image::at(size_t x, size_t y) {
    assert(x < w && y < h);
    size_t idx = y * w + x;
    dirty = true;
    return pixels[idx];
}

Spectrum HDR_Image::at(size_t x, size_t y) const {
    assert(x < w && y < h);
    size_t idx = y * w + x;
    return pixels[idx];
}

std::string HDR_Image::load_from(std::string file) {

    if(IsEXR(file.c_str()) == TINYEXR_SUCCESS) {

        int n_w, n_h;
        float* data;
        const char* err = nullptr;

        int ret = LoadEXR(&data, &n_w, &n_h, file.c_str(), &err);

        if(ret != TINYEXR_SUCCESS) {

            if(err) {
                std::string err_s(err);
                FreeEXRErrorMessage(err);
                return err_s;
            } else
                return "Unknown failure.";

        } else {

            resize(n_w, n_h);

            for(size_t j = 0; j < h; j++) {
                for(size_t i = 0; i < w; i++) {
                    size_t didx = 4 * (j * w + i);
                    size_t pidx = (h - j - 1) * w + i;
                    pixels[pidx] = Spectrum(data[didx], data[didx + 1], data[didx + 2]);
                    if(!pixels[pidx].valid()) pixels[pidx] = {};
                }
            }

            free(data);
        }

    } else {

        stbi_set_flip_vertically_on_load(true);

        int n_w, n_h, channels;
        unsigned char* data = stbi_load(file.c_str(), &n_w, &n_h, &channels, 0);

        if(!data) return std::string(stbi_failure_reason());
        if(channels < 3) return "Image has less than 3 color channels.";

        resize(n_w, n_h);

        for(size_t i = 0; i < w * h * channels; i += channels) {
            float r = data[i] / 255.0f;
            float g = data[i + 1] / 255.0f;
            float b = data[i + 2] / 255.0f;
            pixels[i / channels] = Spectrum(r, g, b);
        }

        stbi_image_free(data);

        for(size_t i = 0; i < pixels.size(); i++) {
            pixels[i].make_linear();
            if(!pixels[i].valid()) pixels[i] = {};
        }
    }

    last_path = file;
    dirty = true;
    return {};
}

std::string HDR_Image::loaded_from() const {
    return last_path;
}

void HDR_Image::tonemap(float e) const {

    if(e <= 0.0f) {
        e = exposure;
    } else if(e != exposure) {
        exposure = e;
        dirty = true;
    }

    if(!dirty) return;

    std::vector<unsigned char> data;
    tonemap_to(data, e);
    render_tex.image((int)w, (int)h, data.data());

    dirty = false;
}

const GL::Tex2D& HDR_Image::get_texture(float e) const {
    tonemap(e);
    return render_tex;
}

void HDR_Image::tonemap_to(std::vector<unsigned char>& data, float e) const {

    if(e <= 0.0f) {
        e = exposure;
    }

    if(data.size() != w * h * 4) data.resize(w * h * 4);

    for(size_t j = 0; j < h; j++) {
        for(size_t i = 0; i < w; i++) {

            size_t pidx = (h - j - 1) * w + i;
            const Spectrum& sample = pixels[pidx];

            float r = 1.0f - std::exp(-sample.r * exposure);
            float g = 1.0f - std::exp(-sample.g * exposure);
            float b = 1.0f - std::exp(-sample.b * exposure);

            Spectrum out(r, g, b);
            out.make_srgb();

            size_t didx = 4 * (j * w + i);
            data[didx] = (unsigned char)std::round(out.r * 255.0f);
            data[didx + 1] = (unsigned char)std::round(out.g * 255.0f);
            data[didx + 2] = (unsigned char)std::round(out.b * 255.0f);
            data[didx + 3] = 255;
        }
    }
}
