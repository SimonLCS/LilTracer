#pragma once

#include "shape_invariant.h"
#include "lambert.h"

namespace LT_NAMESPACE {

    class MicrograinMicrosurface {
    public:
        MicrograinMicrosurface() 
            : tau_0(0.1)
            , use_smith(false)
            , sig_asia_2023(false)
            , height_and_direction(true)  {}

        Float D(const vec3& wh_u);
        Float D(const vec3& wh_u, const vec3& wi_u);

        Float pdf(const vec3& wh_u);
        Float pdf(const vec3& wh_u, const vec3& wi_u);

        vec3 sample_D(Sampler& sampler);
        vec3 sample_D(const vec3& wi_u, Sampler& sampler);

        Float G1(const vec3& wh_u, const vec3& wi_u);
        Float G1_0(const vec3& wi_u);
        Float G2(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u);
        Float G2_0(const vec3& wi_u, const vec3& wo_u);

        Float tau_v(const vec3& wi_u);

        Float sigma_base(const vec3& wh_u);
        Float sigma_shadow(const vec3& wh_u, const vec3& wi_u);
        Float sigma_shadow_0(const vec3& wi_u);
        
        Float sigma_shadow_inter(const vec3& wh_u, const vec3& wi_u, const vec3& wo_u, const vec2& p, const vec2& pi, const vec2& po);
        Float sigma_shadow_inter_0(const vec3& wi_u, const vec3& wo_u, const vec2& p, const vec2& pi, const vec2& po);
        
        Float sigma(const vec3& wi_u);
        
        Float lambda(const vec3& wi_u);

        // Eq 24. Siggraph Asia 2023
        Float w_plus(const vec3& wi_u, const vec3& wo_u);

        Float one_to_many(const Float& sigma_);

        Float tau_0;
        bool use_smith;
        bool height_and_direction;
        bool sig_asia_2023;
    };




    class RoughMicrograin : public RoughShapeInvariantMicrosurface<MicrograinMicrosurface> {
    public:

        RoughMicrograin(const Float& scale_x = 0.1, const Float& scale_y = 0.1)
            : RoughShapeInvariantMicrosurface<MicrograinMicrosurface>("RoughMicrograin", scale_x, scale_y)
        {
            base = std::make_shared<Diffuse>(Spectrum(0.5));
            link_params();
        }

        Spectrum eval(vec3 wi, vec3 wo, Sampler& sampler) {
            Spectrum surf_brdf = RoughShapeInvariantMicrosurface<MicrograinMicrosurface>::eval(wi, wo, sampler);
            Spectrum base_brdf = base->eval(wi, wo, sampler);
            
            if (ms.sig_asia_2023) {
                Float wei = ms.w_plus(to_unit_space(wi), to_unit_space(wo));
                return wei * surf_brdf + (1.f - wei) * base_brdf;
            }

            Float visibility = ms.G2_0(to_unit_space(wi), to_unit_space(wo));
            return ms.tau_0 * surf_brdf + (1.f - ms.tau_0) * base_brdf * visibility;
        }

        Brdf::Sample sample(const vec3& wi, Sampler& sampler)
        {
            Brdf::Sample bs;

            Float porosity = 1 - ms.tau_v(to_unit_space(wi));
            // Eq 26 Siggraph 2024
            Float base_weight = porosity / (ms.tau_0 + porosity);
            
            if (sampler.next_float() < base_weight) {
                bs = base->sample(wi, sampler);

                Float visibility = ms.G2_0(to_unit_space(wi), to_unit_space(bs.wo));
                assert(visibility == visibility);

                bs.value *= (1.f - ms.tau_0) * visibility / base_weight;
            }
            else {
                bs = RoughShapeInvariantMicrosurface<MicrograinMicrosurface>::sample(wi, sampler);
                bs.value *= ms.tau_0 / (1-base_weight);
            }

            return bs;
        }

