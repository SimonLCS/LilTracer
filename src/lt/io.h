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
#include <filesystem>

namespace LT_NAMESPACE {

using json = nlohmann::json;

/**
 * @brief Set a bool value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the bool variable.
 */
static void json_set_bool(const json& j, bool* ptr) { *ptr = (bool)j; }

/**
 * @brief Set a float value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the float variable.
 */
static void json_set_float(const json& j, float* ptr) { *ptr = j; }

/**
 * @brief Set a int value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the int variable.
 */
static void json_set_int(const json& j, int* ptr) { *ptr = j; }


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
 * @brief Set a ior value from JSON.
 * @param j The JSON value.
 * @param ptr Pointer to the ior variable.
 */
static void json_set_ior(const json& j, Spectrum* ptr)
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
        Log(logError) << texture_path << " : cannot be loaded. ";
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
            case Params::Type::BOOL:
                json_set_bool(j[params.names[i]], (bool*)params.ptrs[i]);
                break;
            case Params::Type::FLOAT:
                json_set_float(j[params.names[i]], (float*)params.ptrs[i]);
                break;
            case Params::Type::INT:
                json_set_int(j[params.names[i]], (int*)params.ptrs[i]);
                break;
            case Params::Type::VEC3:
                json_set_vec3(j[params.names[i]], (vec3*)params.ptrs[i]);
                break;
            case Params::Type::IOR:
                json_set_ior(j[params.names[i]], (Spectrum*)params.ptrs[i]);
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
                Log(logError) << "json to Params::Type not defined";
                break;
            }
        } else {
            Log(logInfo) << "missing : " << params.names[i];
        }
    }
}

/**
 * @brief Generate a scene and renderer from a JSON description.
 *
 * This function parses a JSON description of a scene and generates the
 * corresponding Scene and Renderer objects.
 *
 * @param path The path to the JSON file
 * @param str The JSON description as a string.
 * @param scn Reference to the Scene object to be filled.
 * @param ren Reference to the Renderer object to be filled.
 * @return True if the generation is successful, false otherwise.
 */
static bool generate_from_json(const std::string& path, const std::string& str, Scene& scn,
    Renderer& ren)
{
    std::filesystem::path std_path(path);
    const std::string dir = std_path.parent_path().string() + "/";

    // Map to store references to BRDFs
    std::map<std::string, std::shared_ptr<Brdf>> brdf_ref;

    // Parse the JSON string
    json json_scn;
    try {
        json_scn = json::parse(str);
    } catch (const json::exception& e) {
        // Handle JSON parsing errors
        Log(logError) << e.what();
    }

    // Initialize sampler in the renderer
    ren.sampler = std::make_shared<Sampler>();

    // Parse max sample
    if (json_scn.contains("max_sample")) {
        ren.max_sample = (int)json_scn["max_sample"];
    }

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
        Log(logWarning) << "generate_from_json, cause : Missing integrator in file " << path;
    }

    // Parse Sensor
    if (json_scn.contains("sensor")) {
        json json_sensor = json_scn["sensor"];
        std::shared_ptr<Sensor> sensor = Factory<Sensor>::create(json_sensor["type"]);

        if (!sensor)
            return false;

        // Set parameters and initialize the sensor
        set_params(json_sensor, sensor->params, dir, brdf_ref);
        sensor->init();

        ren.sensor = sensor;
    } else {
        Log(logWarning) << "generate_from_json, cause : Missing sensor in file " << path;
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
        Log(logWarning) << "generate_from_json, cause : Missing camera in file " << path;
    }

    // Parse BRDF
    if (json_scn.contains("brdf")) {
        for (const auto& json_brdf : json_scn["brdf"]) {
            std::shared_ptr<Brdf> brdf = Factory<Brdf>::create(json_brdf["type"]);
            if (!brdf)
                return false;

            // Store references to BRDFs by name
            // !!! We should  check the existence of json_brdf["name"] !!!
            brdf_ref[json_brdf["name"]] = brdf;

            // Set parameters and initialize the BRDF
            set_params(json_brdf, brdf->params, dir, brdf_ref);
            brdf->init();

            // Add the BRDF to the scene
            scn.brdfs.push_back(brdf);
        }
    } else {
        Log(logWarning) << "generate_from_json, cause : Missing brdf in file " << path;
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
        Log(logWarning) << "generate_from_json, cause : Missing light in file " << path;
    }

    // Parse Geometrys
    if (json_scn.contains("geometries")) {
        for (const auto& json_geometry : json_scn["geometries"]) {
            Log(logInfo) << json_geometry;

            std::shared_ptr<Geometry> geometry = Factory<Geometry>::create(json_geometry["type"]);
            if (!geometry)
                return false;

            // Set parameters and initialize the geometry
            set_params(json_geometry, geometry->params, dir, brdf_ref);
            geometry->init();

            if (geometry->brdf->is_emissive() && geometry->type == "Sphere") {
                std::shared_ptr<SphereLight> sphere_light = std::make_shared<SphereLight>();
                sphere_light->sphere = std::dynamic_pointer_cast<Sphere>(geometry);
                sphere_light->init();
                scn.lights.push_back(sphere_light);
            }

            // Add the geometry to the scene
            scn.geometries.push_back(geometry);
        }
    } else {
        Log(logWarning) << "generate_from_json, cause : Missing geometries in file " << path;
    }

    // Initialize the scene's acceleration structure
    scn.init_rtc();

    return true;
}

static bool generate_from_path(const std::string& path, Scene& scn, Renderer& ren) {
    if (path.ends_with(".json")) {
        std::ifstream t(path);
        if (t.fail()) {
            Log(logError) << "generate_from_path: file not found (" << path << ")";
            return false;
        }

        t.seekg(0, std::ios::end);
        size_t size = t.tellg();
        std::string str(size, ' ');
        t.seekg(0);
        t.read(&str[0], size);

        return generate_from_json(path, str, scn, ren);
    }
    Log(logError) << "generate_from_path: file format not supported (" << path << ")";
    return false;
}

} // namespace LT_NAMESPACE