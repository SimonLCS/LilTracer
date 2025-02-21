#pragma once
#include <lt/lt_common.h>
#include <lt/surface_interaction.h>

namespace LT_NAMESPACE {

template <typename DATA_TYPE>
struct Texture {
    size_t w;
    size_t h;
    DATA_TYPE* data;

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
    }

    ~Texture() { delete[] data; }

    void initialize()
    {
        delete[] data;
        data = new DATA_TYPE[w * h]();
    }

    void set(const size_t& x, const size_t& y, const DATA_TYPE& s)
    {
        data[y * w + x] = s;
    }

    DATA_TYPE get(const size_t& x, const size_t& y) { return data[y * w + x]; }

    DATA_TYPE eval(const SurfaceInteraction& si)
    {
        size_t x = std::max(std::min(size_t(si.u * (Float)w), w - 1), (size_t)0);
        size_t y = std::max(std::min(size_t(si.v * (Float)h), h - 1), (size_t)0);
        return get(x, y);
    }

    DATA_TYPE eval(const Float& u, const Float& v)
    {
        size_t x = std::max(std::min(size_t(u * (Float)w), w - 1), (size_t)0);
        size_t y = std::max(std::min(size_t(v * (Float)h), h - 1), (size_t)0);
        return get(x, y);
    }
};


typedef Texture<Float> Float_tex;
typedef Texture<vec3> vec3_tex;


} // namespace LT_NAMESPACE