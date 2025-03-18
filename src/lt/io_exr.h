#pragma once
#include <lt/lt_common.h>
#include <lt/sensor.h>
#include <lt/texture.h>
#include <tiny_exr/tinyexr.h>
#include <stb_image/stb_image.h>

#include <filesystem>


namespace LT_NAMESPACE {

enum class TextureExt {
    JPG,
    PNG,
    EXR,
    NOT_SUPPORTED
};

static std::unordered_map<std::string, TextureExt> const string_to_texture_ext =
{ 
    {".jpg",TextureExt::JPG}
   ,{".png",TextureExt::PNG}
   ,{".exr",TextureExt::EXR}
};

static std::unordered_map<TextureExt, std::string> const texture_ext_to_string =
{
    {TextureExt::JPG, ".jpg"}
   ,{TextureExt::PNG, ".png"}
   ,{TextureExt::EXR, ".exr"}
};

static std::string extension(const std::string& path) {
    return std::filesystem::path(path).extension().string();
}

static TextureExt get_texture_extension(const std::string& path) {
    std::string ext = extension(path);
    auto it = string_to_texture_ext.find(ext);
    if (it != string_to_texture_ext.end()) {
        return it->second;
    }
    else {
        return TextureExt::NOT_SUPPORTED;
    }
}


static int load_texture_ldr(const std::string& filename, std::shared_ptr<SpectrumTex>& t) {
    int width;
    int height;
    int channels;
    
    int expected_num_of_channel = 3;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* img = stbi_load( filename.c_str(), &width, &height, &channels, expected_num_of_channel);
    if (img == NULL) {
        Log(logError) << "Load PNG err";
        return -1;
    }

    t->w = (size_t)width;
    t->h = (size_t)height;
    t->initialize();

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int i = expected_num_of_channel * (y * width + x);
            float r = float(img[i]) / 255.f;
            float g = float(img[i+1]) / 255.f;
            float b = float(img[i+2]) / 255.f;
            t->set(x, y, glm::pow(vec3(r, g, b),vec3(2.2)) );
        }
    }
    t->update_mean();

    stbi_image_free(img);

    return 0;
}

static int load_texture_exr(const std::string& filename, std::shared_ptr<SpectrumTex>& t)
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

    t->w = (size_t)width;
    t->h = (size_t)height;
    t->initialize();

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int i = 4 * (y * width + x);
            t->set(x, y, Spectrum(out[i], out[i + 1], out[i + 2]));
        }
    }
    t->update_mean();

    delete[] out;

    return TINYEXR_SUCCESS;
};


static int load_texture(const std::string& path, std::shared_ptr<FloatTex>& t) {
    if (true) {
        t = std::make_shared<DustTex>(0.5);
        return 0;
    }
    return -1;
}



static int load_texture(const std::string& path, std::shared_ptr<SpectrumTex>& t) {

    switch (get_texture_extension(path))
    {
    case lt::TextureExt::JPG:
    case lt::TextureExt::PNG:
        return load_texture_ldr(path, t);
        break;
    case lt::TextureExt::EXR:
        return load_texture_exr(path, t);
        break;
    case lt::TextureExt::NOT_SUPPORTED:
    default:
        Log(logError) << "load_texture err : " << path << " extension not supported";
        return 0;
        break;
    }

    return -1;
}


static int save_sensor_exr(const Sensor& sen, const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::path p(filename);
    fs::path d = p.parent_path();

    bool exist = fs::exists(d);
    bool is_dir = fs::is_directory(d);

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




} // namespace LT_NAMESPACE