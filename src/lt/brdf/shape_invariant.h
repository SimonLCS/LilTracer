
#pragma once

#include "brdf.h"

namespace LT_NAMESPACE {

template <class MICROSURFACE>
class ShapeInvariantMicrosurface : public Brdf {
public:
    vec3 scale;

    ShapeInvariantMicrosurface(const std::string& type, const Float& scale_x,
        const Float& scale_y)
        : Brdf(type)
    {
        scale = vec3(scale_x, scale_y, 1.);
        sample_visible_distribution = false;
    }

    vec3 to_unit_space(const vec3& wi);
    vec3 to_transformed_space(const vec3& wi);
    
    Float G1(const vec3& wh, const vec3& wi);
    Float G2(const vec3& wh, const vec3& wi, const vec3& wo);

    Float D(const vec3& wh);
    Float pdf_wh(const vec3& wh);
    
    Float D(const vec3& wh, const vec3& wi);
    Float pdf_wh(const vec3& wh, const vec3& wi);
    

    vec3 sample_D(Sampler& sampler);
    vec3 sample_D(const vec3& wi, Sampler& sampler);
    
    //virtual Sample sample(const vec3& wi, Sampler& sampler) = 0;
    //virtual Spectrum eval(vec3 wi, vec3 wo) = 0;
    //virtual Float pdf(const vec3& wi, const vec3& wo) = 0;

    MICROSURFACE ms;
    bool sample_visible_distribution;
    bool optimize;
};


/////////////////////
// ShapeInvariantMicrosurface<MICROSURFACE>
///////////////////
template <class MICROSURFACE>
vec3 ShapeInvariantMicrosurface<MICROSURFACE>::to_unit_space(const vec3& wi)
{
    return glm::normalize(wi * scale);
}

template <class MICROSURFACE>
vec3 ShapeInvariantMicrosurface<MICROSURFACE>::to_transformed_space(const vec3& wi)
{
    return glm::normalize(wi / scale);
}

template <class MICROSURFACE>
Float ShapeInvariantMicrosurface<MICROSURFACE>::G1(const vec3& wh, const vec3& wi)
{
    return ms.G1(to_transformed_space(wh), to_unit_space(wi));
}

template <class MICROSURFACE>
Float ShapeInvariantMicrosurface<MICROSURFACE>::G2(const vec3& wh, const vec3& wi, const vec3& wo)
{
    //return G1(wh, wi) * G1(wh, wo);
    return ms.G2(to_transformed_space(wh), to_unit_space(wi), to_unit_space(wo));
}

template <class MICROSURFACE>
Float ShapeInvariantMicrosurface<MICROSURFACE>::D(const vec3& wh)
{
    Float det_m = 1. / std::abs(scale.x * scale.y);
    vec3 wh_u = to_transformed_space(wh);
    return ms.D(wh_u) * det_m * std::pow(wh_u.z / wh.z, 4.);
}

template <class MICROSURFACE>
Float ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(const vec3& wh)
{
    Float det_m = 1. / std::abs(scale.x * scale.y);
    vec3 wh_u = to_transformed_space(wh);
    return ms.pdf(wh_u) * det_m * std::pow(wh_u.z / wh.z, 3.);
}

template <class MICROSURFACE>
Float ShapeInvariantMicrosurface<MICROSURFACE>::D(const vec3& wh, const vec3& wi)
{
    Float det_m = 1. / std::abs(scale.x * scale.y);
    vec3 wh_u = to_transformed_space(wh);
    vec3 wi_u = to_unit_space(wi);
    return ms.D(wh_u, wi_u) * det_m * std::pow(wh_u.z / wh.z, 3.);
}

template <class MICROSURFACE>
Float ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(const vec3& wh, const vec3& wi)
{
    Float det_m = 1. / std::abs(scale.x * scale.y);
    vec3 wh_u = to_transformed_space(wh);
    vec3 wi_u = to_unit_space(wi);
    return ms.pdf(wh_u, wi_u) * det_m * std::pow(wh_u.z / wh.z, 3.);
}

template <class MICROSURFACE>
vec3 ShapeInvariantMicrosurface<MICROSURFACE>::sample_D(Sampler& sampler)
{
    return to_unit_space(ms.sample_D(sampler));
}

template <class MICROSURFACE>
vec3 ShapeInvariantMicrosurface<MICROSURFACE>::sample_D(const vec3& wi, Sampler& sampler)
{
    return to_unit_space(ms.sample_D(to_unit_space(wi), sampler));
}


/////////////////////
// RoughShapeInvariantMicrosurface<MICROSURFACE>
///////////////////

template <class MICROSURFACE>
class RoughShapeInvariantMicrosurface : public ShapeInvariantMicrosurface<MICROSURFACE> {
public:
    RoughShapeInvariantMicrosurface(const std::string& type, const Float& scale_x, const Float& scale_y)
        : ShapeInvariantMicrosurface<MICROSURFACE>(type, scale_x, scale_y)
    {
        Brdf::flags = Brdf::Flags::rough | Brdf::Flags::reflection;
        eta = Spectrum(1.);
        kappa = Spectrum(10000.);
    }

    Spectrum eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler);
    Brdf::Sample sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler);
    Float pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si);

    // Return eval / pdf
    Spectrum eval_optim(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler);

    Spectrum eta;
    Spectrum kappa;
};


