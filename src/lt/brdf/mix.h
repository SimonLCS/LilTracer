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

    Spectrum eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler) {
        return weight * brdf1->eval(wi, wo, si, sampler) + (1.f - weight) * brdf2->eval(wi, wo, si, sampler);
    }


    Sample sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler)
    {
        Sample bs;
        if (sampler.next_float() < weight) {
            bs = brdf1->sample(wi, si, sampler);
        }
        else {
            bs = brdf2->sample(wi, si, sampler);
        }
        return bs;
    }

    Float pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si) {
        return weight * brdf1->pdf(wi, wo, si) + (1.f - weight) * brdf2->pdf(wi, wo, si);
    }

    std::shared_ptr<Brdf> brdf1;
    std::shared_ptr<Brdf> brdf2;
    Float weight;

protected:
    void link_params() 
    { 
        params.add("brdf1", &brdf1);
        params.add("brdf2", &brdf2);
        params.add("weight", &weight);
    }
};


} // namespace LT_NAMESPACE