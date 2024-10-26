/**
 * @file
 * @brief Definitions of the Camera and PerspectiveCamera classes.
 */

#pragma once

#include <lt/factory.h>
#include <lt/lt_common.h>
#include <lt/ray.h>
#include <lt/serialize.h>

namespace LT_NAMESPACE {

/**
 * @brief Abstract base class for camera objects.
 */
class Camera : public Serializable {
public:
    /**
     * @brief Constructor for Camera.
     * @param type The type of camera.
     */
    Camera(const std::string& type)
        : Serializable(type)
    {
    }

    /**
     * @brief Pure virtual function for generating a ray from the camera.
     * @param u The horizontal coordinate of the point on the image plane
     * (normalized between -1 and 1).
     * @param v The vertical coordinate of the point on the image plane
     * (normalized between -1 and 1).
     * @return The generated ray.
     */
    virtual Ray generate_ray(Float u, Float v) = 0;
};

/**
 * @brief Class representing a perspective camera.
 */
class PerspectiveCamera : public Camera {
public:
    /**
     * @brief Default constructor for PerspectiveCamera.
     * Initializes the camera type and default parameters.
     */
    PerspectiveCamera()
        : Camera("PerspectiveCamera")
        , pos(vec3(-1., 0., 0.))
        , center(vec3(0))
        , fov(40.)
        , aspect(1.)
    {
        link_params();
    }

    /**
     * @brief Initialize the perspective camera.
     * Computes view and projection matrices.
     */
    void init();


    /**
     * @brief Generate a ray from the perspective camera.
     * @param u The horizontal coordinate of the point on the image plane
     * (normalized between -1 and 1).
     * @param v The vertical coordinate of the point on the image plane
     * (normalized between -1 and 1).
     * @return The generated ray.
     */
    Ray generate_ray(Float u, Float v);

    vec3 pos; /**< Camera position. */
    vec3 center; /**< Target position. */
    float fov; /**< Field of view angle (in degrees). */
    float aspect; /**< Aspect ratio. */
    glm::mat4 view; /**< View matrix. */
    glm::mat4 proj; /**< Projection matrix. */
    glm::mat4 inv_view; /**< Inverse of view matrix. */
    glm::mat4 inv_proj; /**< Inverse of projection matrix. */

protected:
    /**
     * @brief Link parameters with the Params struct.
     */
    void link_params()
    {
        params.add("pos", ParamType::VEC3, &pos);
        params.add("center", ParamType::VEC3, &center);
        params.add("aspect", ParamType::FLOAT, &aspect);
        params.add("fov", ParamType::FLOAT, &fov);
    }

};



/**
 * @brief Class representing a Gonio camera.
 */
class GonioCamera : public Camera {
public:
    /**
     * @brief Default constructor for PerspectiveCamera.
     * Initializes the camera type and default parameters.
     */
    GonioCamera()
        : Camera("GonioCamera")
        , theta(0.)
        , phi(0.)
        , center(vec3(0))
        , offset(100.)
        , dir(vec3(0.,-1.,0.))
    {
        link_params();
    }

    /**
     * @brief Initialize the perspective camera.
     * Computes view and projection matrices.
     */
    void init();


    /**
     * @brief Generate a ray from the gonio camera.
     * @param u The horizontal coordinate of the point on the image plane
     * (normalized between -1 and 1).
     * @param v The vertical coordinate of the point on the image plane
     * (normalized between -1 and 1).
     * @return The generated ray.
     */
    Ray generate_ray(Float u, Float v);

    float theta = 0.f; /**< Camera zenith angle. */
    float phi   = 0.f; /**< Camera azimuth angle. */
    vec3  center= vec3(0.f); /**< Surface (quad) position. */
    float size  = 1.f; /**< Surface (quad) size */
    float offset= 100.f; /**< Ray offset to get a ray starting out of the surface*/
    vec3 dir    = vec3(0.,-1.,0.); /**< Ray direction computed from theta and phi*/
protected:
    /**
     * @brief Link parameters with the Params struct.
     */
    void link_params()
    {
        params.add("theta", ParamType::FLOAT, &theta);
        params.add("phi", ParamType::FLOAT, &phi);
        params.add("center", ParamType::VEC3, &center);
        params.add("size", ParamType::FLOAT, &size);
        params.add("offset", ParamType::FLOAT, &offset);
    }

};




} // namespace LT_NAMESPACE
