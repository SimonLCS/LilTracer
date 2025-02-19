#include <lt/light.h>

namespace LT_NAMESPACE {

    template<>
    Factory<Light>::CreatorRegistry& Factory<Light>::registry()
    {
        static Factory<Light>::CreatorRegistry registry{
            { "DirectionnalLight", std::make_shared<DirectionnalLight> },
            { "SphereLight", std::make_shared<SphereLight> },
            { "EnvironmentLight", std::make_shared<EnvironmentLight> }
        };
        return registry;
    }

    void DirectionnalLight::init()
    {
        dir = glm::normalize(dir);
    }

    Light::Sample DirectionnalLight::sample(const SurfaceInteraction& si, Sampler& sampler)
    {
        Sample s;
        s.direction = dir;
        s.emission = Spectrum(intensity);
        s.pdf = 1;
        s.expected_distance_to_intersection = std::numeric_limits<Float>::infinity();
        return s;

    }

    Spectrum DirectionnalLight::eval(const vec3& direction)
    {
        return Spectrum(intensity);
    }


    Float DirectionnalLight::pdf(const vec3& p, const vec3& ld)
    {
        return 0.;
    }

    Float DirectionnalLight::power()
    {
        return intensity;
    }

    Float DirectionnalLight::distance(const vec3& p)
    {
        return 1;
    }

    void EnvironmentLight::init()
    {
        dtheta = pi / (Float)envmap.h;
        dphi = 2. * pi / (Float)envmap.w;

        compute_density();
        c = std::vector<float>(
            cumulative_density.data,
            cumulative_density.data + cumulative_density.w * cumulative_density.h);
        c.insert(c.begin(), 0.);

    }

    Light::Sample EnvironmentLight::sample(const SurfaceInteraction& si, Sampler& sampler)
    {
        Sample s;
#if 0
        s.direction =-square_to_uniform_sphere(sampler.next_float(), sampler.next_float());
        s.pdf = square_to_uniform_sphere_pdf();
        Float solid_angle = 1.;
#endif // 0
#if 1
        Float u = sampler.next_float();

        // Binary search
        //int id = std::distance(c.begin(), std::lower_bound(c.begin(), c.end(), u));
        //std::vector<float>::iterator begin = c.begin();
        //int id = std::lower_bound(begin, c.end(), u) - begin;
        // Binary search
        //int id = binary_search<Float>(c,u);
        // From inv_cumulative_density
        int id = inv_cumulative_density.data[int(u * inv_cumulative_density.h * inv_cumulative_density.w)];

        int x = id % envmap.w;
        int y = id / envmap.w;

        Float theta = pi * ((Float)y + 0.5) / (Float)envmap.h;
        Float phi = 2. * pi * ((Float)x + 0.5) / (Float)envmap.w;

        Float solid_angle = sin(theta) * dphi * dtheta;

        theta += dtheta * (sampler.next_float() - 0.5);
        phi += dphi * (sampler.next_float() - 0.5);

        s.direction = -vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

        s.pdf = density.get(x, y) / solid_angle;
        //s.pdf = pdf(vec3(0.), s.direction);
        //s.pdf = density.data[id];
#endif
        s.emission = eval(-s.direction);
        s.expected_distance_to_intersection = std::numeric_limits<Float>::infinity();
        
        return s;
    }

    /**
     * @brief 
     * 
     * @param direction Toward the envmap
     * @return Spectrum 
     */
    Spectrum EnvironmentLight::eval(const vec3& direction)
    {
        Float phi = glm::atan(direction.z, direction.x);
        phi = (phi < 0. ? 2 * pi + phi : phi);
        Float u = phi / (2 * pi);
        Float v = glm::acos(direction.y) / pi;
        return envmap.eval(u, v) * intensity;
    }

    /**
     * @brief 
     * 
     * @param p 
     * @param ld Toward the scene
     * @return Float 
     */
    Float EnvironmentLight::pdf(const vec3& p, const vec3& ld)
    {
        vec3 dir = -ld;
        Float phi = glm::atan(dir.z, dir.x);
        phi = (phi < 0. ? 2 * pi + phi : phi);
        Float u = phi / (2 * pi);
        Float v = glm::acos(dir.y) / pi;
        Float solid_angle = std::sqrt(glm::clamp(1.0f - dir.y * dir.y, 0.000001f, 1.0f)) * dphi * dtheta;
        return density.eval(u, v) / solid_angle;
    }

    Float EnvironmentLight::power()
    {
        return power_;
    }

    Float EnvironmentLight::distance(const vec3& p)
    {
        return 1;
    }