        Float pdf(const vec3& wi, const vec3& wo)
        {

            Float porosity = 1 - ms.tau_v(to_unit_space(wi));
            // Eq 26 Siggraph 2024
            Float base_weight = porosity / (ms.tau_0 + porosity);
           
            return (1- base_weight) * RoughShapeInvariantMicrosurface<MicrograinMicrosurface>::pdf(wi,wo) + base_weight * base->pdf(wi,wo);
        }
        
        std::shared_ptr<Brdf> base;

    protected:
        void link_params()
        {
            params.add("rough_x", Params::Type::FLOAT, &scale[0]);
            params.add("rough_y", Params::Type::FLOAT, &scale[1]);
            params.add("tau", Params::Type::FLOAT, &(ms.tau_0));
            params.add("eta", Params::Type::IOR, &eta);
            params.add("kappa", Params::Type::IOR, &kappa);
            params.add("height_and_direction", Params::Type::BOOL, &(ms.height_and_direction));
            params.add("use_smith", Params::Type::BOOL, &(ms.use_smith));
            params.add("sig_asia_2023", Params::Type::BOOL, &(ms.sig_asia_2023));
            params.add("base", Params::Type::BRDF, &base);
        }
        
    };



    class DiffuseMicrograin : public DiffuseShapeInvariantMicrosurface<MicrograinMicrosurface> {
    public:

        DiffuseMicrograin(const Float& scale_x = 0.1, const Float& scale_y = 0.1)
            : DiffuseShapeInvariantMicrosurface<MicrograinMicrosurface>("DiffuseMicrograin", scale_x, scale_y)
        {
            base = std::make_shared<Diffuse>(Spectrum(0.5));
            link_params();
        }

        Spectrum eval(vec3 wi, vec3 wo, Sampler & sampler) {
            Spectrum surf_brdf = DiffuseShapeInvariantMicrosurface<MicrograinMicrosurface>::eval(wi, wo, sampler);
            Spectrum base_brdf = base->eval(wi, wo, sampler);

            Float visibility = ms.G2_0(to_unit_space(wi), to_unit_space(wo));
            return ms.tau_0 * surf_brdf + (1.f - ms.tau_0) * base_brdf * visibility;
        }

        Brdf::Sample sample(const vec3 & wi, Sampler & sampler)
        {
            Brdf::Sample bs;

            Float porosity = 1 - ms.tau_v(to_unit_space(wi));
            // Eq 26 Siggraph 2024
            Float base_weight = porosity / (ms.tau_0 + porosity);

            if (sampler.next_float() < base_weight) {
                bs = base->sample(wi, sampler);
                Float visibility = ms.G2_0(to_unit_space(wi), to_unit_space(bs.wo));
                bs.value *= (1.f - ms.tau_0) * visibility / base_weight;
            }
            else {
                bs = DiffuseShapeInvariantMicrosurface<MicrograinMicrosurface>::sample(wi, sampler);
                bs.value *= ms.tau_0 / (1 - base_weight);
            }

            return bs;
        }

        Float pdf(const vec3 & wi, const vec3 & wo)
        {
            Float porosity = 1 - ms.tau_v(to_unit_space(wi));
            // Eq 26 Siggraph 2024
            Float base_weight = porosity / (ms.tau_0 + porosity);

            return (1 - base_weight) * DiffuseShapeInvariantMicrosurface<MicrograinMicrosurface>::pdf(wi, wo) + base_weight * base->pdf(wi, wo);
        }

        std::shared_ptr<Brdf> base;

    protected:
        void link_params()
        {
            params.add("rough_x", Params::Type::FLOAT, &scale[0]);
            params.add("rough_y", Params::Type::FLOAT, &scale[1]);
            params.add("tau", Params::Type::FLOAT, &(ms.tau_0));
            params.add("albedo", Params::Type::VEC3, &albedo);
            params.add("use_smith", Params::Type::BOOL, &(ms.use_smith));
            params.add("base", Params::Type::BRDF, &base);
        }
    };


} // namespace LT_NAMESPACE

