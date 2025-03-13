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

    /*
    Texture(const Texture& src) :
        w(src.w),
        h(src.h),
        mean(src.mean)
    {
        initialize();

        for (int i = 0; i < src.w * src.h; ++i) {
            data[i] = src.data[i];
        }
    }

    Texture(Texture&& o) :
        w(std::move(o.w)),      
        h(std::move(o.h)),      
        mean(std::move(o.mean))
    {
        if (data){
            delete[] data;
            data = nullptr;
        }
        data = o.data;
    }
    
    Texture& operator=(const Texture& rhs) {
        if (&rhs != this) {
            Texture temp(rhs); // copies the array
            temp.swap(*this);
        }
        return *this;
    }

    Texture& operator=(Texture&& rhs) {
        Texture temp(std::move(rhs));
        temp.swap(*this);
        return *this;
    }
    void swap(Texture& other) {
        // swap the array pointers...
        std::swap(data, other.data);
        std::swap(w, other.w);
        std::swap(h, other.h);
        std::swap(mean, other.mean);
    }
    */

    ~Texture() { 
        /*if (data) {
            delete[] data;
            data = nullptr;
        }*/
    }

    void initialize()
    {
        //if (data) {
        //    delete[] data;
        //    data = nullptr;
        //}
        // 
        //data = new DATA_TYPE[w * h]();
        
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


} // namespace LT_NAMESPACE