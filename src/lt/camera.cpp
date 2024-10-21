#include <lt/camera.h>

namespace LT_NAMESPACE {

template<>
Factory<Camera>::CreatorRegistry& Factory<Camera>::registry()
{
    static Factory<Camera>::CreatorRegistry registry {
        { "PerspectiveCamera", std::make_shared<PerspectiveCamera> }
       ,{ "GonioCamera", std::make_shared<GonioCamera> }
    };
    return registry;
}


void PerspectiveCamera::init()
{
    view = glm::lookAt(pos, center, vec3(0., 1., 0.));
    inv_view = glm::inverse(view);
    proj = glm::perspective((double)fov * pi / 180.f, (double)aspect, 0.001, 100.);
    inv_proj = glm::inverse(proj);
}


Ray PerspectiveCamera::generate_ray(Float u, Float v)
{
    glm::vec4 d_eye = inv_proj * glm::vec4(u / aspect, v / aspect, -1, 1.);
    d_eye.w = 0.;

    vec3 d = glm::vec3(inv_view * d_eye);
    d = glm::normalize(d);

    return Ray(pos, d);
}




void GonioCamera::init()
{
    dir = -vec3(std::cos(phi) * std::sin(theta), std::cos(theta), std::sin(phi) * std::sin(theta));
}


Ray GonioCamera::generate_ray(Float u, Float v)
{
    vec3 pos = center + size * vec3(u, 0, v);
    return Ray(pos - dir * offset, dir);
}


} // namespace LT_NAMESPACE