#include <lt/geometry.h>

namespace LT_NAMESPACE {

template<>
Factory<Geometry>::CreatorRegistry& Factory<Geometry>::registry()
{
    static Factory<Geometry>::CreatorRegistry registry {
          { "Mesh", std::make_shared<Mesh> }
        , { "Rectangle", std::make_shared<Rectangle> }
        , { "Sphere", std::make_shared<Sphere> }
    };
    return registry;
}

} // namespace LT_NAMESPACE