    void EnvironmentLight::compute_density()
    {
        // Compute density
        density.w = envmap.w;
        density.h = envmap.h;
        density.initialize();

        for (int y = 0; y < envmap.h; y++) {
            Float theta = pi * ((Float)y + 0.5) / (Float)envmap.h;
            Float sin_theta = std::sin(theta);
            Float solid_angle = sin_theta * dphi * dtheta;
            for (int x = 0; x < envmap.w; x++) {
                Spectrum s = envmap.get(x, y);
                Float mean = (s.r + s.g + s.b) * 0.333333f;
                density.set(x, y, mean * sin_theta);
            }
        }

        // Compute cumulative density
        cumulative_density.w = envmap.w;
        cumulative_density.h = envmap.h;
        cumulative_density.initialize();
        cumulative_density.data[0] = density.data[0];
        for (int n = 1; n < envmap.w * envmap.h; n++) {
            cumulative_density.data[n] = cumulative_density.data[n - 1] + density.data[n];
        }

        power_ = cumulative_density.data[envmap.w * envmap.h - 1] * intensity;

        // Normalize density
        for (int n = 0; n < envmap.w * envmap.h; n++) {
            density.data[n] /= cumulative_density.data[envmap.w * envmap.h - 1];
            cumulative_density.data[n] /= cumulative_density.data[envmap.w * envmap.h - 1];
        }

        inv_cumulative_density.w = envmap.w;
        inv_cumulative_density.h = envmap.h;
        inv_cumulative_density.initialize();

        std::vector<Float> u = linspace<Float>(0.,1., envmap.w * envmap.h, true);
        
        for (int n = 0; n < envmap.w * envmap.h; n++) {
            int idx = binary_search<Float>(cumulative_density.data, u[n], envmap.w* envmap.h);
            inv_cumulative_density.data[n] = idx;
        }

    }



    Light::Sample SphereLight::sample(const SurfaceInteraction& si, Sampler& sampler)
    {
        Sample s;
#if 0
        vec3 point = square_to_uniform_sphere(sampler.next_float(), sampler.next_float());
        vec3 point_on_surface = point * sphere->rad + sphere->pos;

        vec3 direction = si.pos - point_on_surface;
        Float distance = glm::length(direction);
        direction /= distance;

        Float light_cosine = glm::dot(si.nor, -direction);
        Float light_area = 4. * pi * sphere->rad * sphere->rad;

        s.direction = direction;
        s.pdf = distance * distance / (light_area * light_cosine);
        s.expected_distance_to_intersection = distance;
#endif

#if 1
        vec3 direction = sphere->pos - si.pos;
        Float distance = glm::length(direction);
        direction /= distance;

        Float dist_sqr = distance * distance;
        Float rad_sqr = sphere->rad * sphere->rad;

        Float cos_theta_max = std::sqrt(1. - rad_sqr / dist_sqr);
        Float solid_angle = 2. * pi * (1 - cos_theta_max);

        Float u = sampler.next_float();
        Float cos_theta = (1 - u) + u * cos_theta_max;
        Float cos_theta_sqr = cos_theta * cos_theta;
        Float sin_theta = std::sqrt(1 - cos_theta_sqr);

        Float phi = 2 * pi * sampler.next_float();
        Float x = cos(phi) * sin_theta;
        Float y = sin(phi) * sin_theta;
        vec3 cone_sample = vec3(x, y, cos_theta);

        glm::mat3 uvw = build_tbn_from_w(direction);
        s.direction = -glm::normalize(uvw * cone_sample);
        s.pdf = 1. / solid_angle;
        s.expected_distance_to_intersection = distance * cos_theta - std::sqrt(rad_sqr - dist_sqr * (1 - cos_theta_sqr));

#endif
        s.emission = sphere->brdf->emission();
        return s;
    }

    Spectrum SphereLight::eval(const vec3& direction) { return sphere->brdf->emission(); }

    Float SphereLight::pdf(const vec3& p, const vec3& ld)
    {
        Float dist = (sphere->pos - p).length();
        Float cos_theta_max = std::sqrt(std::max(dist * dist - sphere->rad * sphere->rad, 0.0f)) / dist;
        Float solid_angle = 2. * pi * (1 - cos_theta_max);
        return 1. / solid_angle;
    }

    Float SphereLight::power()
    {
        Spectrum em = sphere->brdf->emission();
        return 4. * pi * sphere->rad * sphere->rad * (em.r + em.g + em.b) * 0.33333333;
    }

    Float SphereLight::distance(const vec3& p)
    {
        return glm::distance(p,sphere->pos);
    }


    Light::Sample RectangleLight::sample(const SurfaceInteraction& si, Sampler& sampler)
    {
        Sample s;

        vec3 u = rectangle->vertex[1] - rectangle->vertex[0];
        vec3 v = rectangle->vertex[2] - rectangle->vertex[0];
        vec3 point_on_surface = rectangle->vertex[0] + u * sampler.next_float() + v * sampler.next_float();
        
        vec3 direction = si.pos - point_on_surface;
        Float distance = glm::length(direction);
        direction /= distance;

        Float light_cosine = std::abs(glm::dot(rectangle->normal[0], -direction));
        Float light_area = 4. * glm::determinant(rectangle->local_to_world);

        s.direction = direction;
        s.pdf = distance * distance / (light_area * light_cosine);
        s.expected_distance_to_intersection = distance;
        s.emission = rectangle->brdf->emission();
        return s;
    }

    Spectrum RectangleLight::eval(const vec3& direction) { return rectangle->brdf->emission(); }

    Float RectangleLight::pdf(const vec3& p, const vec3& ld)
    {
        return 1. / (4. * glm::determinant(rectangle->local_to_world));
    }

    Float RectangleLight::power()
    {
        Spectrum em = rectangle->brdf->emission();
        return 4. * glm::determinant(rectangle->local_to_world) * (em.r + em.g + em.b) * 0.33333333;
    }

    Float RectangleLight::distance(const vec3& p)
    {
        return glm::distance(p, (rectangle->vertex[0] + rectangle->vertex[2]) * 0.5f);
    }

} // namespace LT_NAMESPACE