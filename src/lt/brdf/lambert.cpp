#include "lambert.h"

namespace LT_NAMESPACE {

/////////////////////
// Diffuse 
///////////////////
Spectrum Diffuse::eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler)
{
    return albedo->eval(si) / pi * glm::clamp(wo[2], 0.f, 1.f);
}

Brdf::Sample Diffuse::sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler)
{
    Sample bs;
    bs.wo = square_to_cosine_hemisphere(sampler.next_float(), sampler.next_float());
    bs.value = albedo->eval(si);
    return bs;
}

float Diffuse::pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si)
{
    return square_to_cosine_hemisphere_pdf(wo);
}


} // namespace LT_NAMESPACE