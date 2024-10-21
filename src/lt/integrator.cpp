#include <lt/integrator.h>

namespace LT_NAMESPACE {
    
template<>
Factory<Integrator>::CreatorRegistry& Factory<Integrator>::registry()
{
    static Factory<Integrator>::CreatorRegistry registry {
        { "BrdfIntegrator", std::make_shared<BrdfIntegrator> },
        { "PathIntegrator", std::make_shared<PathIntegrator> },
        { "DirectIntegrator", std::make_shared<DirectIntegrator> },
        { "GonioIntegrator", std::make_shared<GonioIntegrator> },
        { "AOIntegrator", std::make_shared<AOIntegrator> }
    };
    return registry;
}

} // namespace LT_NAMESPACE
