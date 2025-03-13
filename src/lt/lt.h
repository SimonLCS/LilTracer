#pragma once

#include <lt/brdf_common.h>
#include <lt/camera.h>
#include <lt/geometry.h>
#include <lt/integrator.h>
#include <lt/io.h>
#include <lt/io_exr.h>
#include <lt/lt_common.h>
#include <lt/ray.h>
#include <lt/sampler.h>
#include <lt/scene.h>
#include <lt/sensor.h>

namespace LT_NAMESPACE {

	inline bool dir_light(Scene& scn, Renderer& ren)
	{
		return generate_from_json("", R"(
	   	{
			"integrator": {
				"type":"DirectIntegrator"
			},
			"brdf": [
				{"type":"Diffuse","name":"sphere_brdf","albedo":[0.2,0.5,0.8]}
			],
			"light": [
				{"type":"DirectionnalLight", "intensity" : 1.0, "dir" : [1.0, 0.0, 0.0] }
			],
			"geometries": [
				{"type":"Sphere","brdf":"sphere_brdf" , "pos":[0,0,0], "rad":1.0}
			],
			"sensor": {
				"type" : "Sensor",
				"width" : 512,
				"height" : 512
			},
			"camera": {
				"type":"PerspectiveCamera",
				"fov" : 30,
				"aspect" : 1.0,
				"center" : [0.0,0.0,0.0],
				"pos" : [5.0,0.0,0.0]
			}
		}
		)",
			scn, ren);
	}

} // namespace LT_NAMESPACE