
#pragma once

#include "shape_invariant.h"

namespace LT_NAMESPACE {

class BeckmannMicrosurface {
public:
    Float D(const vec3& wh_u);
    Float D(const vec3& wh_u, const vec3& wi_u);

    Float pdf(const vec3& wh_u);
    Float pdf(const vec3& wh_u, const vec3& wi_u);

    vec3 sample_D(Sampler& sampler);
    vec3 sample_D(const vec3& wi_u, Sampler& sampler);

    Float lambda(const vec3& wi_u);
    Float G1(const vec3& wh_u, const vec3& wi_u);
    Float G2(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u);
};


class RoughBeckmann : public RoughShapeInvariantMicrosurface<BeckmannMicrosurface> {
public:
    RoughBeckmann()
        : RoughShapeInvariantMicrosurface<BeckmannMicrosurface>("RoughBeckmann", 0.1, 0.1)
    {
        link_params();
    }

    RoughBeckmann(const Float& scale_x, const Float& scale_y)
        : RoughShapeInvariantMicrosurface<BeckmannMicrosurface>("RoughBeckmann", scale_x, scale_y)
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
    }
};



} // namespace LT_NAMESPACE