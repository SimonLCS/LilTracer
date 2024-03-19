/**
 * @file
 * @brief Contains functions for generating scenes and rendering them from JSON
 * descriptions.
 */

#pragma once
#include <lt/lt_common.h>
#include <lt/renderer.h>
#include <lt/scene.h>
#include <lt/texture.h>

#include <tiny_exr/tinyexr.h>
#include <nlohmann/json.hpp>

#include <fstream>

namespace LT_NAMESPACE {

using json = nlohmann::json;

/**
 * @brief Set a float value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the float variable.
 */
static void json_set_float(const json& j, float* ptr) { *ptr = j; }

/**
 * @brief Set a vec3 value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the vec3 variable.
 */
static void json_set_vec3(const json& j, vec3* ptr)
{
    (*ptr)[0] = j[0];
    (*ptr)[1] = j[1];
    (*ptr)[2] = j[2];
}

/**
 * @brief Set a path value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the string variable.
 */
static void json_set_path(const json& j, std::string* ptr, const std::string& dir)
{
    *ptr = dir + std::string(j);
}

/**
 * @brief Set a BRDF value from JSON.
 * the map of BRDFs have to be defined beforehand
 * @param j The JSON value.
 * @param ptr Pointer to the BRDF variable.
 * @param ref Reference to the map of BRDFs.
 */
static void json_set_brdf(const json& j, std::shared_ptr<Brdf>* ptr,
    std::map<std::string, std::shared_ptr<Brdf>>& ref)
{
    std::string brdf_name = j;
    *ptr = ref[brdf_name];
}

static void json_set_texture(const json& j, Texture<Spectrum>* ptr, const std::string& dir)
{
    std::string texture_path = dir + std::string(j);
    if (load_texture_exr(texture_path, *ptr))
        std::cerr << texture_path << " : cannot be loaded. " << std::endl;
}

/**
 * @brief Set parameters from JSON.
 * @param j The JSON object.
 * @param params The Params object containing parameter information.
 * @param brdf_ref Reference to the map of BRDFs.
 */
static void set_params(const json& j, const Params& params, const std::string& dir,
    std::map<std::string, std::shared_ptr<Brdf>>& brdf_ref)
{
    for (int i = 0; i < params.count; i++) {
        if (j.contains(params.names[i])) {
            switch (params.types[i]) {
            case Params::Type::FLOAT:
                json_set_float(j[params.names[i]], (float*)params.ptrs[i]);
                break;
            case Params::Type::VEC3:
                json_set_vec3(j[params.names[i]], (vec3*)params.ptrs[i]);
                break;
            case Params::Type::PATH:
                json_set_path(j[params.names[i]], (std::string*)params.ptrs[i], dir);
                break;
            case Params::Type::BRDF:
                json_set_brdf(j[params.names[i]],
                    (std::shared_ptr<Brdf>*)params.ptrs[i], brdf_ref);
                break;
            case Params::Type::TEXTURE:
                json_set_texture(j[params.names[i]],
                    (Texture<Spectrum>*)params.ptrs[i], dir);
                break;
            default:
                std::cerr << "json to Params::Type not defined" << std::endl;
                break;
            }
        } else {
            std::cout << "missing : " << params.names[i] << std::endl;
        }
    }
}

/**
 * @brief Generate a scene and renderer from a JSON description.
 *
 * This function parses a JSON description of a scene and generates the
 * corresponding Scene and Renderer objects.
 *
 * @param str The JSON description as a string.
 * @param scn Reference to the Scene object to be filled.
 * @param ren Reference to the Renderer object to be filled.
 * @return True if the generation is successful, false otherwise.
 */
static bool generate_from_json(const std::string& dir, const std::string& str, Scene& scn,
    Renderer& ren)
{
    // Map to store references to BRDFs
    std::map<std::string, std::shared_ptr<Brdf>> brdf_ref;

    // Parse the JSON string
    json json_scn;
    try {
        json_scn = json::parse(str);
    } catch (const json::exception& e) {
        // Handle JSON parsing errors
        std::cerr << e.what() << std::endl;
    }

    // Initialize sampler in the renderer
    ren.sampler = std::make_shared<Sampler>();

    // Parse Integrator
    if (json_scn.contains("integrator")) {
        json json_integrator = json_scn["integrator"];
        std::shared_ptr<Integrator> integrator = Factory<Integrator>::create(json_integrator["type"]);

        if (!integrator)
            return false;

        // Set parameters and initialize the integrator
        set_params(json_integrator, integrator->params, dir, brdf_ref);
        integrator->init();

        // Set the integrator in the renderer
        ren.integrator = integrator;
    } else {
        std::cerr << "Abort generate_from_json, cause : Missing integrator"
                  << std::endl;
    }

    // Parse Sensor
    if (json_scn.contains("sensor")) {
        json json_sensor = json_scn["sensor"];
        uint16_t width = json_sensor["width"];
        uint16_t height = json_sensor["height"];
        // Create and set the sensor in the renderer
        ren.sensor = std::make_shared<Sensor>(width, height);
    } else {
        std::cerr << "Abort generate_from_json, cause : Missing sensor"
                  << std::endl;
    }

    // Parse Camera
    if (json_scn.contains("camera")) {
        json json_camera = json_scn["camera"];
        std::shared_ptr<Camera> camera = Factory<Camera>::create(json_camera["type"]);

        if (!camera)
            return false;

        // Set parameters and initialize the camera
        set_params(json_camera, camera->params, dir, brdf_ref);
        camera->init();

        // Set the camera in the renderer
        ren.camera = camera;
    } else {
        std::cerr << "Abort generate_from_json, cause : Missing camera"
                  << std::endl;
    }

    // Parse BRDF
    if (json_scn.contains("brdf")) {
        for (const auto& json_brdf : json_scn["brdf"]) {
            std::shared_ptr<Brdf> brdf = Factory<Brdf>::create(json_brdf["type"]);
            if (!brdf)
                return false;

            // Store references to BRDFs by name
            brdf_ref[json_brdf["name"]] = brdf;

            // Set parameters and initialize the BRDF
            set_params(json_brdf, brdf->params, dir, brdf_ref);
            brdf->init();

            // Add the BRDF to the scene
            scn.brdfs.push_back(brdf);
        }
    } else {
        std::cerr << "Abort generate_from_json, cause : Missing brdf" << std::endl;
    }

    // Parse Background
    if (json_scn.contains("background")) {
        json json_background = json_scn["background"];
        std::shared_ptr<Light> envmap = Factory<Light>::create(json_background["type"]);

        if (!envmap)
            return false;

        // Set parameters and initialize the camera
        set_params(json_background, envmap->params, dir, brdf_ref);
        envmap->init();

        // Set the camera in the renderer
        scn.infinite_lights.push_back(envmap);
    }

    // Parse Light
    if (json_scn.contains("light")) {
        for (const auto& json_light : json_scn["light"]) {
            std::shared_ptr<Light> light = Factory<Light>::create(json_light["type"]);
            if (!light)
                return false;

            // Set parameters and initialize the light
            set_params(json_light, light->params, dir, brdf_ref);
            light->init();

            // Add the light to the scene
            scn.lights.push_back(light);
        }
    } else {
        std::cerr << "Abort generate_from_json, cause : Missing light" << std::endl;
    }

    // Parse Geometrys
    if (json_scn.contains("geometries")) {
        for (const auto& json_geometry : json_scn["geometries"]) {
            std::cout << json_geometry << std::endl;

            std::shared_ptr<Geometry> geometry = Factory<Geometry>::create(json_geometry["type"]);
            if (!geometry)
                return false;

            // Set parameters and initialize the geometry
            set_params(json_geometry, geometry->params, dir, brdf_ref);
            geometry->init();

            if (geometry->brdf->emissive() && geometry->type == "Sphere") {
                std::shared_ptr<SphereLight> sphere_light = std::make_shared<SphereLight>();
                sphere_light->sphere = std::dynamic_pointer_cast<Sphere>(geometry);
                sphere_light->init();
                scn.lights.push_back(sphere_light);
            }

            // Add the geometry to the scene
            scn.geometries.push_back(geometry);
        }
    } else {
        std::cerr << "Abort generate_from_json, cause : Missing geometries"
                  << std::endl;
    }

    // Initialize the scene's acceleration structure
    scn.init_rtc();

    return true;
}

static bool generate_from_path(const std::string& path, Scene& scn, Renderer& ren) {
    
    std::ifstream t(path);
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string str(size, ' ');
    t.seekg(0);
    t.read(&str[0], size);

    std::filesystem::path std_path(path);
    std::filesystem::path std_dir = std_path.parent_path();

    return generate_from_json(std_dir.string()+"/", str, scn, ren);
}

} // namespace LT_NAMESPACE