#pragma once

#include <math.h>
#include <stdint.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>

#define LT_NAMESPACE lt

namespace LT_NAMESPACE {

using vec3 = glm::vec3;
using vec2 = glm::vec2;

using Spectrum = vec3;

using Float = float;

const Float pi = 3.14159265359;

enum LogType {
      logNoLabel
    , logDebug
    , logInfo
    , logHighlight
    , logSuccess
    , logWarning
    , logError
};

struct State {
    static std::string exectuable_path;
    static LogType log_level; 
};

class Log
{
private:
    bool show;
    std::stringstream s;
public:
    Log() : show(false) {}
    Log(LogType type) : show(false) {
        show = type >= State::log_level;
        switch (type) {
        case logDebug:
            s << "\033[34m[LT - Debug] ";
            break;
        case logInfo:
            s << "\033[37m[LT - Info] ";
            break;
        case logHighlight:
            s << "\033[97;1m[LT - Info] ";
            break;
        case logSuccess:
            s << "\033[32m[LT - Success] ";
            break;
        case logWarning:
            s << "\033[35m[LT - Warning] ";
            break;
        case logError:
            s << "\033[31m[LT - Error] ";
            break;
        case logNoLabel:
            break;
        }
    }


    ~Log() {
        if (show)
            std::cout << s.str() << "\033[0m" << std::endl;
    }

