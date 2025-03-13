#pragma once
#include <lt/lt_common.h>
#include <lt/surface_interaction.h>

namespace LT_NAMESPACE {

template <typename DATA_TYPE>
struct Texture {
    size_t w;
    size_t h;
    std::shared_ptr<DATA_TYPE[]> data;
    DATA_TYPE mean;

    Texture()
        : data(nullptr)
        , w(1)
        , h(1)
    {
        initialize();
    }
    Texture(const size_t& w, const size_t& h)
        : data(nullptr)
        , w(w)
        , h(h)
    {
        initialize();
    }

    Texture(const DATA_TYPE& value) 
        : data(nullptr)
        , w(1)
        , h(1) 
    {
        initialize();
        data[0] = value;
        mean = value;
    }

    ~Texture() { 
    }

    void initialize()
    {
        data.reset();
        data = std::make_shared<DATA_TYPE[]>(w * h);
    }

    void set(const size_t& x, const size_t& y, const DATA_TYPE& s)
    {
        data[y * w + x] = s;
    }

    void update_mean() {
        mean = DATA_TYPE(0);
        for (int i = 0; i < w * h; i++)
            mean += data[i];
        mean /= DATA_TYPE(w * h);
    }

    DATA_TYPE get(const size_t& x, const size_t& y) { return data[y * w + x]; }

    DATA_TYPE eval(const SurfaceInteraction& si)
    {
        size_t x = std::max(std::min(size_t(si.uv.x * (Float)w), w - 1), (size_t)0);
        size_t y = std::max(std::min(size_t(si.uv.y * (Float)h), h - 1), (size_t)0);
        return get(x, y);
    }

    DATA_TYPE eval(const Float& u, const Float& v)
    {
        size_t x = std::max(std::min(size_t(u * (Float)w), w - 1), (size_t)0);
        size_t y = std::max(std::min(size_t(v * (Float)h), h - 1), (size_t)0);
        return get(x, y);
    }
};


typedef Texture<Float> FloatTex;
using SpectrumTex = Texture<Spectrum>;


struct DustTexture : Texture<Float>
{
    DustTexture(const Float& v) { mean = v; }

    void set(const size_t& x, const size_t& y, const Float& s){}
    void update_mean() {}

    Float get(const size_t& x, const size_t& y) { return mean; }

    Float eval(const SurfaceInteraction& si)
    {
        return glm::clamp(si.nor.z,0.f,1.f);
    }

    Float eval(const Float& u, const Float& v)
    {
        return mean;
    }
};


} // namespace LT_NAMESPACE