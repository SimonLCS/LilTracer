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
    Spectrum albedo; /**< Albedo of the surface. */

    Diffuse(const Spectrum& albedo = Spectrum(0.5))
        : Brdf("Diffuse")
        , albedo(albedo)
    {
        flags = Flags::diffuse | Flags::reflection;
        link_params();
    }


    Spectrum eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler);
    Sample sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler);
    float pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si);

protected:
    void link_params() { params.add("albedo", ParamType::RGB, &albedo); }
};

} // namespace LT_NAMESPACE