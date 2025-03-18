
#include "beckmann.h"

namespace LT_NAMESPACE {

/////////////////////
// BeckmannMicrosurface
///////////////////

Float BeckmannMicrosurface::D(const vec3& wh_u, const SurfaceInteraction& si) {
    Float cos_theta_h_sqr = wh_u.z * wh_u.z;
    Float tan_theta_h_sqr = (1 - cos_theta_h_sqr) / cos_theta_h_sqr;
    return std::exp(-tan_theta_h_sqr) / (cos_theta_h_sqr* cos_theta_h_sqr * pi);
}

Float BeckmannMicrosurface::pdf(const vec3& wh_u, const SurfaceInteraction& si)
{
    return D(wh_u, si) * wh_u.z;
}

vec3 BeckmannMicrosurface::sample_D(Sampler& sampler, const SurfaceInteraction& si)
{
    Float log_sample = std::log(1 - sampler.next_float());
    if (std::isinf(log_sample)) log_sample = 0;
    Float tan_theta_sqr = -log_sample;
    Float phi = sampler.next_float() * 2 * pi;
    Float cos_theta = 1 / std::sqrt(1 + tan_theta_sqr);
    Float sin_theta = std::sqrt(std::max((Float)0, 1 - cos_theta * cos_theta));
    vec3 wh_u = vec3(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta);
    return wh_u;
}

Float BeckmannMicrosurface::lambda(const vec3& wi_u)
{
    Float tan_theta_i_sqr = (1 - wi_u.z * wi_u.z) / (wi_u.z * wi_u.z);
    Float abs_tan_theta = std::abs(std::sqrt(tan_theta_i_sqr));
    if (std::isinf(abs_tan_theta)) return 0.;

    Float a = 1 / (abs_tan_theta);
    if (a >= 1.6f)
        return 0;
    return (1 - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
}

Float BeckmannMicrosurface::G1(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si) {
    return 1. / (1. + lambda(wi_u));
}

Float BeckmannMicrosurface::G2(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u, const SurfaceInteraction& si) {
    // [Ross05]
    return 1. / (1. + lambda(wi_u) + lambda(wo_u));
}

Float BeckmannMicrosurface::D(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si)
{
    return D(wh_u, si);
}

Float BeckmannMicrosurface::pdf(const vec3& wh_u, const vec3& wi_u, const SurfaceInteraction& si)
{
    return pdf(wh_u, si);
}

vec3 BeckmannMicrosurface::sample_D(const vec3& wi_u, Sampler& sampler, const SurfaceInteraction& si)
{
    return sample_D(sampler, si);
}


} // namespace LT_NAMESPACE