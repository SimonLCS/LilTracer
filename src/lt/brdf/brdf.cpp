#include "brdf.h"

namespace LT_NAMESPACE {


/////////////////////
// Base 
///////////////////
Spectrum Brdf::eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler)
{
    return Spectrum(0.); 
};

Brdf::Sample Brdf::sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler)
{ 
    Sample bs;
    bs.wo = square_to_cosine_hemisphere(sampler.next_float(), sampler.next_float());
    bs.value = eval(wi, bs.wo, si, sampler) / pdf(wi, bs.wo, si);
    return bs;
}

float Brdf::pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si)
{
    return square_to_cosine_hemisphere_pdf(wo); 
}

Spectrum Brdf::emission() 
{
    return Spectrum(0.); 
}



} // namespace LT_NAMESPACE