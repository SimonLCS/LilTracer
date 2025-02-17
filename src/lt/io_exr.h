#pragma once
#include <lt/lt_common.h>
#include <lt/sensor.h>
#include <lt/texture.h>
#include <tiny_exr/tinyexr.h>

#include <filesystem>

namespace LT_NAMESPACE {

static int save_sensor_exr(const Sensor& sen, const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::path p(filename);
    fs::path d = p.parent_path();
    bool exist = fs::exists(d);
    std::cout << exist << std::endl;
    bool is_dir = fs::is_directory(d);
    std::cout << is_dir << std::endl;

    if (!exist || !is_dir) { // Check if src folder exists
        fs::create_directories(d); // create src folder
    }

    const char* err;
    int ret = SaveEXR((float*)sen.value.data(), sen.w, sen.h, 3, 0,
        filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        Log(logError) << "SaveEXR err : " << err;
        return ret;
    }

    Log(logHighlight) << "Saved exr file. [ " << filename << "]";

    return 0;
};

static int load_texture_exr(const std::string& filename, Texture<Spectrum>& t)
{
    float* out;
    int width;
    int height;
    const char* err;
    int ret = LoadEXR(&out, &width, &height, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        Log(logError) << "Load EXR err : " << err;
        return ret;
    }

    t.w = (size_t)width;
    t.h = (size_t)height;
    t.initialize();

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int i = 4 * (y * width + x);
            t.set(x, y, Spectrum(out[i], out[i + 1], out[i + 2]));
        }
    }

    delete[] out;

    return TINYEXR_SUCCESS;
};

} // namespace LT_NAMESPACE