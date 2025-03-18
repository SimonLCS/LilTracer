#include "ggx.h"

namespace LT_NAMESPACE {

/////////////////////
// SphereMicrosurface
///////////////////

Float SphereMicrosurface::lambda(const vec3& wi_u)
{
    Float cos_sqr = glm::clamp(wi_u.z * wi_u.z, 0.0001f, 0.9999f);
    Float tan_sqr = (1. - cos_sqr) / cos_sqr;
    return (-1. + std::sqrt(1. + tan_sqr)) / 2.;
}

Float SphereMicrosurface::G1(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si) {
    return 1. / (1. + lambda(wi_u));
}

Float SphereMicrosurface::G2(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u, const SurfaceInteraction& si) {
    // [Ross05]
    return 1. / (1. + lambda(wi_u) + lambda(wo_u));
}

Float SphereMicrosurface::D(const vec3& wh_u, const SurfaceInteraction& si) {
    return 1. / pi; 
}

Float SphereMicrosurface::pdf(const vec3& wh_u, const SurfaceInteraction& si)
{
    return square_to_cosine_hemisphere_pdf(wh_u);
}
vec3 SphereMicrosurface::sample_D(Sampler& sampler, const SurfaceInteraction& si)
{
    return square_to_cosine_hemisphere(sampler.next_float(), sampler.next_float());
}

Float SphereMicrosurface::D(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si)
{
    return G1(wh_u, wi_u, si) * glm::clamp(glm::dot(wh_u, wi_u), 0.f, 1.f) / (wi_u.z * pi);
}

Float SphereMicrosurface::pdf(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si)
{
    return D(wh_u, wi_u, si);
}

// Sampling method from Sampling Visible GGX Normals with Spherical Caps, Jonathan Dupuy, Anis Benyoub
vec3 SphereMicrosurface::sample_D(const vec3& wi_u, Sampler& sampler, const SurfaceInteraction& si)
{
    float phi = 2. * pi * sampler.next_float();
    float z = std::fma(1. - sampler.next_float(), 1 + wi_u.z, -wi_u.z);
    float sin_theta = std::sqrt(std::clamp(1. - z * z, 0., 1.));
    float x = sin_theta * std::cos(phi);
    float y = sin_theta * std::sin(phi);
    return glm::normalize(wi_u + vec3(x, y, z));
}


} // namespace LT_NAMESPACE