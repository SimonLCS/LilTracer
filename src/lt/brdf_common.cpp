#include "brdf_common.h"

namespace LT_NAMESPACE {

    int BrdfValidation::number_of_theta = 90;
    int BrdfValidation::number_of_sample = 10000;

    /////////////////////
    // Brdf Factory
    ///////////////////

    template<>
    Factory<Brdf>::CreatorRegistry& Factory<Brdf>::registry()
    {
        static Factory<Brdf>::CreatorRegistry registry{
            { "Emissive", std::make_shared<Emissive> },
            { "Diffuse", std::make_shared<Diffuse> },
            { "DiffuseGGX", std::make_shared<DiffuseGGX> },
            { "DiffuseMicrograin", std::make_shared<DiffuseMicrograin> },
            { "RoughGGX", std::make_shared<RoughGGX> },
            { "RoughGGXRetro", std::make_shared<RoughGGXRetro> },
            { "RoughBeckmann", std::make_shared<RoughBeckmann> },
            { "RoughMicrograin", std::make_shared<RoughMicrograin> },
            { "Mix", std::make_shared<Mix> },
            { "TestBrdf", std::make_shared<TestBrdf> }
        };
        return registry;
    }

} // namespace LT_NAMESPACE