/**
 * @file
 * @brief Definitions of the Light and DirectionalLight classes.
 */

#pragma once

#include <lt/factory.h>
#include <lt/geometry.h>
#include <lt/io_exr.h>
#include <lt/lt_common.h>
#include <lt/serialize.h>
#include <lt/texture.h>

namespace LT_NAMESPACE {


    /**
     * @brief Abstract base class for light sources.
     */
    class Light : public Serializable {
    public:

        enum class Flags : uint16_t
        {
            dirac = 1 << 0,
            infinite = 1 << 1
        };


        struct Sample {
            vec3 direction;
            vec3 emission;
            Float pdf;
            Float expected_distance_to_intersection;
        };

        /**
            * @brief Constructor for Light.
            * @param type The type of light. ex : DirectionnalLight, EnvironmentLight ...
            */
        Light(const std::string& type)
            : Serializable(type)
        {}

        virtual Sample sample(const SurfaceInteraction& si, Sampler& sampler) = 0;

        virtual Spectrum eval(const vec3& direction) = 0;
        virtual Float pdf(const vec3& p, const vec3& ld) = 0;

        virtual int geometry_id() { return RTC_INVALID_GEOMETRY_ID; }

        Flags flags;
        inline bool is_dirac() {
            return static_cast<uint16_t>(flags) & static_cast<uint16_t>(Light::Flags::dirac);
        }

        inline bool is_infinite() {
            return static_cast<uint16_t>(flags) & static_cast<uint16_t>(Light::Flags::infinite);
        }
    };


    inline Light::Flags operator|(const Light::Flags& lhs, const Light::Flags& rhs)
    {
        return (Light::Flags)(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
    }

    inline Light::Flags operator&(const Light::Flags& lhs, const Light::Flags& rhs)
    {
        return (Light::Flags)(static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs));
    }





    /**
     * @brief Class representing a directional light source.
     */
    class DirectionnalLight : public Light {
    public:
        /**
         * @brief Default constructor for DirectionalLight.
         * Initializes the light type and default parameters.
         */
        DirectionnalLight()
            : Light("DirectionnalLight")
        {
            flags = Flags::dirac | Flags::infinite;
            link_params();
        }

        /**
         * @brief Sample the direction of the light.
         * @return The direction of the light.
         */
        Sample sample(const SurfaceInteraction& si, Sampler& sampler);

        Spectrum eval(const vec3& direction);
        Float pdf(const vec3& p, const vec3& ld);

        /**
         * @brief Initialize the directional light.
         * Normalize the direction vector.
         */
        void init();

        Float intensity = 0.5; /**< Intensity of the light. */
        vec3 dir = vec3(1, 0, 0); /**< Direction of the light. */

    protected:
        /**
         * @brief Link parameters with the Params struct.
         */
        void link_params()
        {
            params.add("dir", ParamType::VEC3, &dir);
            params.add("intensity", ParamType::FLOAT, &intensity);
        }
    };

    /**
     * @brief Class representing a directional light source.
     */
    class EnvironmentLight : public Light {
    public:
        EnvironmentLight()
            : Light("EnvironmentLight")
        {
            flags = Flags::infinite;
            link_params();
        }

        Sample sample(const SurfaceInteraction& si, Sampler& sampler);

        Spectrum eval(const vec3& direction);
        Float pdf(const vec3& p, const vec3& ld);

        void compute_density();

        void init();

        Texture<Spectrum> envmap;
        Float intensity;

        Texture<Float> density;
        Texture<Float> cumulative_density;
        Texture<int> inv_cumulative_density;
        std::vector<Float> c;
        Float dtheta;
        Float dphi;

    protected:
        void link_params()
        {
            params.add("texture", ParamType::TEXTURE, &envmap);
            params.add("intensity", ParamType::FLOAT, &intensity);
        }

    };

    class SphereLight : public Light {
    public:
        SphereLight()
            : Light("SphereLight")
        {
            flags = (Flags)0;
            link_params();
        }

        Sample sample(const SurfaceInteraction& si, Sampler& sampler);

        Spectrum eval(const vec3& direction);
        Float pdf(const vec3& p, const vec3& ld);

        int geometry_id() override { return sphere->rtc_id; }

        std::shared_ptr<Sphere> sphere;


    protected:
        /**
         * @brief All param are from Sphere and Sphere::brdf.
         */
        void link_params() { }
    };

    class RectangleLight : public Light {
    public:
        RectangleLight()
            : Light("RectangleLight")
        {
            flags = (Flags)0;
            link_params();
        }

        Sample sample(const SurfaceInteraction& si, Sampler& sampler);

        Spectrum eval(const vec3& direction);
        Float pdf(const vec3& p, const vec3& ld);

        int geometry_id() override { return rectangle->rtc_id; }

        std::shared_ptr<Rectangle> rectangle;


    protected:
        /**
         * @brief All param are from Sphere and Sphere::brdf.
         */
        void link_params() { }
    };


} // namespace LT_NAMESPACE