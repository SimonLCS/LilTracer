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



// Todo add infinite_lights supports
class PowerStrategie {
public:

    PowerStrategie(std::vector<std::shared_ptr<Light>>& lights_, std::vector<std::shared_ptr<Light>>& infinite_lights_) {

        lights = &lights_;
        infinite_lights = &infinite_lights_;

        lights_cdf.resize(lights->size() + 1);
        lights_pdf.resize(lights->size());

        for (int i = 0; i < lights->size(); i++) {
            lights_pdf[i] = (*lights)[i]->power();
        }

        lights_cdf[0] = 0;
        for (int i = 0; i < lights->size(); i++) {
            lights_cdf[i + 1] = lights_cdf[i] + lights_pdf[i];
        }

        for (int i = 0; i < lights->size(); i++) {
            lights_pdf[i] /= lights_cdf[lights_cdf.size() - 1];
            lights_cdf[i + 1] /= lights_cdf[lights_cdf.size() - 1];
        }

    }

    std::shared_ptr<Light> sample(const Float& u, Float* pdf) {

        int light_idx = binary_search(lights_cdf, u);

        const std::shared_ptr<Light>& light = (*lights)[light_idx];

        *pdf = lights_pdf[light_idx];
        return light;
    }

    std::vector<Float> lights_cdf;
    std::vector<Float> lights_pdf;
    std::vector<std::shared_ptr<Light>>* lights;
    std::vector<std::shared_ptr<Light>>* infinite_lights;
};

// Todo add infinite_lights supports
class SpatialPowerStrategie {

    struct Probe {
        std::vector<Float> lights_cdf;
        std::vector<Float> lights_pdf;
        
        Probe(const vec3& pos, std::vector<std::shared_ptr<Light>>* lights) {
            lights_cdf.resize(lights->size() + 1);
            lights_pdf.resize(lights->size());

            for (int i = 0; i < lights->size(); i++) {
                float dist = (*lights)[i]->distance(pos);
                lights_pdf[i] = (*lights)[i]->power() / (dist * dist);
            }

            lights_cdf[0] = 0;
            for (int i = 0; i < lights->size(); i++) {
                lights_cdf[i + 1] = lights_cdf[i] + lights_pdf[i];
            }

            for (int i = 0; i < lights->size(); i++) {
                lights_pdf[i] /= lights_cdf[lights_cdf.size() - 1];
                lights_cdf[i + 1] /= lights_cdf[lights_cdf.size() - 1];
            }
        }

        int sample(const Float& u, Float* pdf) {

            int light_idx = binary_search(lights_cdf, u);

            *pdf = lights_pdf[light_idx];
            return light_idx;
        }
    };

    struct Probe3DGrid {
        int subdiv_x;
        int subdiv_y;
        int subdiv_z;
        std::vector<Float> xpos;
        std::vector<Float> ypos;
        std::vector<Float> zpos;
        Bbox bbox;

        std::vector<std::vector<std::vector<Probe>>> probes;
 
        Probe3DGrid(std::vector<std::shared_ptr<Light>>* lights, const Bbox& b, const int& subdiv): bbox(b) {
            
            float maxdif = glm::max(bbox.pmax.x - bbox.pmin.x, bbox.pmax.y - bbox.pmin.y, bbox.pmax.z - bbox.pmin.z);

            subdiv_x = int(float(subdiv) * abs(bbox.pmax.x - bbox.pmin.x) / maxdif) ;
            subdiv_y = int(float(subdiv) * abs(bbox.pmax.y - bbox.pmin.y) / maxdif);
            subdiv_z = int(float(subdiv) * abs(bbox.pmax.z - bbox.pmin.z) / maxdif);

            xpos = linspace(bbox.pmin.x, bbox.pmax.x, subdiv_x, true);
            ypos = linspace(bbox.pmin.y, bbox.pmax.y, subdiv_y, true);
            zpos = linspace(bbox.pmin.z, bbox.pmax.z, subdiv_z, true);
            
            for (int i = 0; i < subdiv_x; i++) {
                probes.push_back(std::vector<std::vector<Probe>>());
                for (int j = 0; j < subdiv_y; j++) {
                    probes[i].push_back(std::vector<Probe>());
                    for (int k = 0; k < subdiv_z; k++) {
                        vec3 pos = vec3(xpos[i], ypos[j], zpos[k]);
                        probes[i][j].push_back(Probe(pos,lights));
                    }
                }
            }


        }

        Probe& fetch(vec3 pos) {
            glm::uvec3 idx = glm::uvec3(vec3(subdiv_x, subdiv_y, subdiv_z) * ((pos - bbox.pmin) / (bbox.pmax - bbox.pmin)));
            return probes[idx.x][idx.y][idx.z];
        };
    };

public:

    SpatialPowerStrategie(std::vector<std::shared_ptr<Light>>& lights_, std::vector<std::shared_ptr<Light>>& infinite_lights_, const Bbox& bbox, const int& subdiv = 10) {

        lights = &lights_;
        infinite_lights = &infinite_lights_;

        grid = std::make_unique<Probe3DGrid>(lights, bbox, subdiv);

    }

    std::shared_ptr<Light> sample(const vec3& pos, const Float& u, Float* pdf) {
        Probe& probe = grid->fetch(pos);    
        int light_idx = probe.sample(u, pdf);
        const std::shared_ptr<Light>& light = (*lights)[light_idx];
        return light;
    }

    std::vector<std::shared_ptr<Light>>* lights;
    std::vector<std::shared_ptr<Light>>* infinite_lights;

    std::unique_ptr<Probe3DGrid> grid;
};


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
            si.uv = geom->get_uv(rayhit, si.pos);
            si.geom_id = geom_id;
            
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

    void init()
    {
        
        if (geometries.size() > 0) {
            bbox = geometries[0]->bbox();
            for (auto geometry : geometries) {
                bbox.grow(geometry->bbox());
            }
        }

        
        ps = std::make_shared<PowerStrategie>(lights, infinite_lights);
        sps = std::make_shared<SpatialPowerStrategie>(lights, infinite_lights, bbox, 500);

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

    std::shared_ptr<PowerStrategie> ps;
    std::shared_ptr<SpatialPowerStrategie> sps;

    Bbox bbox;
};


} // namespace LT_NAMESPACE