    template<class T>
    Log& operator<<(const T& msg) {
        s << msg;
        return *this;
    }


};


template <typename T>
inline std::vector<T> linspace(T start, T end, size_t size, bool centered = true)
{
    std::vector<T> arr;
    arr.resize(size);
    if (size == 0)
        return arr;
    if (size == 1)
        return { start };

    //T delta = (end - start) / (size - 1);
    if (centered) {
        T delta = (end - start) / size;
        for (size_t i = 0; i < size; i++)
            arr[i] = start + delta * i + delta * 0.5;
    }
    else {
        T delta = (end - start) / (size - 1);
        for (size_t i = 0; i < size; i++)
            arr[i] = start + delta * i;
    }
    return arr;
}

inline vec3 polar_to_card(Float theta, Float phi)
{
    return vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

inline vec3 square_to_uniform_sphere(Float u1, Float u2)
{
    Float z = 1. - 2. * u1;
    Float r = std::sqrt(std::max(0., 1. - z * z));
    Float ph = 2. * pi * u2;
    return vec3(r * cos(ph), r * sin(ph), z);
}

inline Float square_to_uniform_sphere_pdf() { return 1. / (4. * pi); }

inline vec3 square_to_uniform_hemisphere(Float u1, Float u2)
{
    Float z = u1;
    Float r = std::sqrt(std::max(0., 1. - z * z));
    Float ph = 2. * pi * u2;
    return vec3(r * cos(ph), r * sin(ph), z);
}

inline Float square_to_uniform_hemisphere_pdf() { return 1. / (2. * pi); }

inline vec3 square_to_cosine_hemisphere(Float u1, Float u2)
{
#if 0
    Float cos_theta = std::sqrt(u1);
    Float sin_theta = std::sqrt(glm::clamp(1.f - cos_theta * cos_theta,0.f,0.99999f));
    Float phi = 2. * pi * u2;
    Float x = sin_theta * std::cos(phi);
    Float y = sin_theta * std::sin(phi);
    return vec3(x, y, cos_theta);
#endif
#if 1
    Float r = std::sqrt(u1);
    Float theta = 2. * pi * u2;
    Float dx = r * std::cos(theta);
    Float dy = r * std::sin(theta);
    Float z = std::sqrt(glm::clamp(1.f - dx * dx - dy * dy,0.00001f,1.f));
    if (z != z)
        Log(logError) << "square_to_cosine_hemisphere : invalid sample generated";
    return vec3(dx, dy, z);
#endif
}

inline Float square_to_cosine_hemisphere_pdf(const vec3& w)
{
    return glm::clamp(w.z,0.f,1.f) / pi;
}

inline void orthonormal_basis(const vec3& n, vec3& t, vec3& b)
{
    if (n.z < -0.999999) {
        t = vec3(0, -1, 0);
        b = vec3(-1, 0, 0);
    } else {
        float c1 = 1. / (1. + n.z);
        float c2 = -n.x * n.y * c1;
        t = glm::normalize(vec3(1.f - n.x * n.x * c1, c2, -n.x));
        b = glm::normalize(vec3(c2, 1.f - n.y * n.y * c1, -n.y));
    }
}

inline glm::mat3 build_tbn_from_w(const vec3& w)
{
    vec3 normal = w;
    float sign = copysignf(1.0f, normal.z);
    const float a = -1.0f / (sign + normal.z);
    const float b = normal.x * normal.y * a;
    vec3 tangent = vec3(1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
    vec3 bitangent = vec3(b, sign + normal.y * normal.y * a, -normal.y);
    return glm::mat3(tangent, bitangent, normal);
}

inline Spectrum fresnelConductor(const Float& cosThetaI, const Spectrum& eta, const Spectrum& k) {
    /* Modified from "Optics" by K.D. Moeller, University Science Books, 1988 */

    Float cosThetaI2 = cosThetaI * cosThetaI,
        sinThetaI2 = 1 - cosThetaI2,
        sinThetaI4 = sinThetaI2 * sinThetaI2;

    Spectrum temp1 = eta * eta - k * k - sinThetaI2;
    Spectrum a2pb2 = glm::sqrt(glm::max(temp1 * temp1 + 4.f * k * k * eta * eta, 0.00001f));
    Spectrum a = glm::sqrt(glm::max(0.5f * (a2pb2 + temp1), 0.00001f));

    Spectrum term1 = a2pb2 + cosThetaI2;
    Spectrum term2 = 2.f * a * cosThetaI;

    Spectrum Rs2 = (term1 - term2) / (term1 + term2);

    Spectrum term3 = a2pb2 * cosThetaI2 + sinThetaI4;
    Spectrum term4 = term2 * sinThetaI2;

    Spectrum Rp2 = Rs2 * (term3 - term4) / (term3 + term4);

    return 0.5f * (Rp2 + Rs2);
}

//template <class T>
//int binary_search(const T* arr, const T& val, const int& size) {
//    // edge case: value of smaller than min or larger than max
//    if (arr[0] >= val) return 0;
//    if (arr[size - 1] <= val) return size - 1;
//
//    int start = 0;
//    int end = size - 1;
//
//    //while (start <= end) {
//    while (end - start >= 1) {
//        int mid = (end + start) / 2;
//
//        // value is in interval from previous to current element
//        if (val >= arr[mid] && val <= arr[mid+1]) {
//            return mid ;
//        }
//        else {
//            if (arr[mid] < val) {
//                start = mid;
//            }
//            else {
//                end = mid;
//            }
//        }
//    }
//    return -1;
//}

template <class T>
int binary_search(const T* arr, const T& val, const int& size) {
    int first = 0, len = size;
    while (len > 0) {
        int half = len >> 1, middle = first + half;
        // Bisect range based on value of _pred_ at _middle_
        if (arr[middle] <= val) {
            first = middle + 1;
            len -= half + 1;
        }
        else
            len = half;
    }
    return glm::clamp(first - 1, 0, size - 2);
}

template <class T>
int binary_search(const std::vector<T>& arr, const T& val) {
    return binary_search<T>(arr.data(), val, arr.size());
}




inline double igf(double S, double Z)
{
    if (Z < 0.0)
    {
        return 0.0;
    }
    double Sc = (1.0 / S);
    Sc *= pow(Z, S);
    Sc *= exp(-Z);

    double Sum = 1.0;
    double Nom = 1.0;
    double Denom = 1.0;

    for (int I = 0; I < 200; I++)
    {
        Nom *= Z;
        S++;
        Denom *= S;
        Sum += (Nom / Denom);
    }

    return Sum * Sc;
}


inline double approx_gamma(double Z)
{
    const double RECIP_E = 0.36787944117144232159552377016147;  // RECIP_E = (E^-1) = (1.0 / E)
    const double TWOPI = 6.283185307179586476925286766559;  // TWOPI = 2.0 * PI

    double D = 1.0 / (10.0 * Z);
    D = 1.0 / ((12 * Z) - D);
    D = (D + Z) * RECIP_E;
    D = pow(D, Z);
    D *= sqrt(TWOPI / Z);

    return D;
}


inline double chisqr(int Dof, double Cv)
{
    if (Cv < 0 || Dof < 1)
    {
        return 0.0;
    }
    double K = ((double)Dof) * 0.5;
    double X = Cv * 0.5;
    if (Dof == 2)
    {
        return exp(-1.0 * X);
    }

    double PValue = igf(K, X);
    if (isnan(PValue) || isinf(PValue) || PValue <= 1e-8)
    {
        return 1e-14;
    }

    PValue /= tgamma(K); 

    return (1.0 - PValue);
}


class Bbox {
public:
    vec3 pmin;
    vec3 pmax;

    Bbox() : pmin(0), pmax(0){}

    Bbox(vec3 p) {
        pmin = p;
        pmax = p;
    }

    void grow(const Bbox& b) {
        pmin = glm::min(pmin, b.pmin);
        pmax = glm::max(pmax, b.pmax);
    }

    void grow(const vec3& p) {
        pmin = glm::min(pmin, p);
        pmax = glm::max(pmax, p);
    }
};


} // namespace LT_NAMESPACE
