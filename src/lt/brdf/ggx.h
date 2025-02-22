
#pragma once

#include "shape_invariant.h"

namespace LT_NAMESPACE {

class SphereMicrosurface {
public:
    Float D(const vec3& wh_u);
    Float D(const vec3& wh_u, const vec3& wi_u);

    Float pdf(const vec3& wh_u);
    Float pdf(const vec3& wh_u, const vec3& wi_u);

    vec3 sample_D(Sampler& sampler);
    // Sampling method from Sampling Visible GGX Normals with Spherical Caps, Jonathan Dupuy, Anis Benyoub
    vec3 sample_D(const vec3& wi_u, Sampler& sampler);

    Float lambda(const vec3& wi_u);
    Float G1(const vec3& wh_u, const vec3& wi_u);
    Float G2(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u);
};


class RoughGGX : public RoughShapeInvariantMicrosurface<SphereMicrosurface> {
public:
    RoughGGX()
        : RoughShapeInvariantMicrosurface<SphereMicrosurface>("RoughGGX", 0.1, 0.1)
    {
        link_params();
    }

    RoughGGX(const Float& scale_x, const Float& scale_y)
        : RoughShapeInvariantMicrosurface<SphereMicrosurface>("RoughGGX", scale_x, scale_y)
    {
        link_params();
    }
    
protected:
    void link_params()
    {
        params.add("rough_x", ParamType::FLOAT, &scale[0]);
        params.add("rough_y", ParamType::FLOAT, &scale[1]);
        params.add("eta", ParamType::IOR, &eta);
        params.add("kappa", ParamType::IOR, &kappa);
        params.add("sample_visible_distribution", ParamType::BOOL, &sample_visible_distribution);
    }
};





class DiffuseGGX : public DiffuseShapeInvariantMicrosurface<SphereMicrosurface> {
public:
    DiffuseGGX()
        : DiffuseShapeInvariantMicrosurface<SphereMicrosurface>("DiffuseGGX", 0.1, 0.1)
    {
        link_params();
    }

    DiffuseGGX(const Float& scale_x, const Float& scale_y)
        : DiffuseShapeInvariantMicrosurface<SphereMicrosurface>("DiffuseGGX", scale_x, scale_y)
    {
        link_params();
    }

protected:
    void link_params()
    {
        params.add("rough_x", ParamType::FLOAT, &scale[0]);
        params.add("rough_y", ParamType::FLOAT, &scale[1]);
        params.add("albedo", ParamType::RGB, &albedo);
        params.add("sample_visible_distribution", ParamType::BOOL, &sample_visible_distribution);
    }
};




} // namespace LT_NAMESPACE