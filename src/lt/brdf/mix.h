/**
 * @file brdf.h
 * @brief Defines classes related to Bidirectional Reflectance Distribution
 * Functions (BRDFs).
 */

#pragma once

#include "brdf.h"
#include "lambert.h"

namespace LT_NAMESPACE {

class Mix : public Brdf {
public:
    PARAMETER(Spectrum, albedo, 0.5); /**< Albedo of the surface. */

    Mix() : Brdf("Mix")
    {
        link_params();
        brdf1 = std::make_shared<Diffuse>(Spectrum(0.1, 0.5, 0.9));
        brdf2 = std::make_shared<Diffuse>(Spectrum(0.9, 0.5, 0.1));
        weight = 0.5;
    }

    void init() {
        flags = brdf1->flags | brdf2->flags;
    }

    Spectrum eval(vec3 wi, vec3 wo, Sampler& sampler) {
        return weight * brdf1->eval(wi, wo, sampler) + (1.f - weight) * brdf2->eval(wi, wo, sampler);
    }


    Sample sample(const vec3& wi, Sampler& sampler) 
    {
        Sample bs;
        if (sampler.next_float() < weight) {
            bs = brdf1->sample(wi, sampler);
        }
        else {
            bs = brdf2->sample(wi, sampler);
        }
        return bs;
    }

    Float pdf(const vec3& wi, const vec3& wo) {
        return weight * brdf1->pdf(wi, wo) + (1.f - weight) * brdf2->pdf(wi, wo);
    }

    std::shared_ptr<Brdf> brdf1;
    std::shared_ptr<Brdf> brdf2;
    Float weight;

protected:
    void link_params() 
    { 
        params.add("brdf1", ParamType::BRDF, &brdf1);
        params.add("brdf2", ParamType::BRDF, &brdf2);
        params.add("weight", ParamType::FLOAT, &weight);
    }
};


} // namespace LT_NAMESPACE