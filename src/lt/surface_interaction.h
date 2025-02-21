/**
 * @file
 * @brief Definition of the SurfaceInteraction class.
 */

#pragma once

//#include <lt/brdf_common.h>
#include <lt/brdf/brdf.h>
#include <lt/lt_common.h>

namespace LT_NAMESPACE {

class Brdf;

class SurfaceInteraction {
public:
    /**
     * @brief Default constructor for SurfaceInteraction.
     * Initializes normal and position to zero vectors, sets t to a large value,
     * and initializes brdf to nullptr.
     */
    SurfaceInteraction()
        : nor(vec3(0.))
        , pos(vec3(0.))
        , t(1000000.)
        , u(0.)
        , v(0.)
        , brdf(nullptr)
    {
    }

    /**
     * @brief Constructor for SurfaceInteraction.
     * @param pos Position of the intersection point.
     * @param nor Normal at the intersection point.
     * Initializes normal and position with given parameters, sets t to a large
     * value, and initializes brdf to nullptr.
     */
    SurfaceInteraction(vec3 pos, vec3 nor)
        : nor(nor)
        , pos(pos)
        , t(1000000.)
        , u(0.)
        , v(0.)
        , brdf(nullptr)
    {
    }
    vec3 nor; /**< Normal at the intersection point. */
    vec3 pos; /**< Position of the intersection point. */
    Float t; /**< Distance to the intersection point. */
    Float u;
    Float v;
    std::shared_ptr<Brdf> brdf; /**< Pointer to the surface BRDF. */
    unsigned int geom_id;

    glm::mat3 tbn; /**< Tangent-Bitangent-Normal matrix. */
    glm::mat3 inv_tbn; /**< Inverse Tangent-Bitangent-Normal matrix. */
    vec3 tan; /**< Tangent vector. */
    vec3 bitan; /**< Bitangent vector. */


    /**
     * @brief Finalizes the surface interaction after the setting normal and
     * position vectors by computing the orthonormal basis
     */
    void finalize()
    {
        orthonormal_basis(nor, tan, bitan);
        tbn = glm::mat3(tan, bitan, nor);
        inv_tbn = glm::transpose(tbn);
    }

    /**
     * @brief Transforms a vector from local space to world space.
     * \ref finalize() need to be called beforehand.
     * @param v The vector to transform.
     * @return The transformed vector in world space.
     */
    vec3 to_world(const vec3& v) {
        return tbn * v;
    }

    /**
     * @brief Transforms a vector from world space to local space.
     * \ref finalize() need to be called beforehand.
     * @param v The vector to transform.
     * @return The transformed vector in local space.
     */
    vec3 to_local(const vec3& v) {
        return inv_tbn * v;
    }
};


} // namespace LT_NAMESPACE