/**
 * @file brdf.h
 * @brief Defines classes related to Bidirectional Reflectance Distribution
 * Functions (BRDFs).
 */

#pragma once

#include <lt/lt_common.h>
#include <lt/sampler.h>
#include <lt/serialize.h>
#include <lt/surface_interaction.h>
#include <lt/texture.h>

namespace LT_NAMESPACE {

class SurfaceInteraction;

/**
 * @brief Base class for Bidirectional Reflectance Distribution Functions
 * (BRDFs).
 */
class Brdf : public Serializable {
public:


    enum class Flags : uint16_t
    {
        rough = 1 << 0,
        specular = 1 << 1,
        diffuse = 1 << 2,
        reflection = 1 << 3,
        transmission = 1 << 4,
        emissive = 1 << 5
    };


    struct Sample {
        vec3 wo;
        Spectrum value; // brdf / pdf
        Flags flags;
    };

    /**
     * @brief Constructor.
     * @param type The type of the BRDF.
     */
    Brdf(const std::string& type)
        : Serializable(type) 
        , flags(Flags::diffuse)
    {};

    /**
     * @brief Evaluates the BRDF * cos_theta_o.
     * @param wi Incident direction.
     * @param wo Outgoing direction.
     * @return The evaluated spectrum.
     */
    virtual Spectrum eval(vec3 wi, vec3 wo, const SurfaceInteraction& si, Sampler& sampler);

    /**
     * @brief Samples the BRDF.
     * @param wi Incident direction.
     * @param sampler The sampler object used for sampling.
     * @return The sampled direction.
     */
    virtual Sample sample(const vec3& wi, const SurfaceInteraction& si, Sampler& sampler);

    /**
     * @brief Computes the density of a sample of wo.
     * @param wo Direction.
     * @return The density value.
     */
    virtual float pdf(const vec3& wi, const vec3& wo, const SurfaceInteraction& si);
    
    Flags flags;
    inline bool is_emissive() {
        return static_cast<uint16_t>(flags) & static_cast<uint16_t>(Flags::emissive);
    }

    virtual Spectrum emission();

};


inline Brdf::Flags operator|(const Brdf::Flags& lhs, const Brdf::Flags& rhs)
{
    return (Brdf::Flags)(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}

inline Brdf::Flags operator&(const Brdf::Flags& lhs, const Brdf::Flags& rhs)
{
    return (Brdf::Flags)(static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs));
}




} // namespace LT_NAMESPACE