template <class MICROSURFACE>
Spectrum RoughShapeInvariantMicrosurface<MICROSURFACE>::eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler)
{
    vec3 wh = glm::normalize(wi + wo);
    Float d = ShapeInvariantMicrosurface<MICROSURFACE>::D(wh);
    Float g = ShapeInvariantMicrosurface<MICROSURFACE>::G2(wh, wi, wo);
    Spectrum f = fresnelConductor(glm::dot(wh, wi), eta, kappa);
    Spectrum brdf = d * g * f / (4.f * glm::clamp(wi[2], 0.0001f, 0.9999f));
    return brdf;
}

template <class MICROSURFACE>
Spectrum RoughShapeInvariantMicrosurface<MICROSURFACE>::eval_optim(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler)
{
    /*if (ShapeInvariantMicrosurface<MICROSURFACE>::optimize) {
        return ShapeInvariantMicrosurface<MICROSURFACE>::sample_visible_distribution
            ? eval(wi, wo, sampler) / pdf(wi, wo)
            : eval(wi, wo, sampler) / pdf(wi, wo);
    }*/
    return eval(wi, wo, si, sampler) / pdf(wi, wo, si);
}


template <class MICROSURFACE>
Brdf::Sample RoughShapeInvariantMicrosurface<MICROSURFACE>::sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler)
{
    Brdf::Sample bs;

    vec3 wh = ShapeInvariantMicrosurface<MICROSURFACE>::sample_visible_distribution
        ? ShapeInvariantMicrosurface<MICROSURFACE>::sample_D(wi, sampler)
        : ShapeInvariantMicrosurface<MICROSURFACE>::sample_D(sampler);

    bs.wo = glm::reflect(-wi, wh);
    //bs.wo = wh;


    bs.value = eval_optim(wi, bs.wo, si, sampler);

    //Float g = sample_visible_distribution ? G2(wh,wi, bs.wo)  / G1(wh,bs.wo) : G1(wh, wi) * G1(wh, bs.wo) * glm::dot(wi, wh) / (glm::clamp(wi[2], 0.0001f, 0.9999f) * glm::clamp(wh[2], 0.0001f, 0.9999f));
    //bs.value = F * e;

    return bs;
}

template <class MICROSURFACE>
Float RoughShapeInvariantMicrosurface<MICROSURFACE>::pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si)
{

    //Float pdf_wh_ = ShapeInvariantMicrosurface<MICROSURFACE>::sample_visible_distribution
    //    ? ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(wo, wi)
    //    : ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(wo);

    //return pdf_wh_;

    vec3 wh = glm::normalize(wi + wo);

    Float pdf_wh_ = ShapeInvariantMicrosurface<MICROSURFACE>::sample_visible_distribution
        ? ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(wh, wi)
        : ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(wh);

    return pdf_wh_ / (4. * glm::clamp(glm::dot(wh, wi), 0.0001f, 0.9999f));
}

/////////////////////
// DiffuseShapeInvariantMicrosurface<MICROSURFACE>
///////////////////

template <class MICROSURFACE>
class DiffuseShapeInvariantMicrosurface : public ShapeInvariantMicrosurface<MICROSURFACE> {
public:
    DiffuseShapeInvariantMicrosurface(const std::string& type, const Float& scale_x, const Float& scale_y)
        : ShapeInvariantMicrosurface<MICROSURFACE>(type, scale_x, scale_y)
    {
        Brdf::flags = Brdf::Flags::diffuse | Brdf::Flags::reflection;
        albedo = Spectrum(0.5);
    }

    Spectrum eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler);
    Brdf::Sample sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler);
    Float pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si);

    Spectrum albedo;
};


template <class MICROSURFACE>
Spectrum DiffuseShapeInvariantMicrosurface<MICROSURFACE>::eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler)
{
    vec3 wh = ShapeInvariantMicrosurface<MICROSURFACE>::sample_visible_distribution
        ? ShapeInvariantMicrosurface<MICROSURFACE>::sample_D(wi, sampler)
        : ShapeInvariantMicrosurface<MICROSURFACE>::sample_D(sampler);

    Float pdf_wh_ = ShapeInvariantMicrosurface<MICROSURFACE>::sample_visible_distribution
        ? ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(wh, wi)
        : ShapeInvariantMicrosurface<MICROSURFACE>::pdf_wh(wh);

    Float d = ShapeInvariantMicrosurface<MICROSURFACE>::D(wh);
    Float g = ShapeInvariantMicrosurface<MICROSURFACE>::G2(wh, wi, wo);

    Float i_dot_m = glm::clamp(glm::dot(wi, wh), 0.00001f, 0.99999f);
    Float o_dot_m = glm::clamp(glm::dot(wo, wh), 0.00001f, 0.99999f);
    Float cos_theta_i = glm::clamp(wi[2], 0.00001f, 0.99999f);

    Float brdf = i_dot_m * o_dot_m * d * g / pdf_wh_;
    return albedo * brdf / cos_theta_i / pi;
}


template <class MICROSURFACE>
Brdf::Sample DiffuseShapeInvariantMicrosurface<MICROSURFACE>::sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler)
{
    Brdf::Sample bs;

    bs.wo = square_to_cosine_hemisphere(sampler.next_float(), sampler.next_float());
    bs.value = eval(wi, bs.wo, si, sampler) / ShapeInvariantMicrosurface<MICROSURFACE>::pdf(wi, bs.wo, si);

    return bs;
}

template <class MICROSURFACE>
Float DiffuseShapeInvariantMicrosurface<MICROSURFACE>::pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si)
{
    return square_to_cosine_hemisphere_pdf(wo);
}


} // namespace LT_NAMESPACE