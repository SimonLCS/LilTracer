/**
 * @file
 * @brief Definition of the Scene class.
 */

#pragma once

#include <lt/brdf_common.h>
#include <lt/geometry.h>
#include <lt/light.h>
#include <lt/lt_common.h>
#include <lt/surface_interaction.h>

namespace LT_NAMESPACE {

/**
 * @brief Class representing a scene for ray tracing.
 */
class Scene {
public:
    /**
     * @brief Intersect a ray with the scene and update the surface interaction if
     * there is an intersection.
     * @param r The ray to intersect with the scene.
     * @param si The surface interaction to update if there is an intersection.
     * @return True if the ray intersects with the scene, false otherwise.
     */
    bool intersect(const Ray& r, SurfaceInteraction& si)
    {
        RTCRayHit rayhit;
        rayhit.ray.org_x = r.o.x;
        rayhit.ray.org_y = r.o.y;
        rayhit.ray.org_z = r.o.z;
        rayhit.ray.dir_x = r.d.x;
        rayhit.ray.dir_y = r.d.y;
        rayhit.ray.dir_z = r.d.z;
        rayhit.ray.tnear = 0.f;
        rayhit.ray.tfar = std::numeric_limits<float>::infinity();
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(scene, &context, &rayhit);

        if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            unsigned int geom_id = rayhit.hit.geomID;
            std::shared_ptr<Geometry> geom = geometries[geom_id];

            si.t = rayhit.ray.tfar;
            si.brdf = geom->brdf;
            si.pos = r.o + r.d * si.t;
            si.nor = geom->get_normal(rayhit, si.pos);
            si.geom_id = geom_id;
            // si.nor = vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);

            si.finalize();
            return true;
        }

        return false;
    }

    /**
     * @brief Check if a ray intersects with the scene.
     * @param r The ray to check for intersection.
     * @return True if the ray intersects with the scene, false otherwise.
     */
    bool shadow(const Ray &r, const Float& tfar = std::numeric_limits<Float>::infinity()) {
      /*RTCRay ray;
      ray.org_x = r.o.x;
      ray.org_y = r.o.y;
      ray.org_z = r.o.z;
      ray.dir_x = r.d.x;
      ray.dir_y = r.d.y;
      ray.dir_z = r.d.z;
      ray.tnear = 0.f;
      ray.tfar = tfar;
      ray.flags = 0;

      rtcOccluded1(scene, &context, &ray);

      if (ray.tfar < 0.) {
        return true;
      }

      return false;*/

    //RTCRayHit rayhit;
    //rayhit.ray.org_x = r.o.x;
    //rayhit.ray.org_y = r.o.y;
    //rayhit.ray.org_z = r.o.z;
    //rayhit.ray.dir_x = r.d.x;
    //rayhit.ray.dir_y = r.d.y;
    //rayhit.ray.dir_z = r.d.z;
    //rayhit.ray.tnear = 0.f;
    //rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    //rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    //rtcIntersect1(scene, &context, &rayhit);

    //if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID && rayhit.ray.tfar < tfar) {
    //    return true;
    //}

    //return false;

    RTCRayHit rayhit;
    rayhit.ray.org_x = r.o.x;
    rayhit.ray.org_y = r.o.y;
    rayhit.ray.org_z = r.o.z;
    rayhit.ray.dir_x = r.d.x;
    rayhit.ray.dir_y = r.d.y;
    rayhit.ray.dir_z = r.d.z;
    rayhit.ray.tnear = 0.f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene, &context, &rayhit);

    if (rayhit.ray.tfar < tfar) {
        return true;
    }

    return false;
    }

    bool shadow_to(const Ray& r, const Float& tfar) {
        

        RTCRayHit rayhit;
        rayhit.ray.org_x = r.o.x;
        rayhit.ray.org_y = r.o.y;
        rayhit.ray.org_z = r.o.z;
        rayhit.ray.dir_x = r.d.x;
        rayhit.ray.dir_y = r.d.y;
        rayhit.ray.dir_z = r.d.z;
        rayhit.ray.tnear = 0.f;
        rayhit.ray.tfar = std::numeric_limits<float>::infinity();
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(scene, &context, &rayhit);

        float epsilon = 0.0001;
        if (rayhit.ray.tfar < tfar + epsilon && rayhit.ray.tfar > tfar - epsilon) {
            return false;
        }

        return true;
    }

    /*
    std::shared_ptr<Geometry> intersect(const Ray& r)
    {
        RTCRayHit rayhit;
        rayhit.ray.org_x = r.o.x;
        rayhit.ray.org_y = r.o.y;
        rayhit.ray.org_z = r.o.z;
        rayhit.ray.dir_x = r.d.x;
        rayhit.ray.dir_y = r.d.y;
        rayhit.ray.dir_z = r.d.z;
        rayhit.ray.tnear = 0.f;
        rayhit.ray.tfar = std::numeric_limits<float>::infinity();
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

        rtcIntersect1(scene, &context, &rayhit);

        if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            return geometries[rayhit.hit.geomID];
        }

        return nullptr;
    }*

    /**
     * @brief Initialize Embree RTC device and scene.
     */
    void init_rtc()
    {
        device = rtcNewDevice(NULL);
        scene = rtcNewScene(device);

        for (int i = 0; i < geometries.size(); i++) {
            geometries[i]->init_rtc(device);
            rtcCommitGeometry(geometries[i]->rtc_geom);
            //rtcSetGeometryTransform(geometries[i]->rtc_geom, 0, RTC_FORMAT_FLOAT4X4_ROW_MAJOR, (float*)(&geometries[i]->local_to_world[0]) );
            unsigned int geomID = rtcAttachGeometry(scene, geometries[i]->rtc_geom);
            geometries[i]->rtc_id = geomID;
            rtcReleaseGeometry(geometries[i]->rtc_geom);
        }

        rtcCommitScene(scene);

        rtcInitIntersectContext(&context);
    }

    RTCDevice device; /**< Embree RTC device. */
    RTCScene scene; /**< Embree RTC scene. */
    RTCIntersectContext context; /**< Embree RTC intersect context. */

    std::vector<std::shared_ptr<Geometry>>
        geometries; /**< Vector of geometry in the scene. */
    std::vector<std::shared_ptr<Light>>
        lights; /**< Vector of light in the scene. */
    std::vector<std::shared_ptr<Brdf>> brdfs; /**< Vector of BRDF in the scene. */
    std::vector<std::shared_ptr<Light>> infinite_lights;
};

} // namespace LT_NAMESPACE
