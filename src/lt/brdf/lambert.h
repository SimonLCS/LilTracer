/**
 * @file brdf.h
 * @brief Defines classes related to Bidirectional Reflectance Distribution
 * Functions (BRDFs).
 */

#pragma once

#include "brdf.h"


namespace LT_NAMESPACE {

/**
 * @brief Diffuse BRDF class.
 */
class Diffuse : public Brdf {
public:
    std::shared_ptr<SpectrumTex> albedo; /**< Albedo of the surface. */

    Diffuse(std::shared_ptr<SpectrumTex> kd)
        : Brdf("Diffuse")
        , albedo(kd)
    {
        flags = Flags::diffuse | Flags::reflection;
        link_params();
    }

    Diffuse(const Spectrum& kd = Spectrum(0.5))
        : Brdf("Diffuse")
        , albedo(std::make_shared<SpectrumTex>(kd))
    {
        flags = Flags::diffuse | Flags::reflection;
        link_params();
    }


    Spectrum eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler);
    Sample sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler);
    float pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si);

protected:
    void link_params() { params.add("albedo", ParamType::SPECTRUM_TEX, &albedo); }
};

} // namespace LT_NAMESPACE