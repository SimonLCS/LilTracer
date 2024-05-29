/**
 * @file integrator.h
 * @brief Defines classes related to rendering integrators.
 */

#pragma once
#include <lt/camera.h>
#include <lt/lt_common.h>
#include <lt/sampler.h>
#include <lt/scene.h>
#include <lt/sensor.h>
#include <lt/serialize.h>

#include <chrono>

//#define SAMPLE_OPTIM
//#define USE_MIS
namespace LT_NAMESPACE {

inline Float power_heuristic(const Float& pdf_1, const Float& pdf_2) {
    return (pdf_1 * pdf_1) / (pdf_1 * pdf_1 + pdf_2 * pdf_2);
}

class Integrator : public Serializable {
public:
    /**
     * @brief Constructor.
     * @param type The type of the integrator.
     */
    Integrator(const std::string& type)
        : Serializable(type)
    {
        n_sample = 1;
    };

    /**
     * @brief Renders the scene.
     * @param camera The camera used for rendering.
     * @param sensor The sensor to capture the rendered image.
     * @param scene The scene to render.
     * @param sampler The sampler used for sampling.
     * @return The time taken in milliseconds.
     */
    float render(std::shared_ptr<Camera> camera, std::shared_ptr<Sensor> sensor,
        Scene& scene, Sampler& sampler)
    {
        auto t1 = std::chrono::high_resolution_clock::now();

#if 0
			for (int h = 0; h < sensor->h; h++) {
				for (int w = 0; w < sensor->w; w++) {
					float jw = (2. * sampler.next_float()) / (float)sensor->w;
					float jh = (2. * sampler.next_float()) / (float)sensor->h;

					Ray r = camera->generate_ray(sensor->u[w] + jw, sensor->v[h] + jh);
					Spectrum s;
					render_pixel(r, s, scene, sampler);
						
					sensor->add(w, h, s);	

				}
			}
#endif
#if 1
        int block_size = 16;
        //std::cout << "in" << std::endl;
#pragma omp parallel for collapse(2) schedule(dynamic)
        for (int h = 0; h < sensor->h / block_size + 1; h++)
            for (int w = 0; w < sensor->w / block_size + 1; w++) {
                Sampler s;
                //s.seed(n_sample);
                s.seed((h + w * (block_size + 1) + 1) * n_sample);
                render_block(h, w, block_size, camera, sensor, scene, sampler);
            }

#endif
        //std::cout << "out" << std::endl;

#if 0
			int block_size = 16;
			int h_num_block = sensor->h / block_size + 1;
			int w_num_block = sensor->w / block_size + 1;
			
			tbb::task_group tg;
			
			for (int h = 0; h < sensor->h / block_size + 1; h++)
				for (int w = 0; w < sensor->w / block_size + 1; w++) {
					tg.run([&] {
						Sampler s;// (sampler);
						s.seed(h + w * (block_size + 1));
						render_block(h, w, block_size, camera, sensor, scene, s);
					});
				}
			tg.wait();
			tbb::missing_wait();
#endif

        n_sample++;

        auto t2 = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        return delta_time;
    };

    /**
     * @brief Renders a block of pixels in the scene.
     * @param id_h The ID of the block in the vertical direction.
     * @param id_w The ID of the block in the horizontal direction.
     * @param block_size The size of the block.
     * @param camera The camera used for rendering.
     * @param sensor The sensor to capture the rendered image.
     * @param scene The scene to render.
     * @param sampler The sampler used for sampling.
     */
    void render_block(uint32_t id_h, uint32_t id_w, uint32_t block_size,
        std::shared_ptr<Camera> camera,
        std::shared_ptr<Sensor> sensor, Scene& scene,
        Sampler& sampler)
    {
        uint32_t h_min = id_h * block_size;
        uint32_t w_min = id_w * block_size;

        uint32_t h_max = std::min((id_h + 1) * block_size, sensor->h);
        uint32_t w_max = std::min((id_w + 1) * block_size, sensor->w);
        for (int h = h_min; h < h_max; h++) {
            for (int w = w_min; w < w_max; w++) {
                float jw = (2. * sampler.next_float()) / (float)sensor->w;
                float jh = (2. * sampler.next_float()) / (float)sensor->h;

                Ray r = camera->generate_ray(sensor->u[w] + jw, sensor->v[h] + jh);
                Spectrum s = render_pixel(r, scene, sampler);

                sensor->add(w, h, s);
            }
        }
    }

    /**
     * @brief Renders a single pixel in the scene.
     * @param r The ray starting from the pixel.
     * @param scene The scene to render.
     * @param sampler The sampler used for sampling.
     * @return The resulting contribution.
     */
    virtual Spectrum render_pixel(Ray& r, Scene& scene, Sampler& sampler) = 0;

    /**
     * @brief Estimates direct lighting contribution from random light source.
     * @param r The ray representing the pixel.
     * @param si Surface interaction data.
     * @param scene The scene to render.
     * @param sampler The sampler used for sampling.
     * @return The estimated direct lighting contribution.
     */
    Spectrum uniform_sample_one_light(Ray& r, SurfaceInteraction& si,
        Scene& scene, Sampler& sampler)
    {
        int n_light = scene.lights.size() + scene.infinite_lights.size();

        if (n_light == 0)
            return Spectrum(0.);

        int light_idx = std::min((int)(sampler.next_float() * n_light), n_light - 1);
        Float light_pdf = (Float)(1.) / n_light;

        const std::shared_ptr<Light>& light = light_idx < scene.lights.size()
            ? scene.lights[light_idx]
            : scene.infinite_lights[n_light - light_idx - 1];

        return estimate_direct(r, si, light, scene, sampler) / light_pdf;
    }

    /**
     * @brief Estimates direct lighting contribution from a light source.
     * @param r The ray representing the pixel.
     * @param si Surface interaction data.
     * @param light The light source.
     * @param scene The scene to render.
     * @param sampler The sampler used for sampling.
     * @return The estimated direct lighting contribution.
     */
    Spectrum estimate_direct(Ray& r, SurfaceInteraction& si,
        const std::shared_ptr<Light>& light, Scene& scene,
        Sampler& sampler)
    {
        vec3 contrib = vec3(0.);
        
        si.pos -= r.d * 0.00001f;

        Light::Sample ls = light->sample(si, sampler);

        vec3 wo = si.to_local(-ls.direction);
        vec3 wi = si.to_local(-r.d);

        Ray rs(si.pos,-ls.direction);

        if (!scene.shadow(rs, ls.expected_distance_to_intersection-0.00001)) {

            if (wi.z < 0.00001)
                return contrib;
            vec3 brdf_contrib = si.brdf->eval(wi, wo, sampler);
            #if defined(USE_MIS)
            if (light->is_dirac()) {
                contrib += brdf_contrib * ls.emission / ls.pdf;
            }
            else {
                Float brdf_pdf = si.brdf->pdf(wi, wo);
                Float weight = power_heuristic(ls.pdf, brdf_pdf);
                contrib += weight * brdf_contrib * ls.emission / ls.pdf;
            }
            #else
            Spectrum light_fac = ls.emission / ls.pdf;
            contrib += light_fac * brdf_contrib;
            assert(ls.pdf > 0.);
            #endif
        }

        #if defined(USE_MIS)
        if (!light->is_dirac()) {
            Brdf::Sample bs = si.brdf->sample(wi, sampler);
            Float weight = 1;

            //if (sample_not_specular) {
                Float light_pdf = light->pdf(si.pos,si.to_world(bs.wo));
                weight = power_heuristic(si.brdf->pdf(wi,bs.wo), light_pdf);
            //}

                vec3 emission = vec3(0.);
            contrib += bs.value * emission * weight;
        }
        #endif

        return contrib;
    }

    uint32_t n_sample;
};

class BrdfIntegrator : public Integrator {
public:
    BrdfIntegrator()
        : Integrator("BrdfIntegrator"),
        max_depth(10)
    {
        link_params();
    };

    Spectrum render_pixel_rec(Ray& r, Scene& scene, Sampler& sampler,
        const int& depth)
    {
        SurfaceInteraction si;

        Spectrum s(0.);

        if (scene.intersect(r, si)) {
            // Continue if there is no brdf
            if (!si.brdf) {
                r = Ray(si.pos + r.d * 0.00001f, r.d);
                return render_pixel_rec(r, scene, sampler, depth);
            }

            if (depth >= max_depth || si.brdf->is_emissive())
                return si.brdf->emission();

            // Compute BRDF  contrib
            vec3 wi = si.to_local(-r.d);
            if (wi.z < 0.000001)
                return s;

            Brdf::Sample bs = si.brdf->sample(wi, sampler);

            if (bs.wo.z < 0.000001)
                return s;

            #if !defined(SAMPLE_OPTIM)
            Float pdf = si.brdf->pdf(wi, bs.wo);
            Spectrum brdf_cos_weighted = si.brdf->eval(wi, bs.wo, sampler);
            assert(brdf_cos_weighted.x == brdf_cos_weighted.x);
            #endif

            Ray r_ = Ray(si.pos - r.d * 0.0001f, si.to_world(bs.wo));
            Spectrum indirect = render_pixel_rec(r_, scene, sampler, depth + 1);
            
            #if !defined(SAMPLE_OPTIM)
            s += brdf_cos_weighted * indirect / pdf;
            #else
            s += bs.value * indirect;
            #endif

            assert(s.x >= 0);
            assert(s.x == s.x);
            
        } else {
            for (const auto& light : scene.infinite_lights) {
                s += light->eval(r.d);
            }
        }

        return s;
    }

    Spectrum render_pixel(Ray& r, Scene& scene, Sampler& sampler)
    {
        return render_pixel_rec(r, scene, sampler, 0);
    }

    int max_depth;

protected:
    void link_params() { 
        params.add("max_depth", lt::Params::Type::INT, &max_depth);
    }
};

/**
 * @brief Direct lighting integrator class.
 */
class DirectIntegrator : public Integrator {
public:
    DirectIntegrator()
        : Integrator("DirectIntegrator")
    {
        link_params();
    };

    Spectrum render_pixel(Ray& r, Scene& scene, Sampler& sampler)
    {
        SurfaceInteraction si;

        Spectrum s(0.);

        if (scene.intersect(r, si)) {
            if (!si.brdf) {
                r = Ray(si.pos + r.d * 0.00001f, r.d);
                return render_pixel(r, scene, sampler);
            }

            if (si.brdf->is_emissive())
                return si.brdf->emission();

            s += uniform_sample_one_light(r, si, scene, sampler);
        } else {
            for (const auto& light : scene.infinite_lights)
                s += light->eval(r.d);
        }

        return s;
    }

protected:
    void link_params() { }
};

/**
 * @brief Path tracing integrator class.
 */
class PathIntegrator : public Integrator {
public:
    PathIntegrator()
        : Integrator("PathIntegrator")
        , max_depth(8)
    {
        link_params();
    };

    Spectrum render_pixel(Ray& r, Scene& scene, Sampler& sampler)
    {
        Spectrum throughput(1.);
        Spectrum s(0.);

        for (int d = 0; d < max_depth; d++) {
            
            SurfaceInteraction si;
            if (scene.intersect(r, si)) {


                if (!si.brdf) {
                    r = Ray(si.pos + r.d * 0.00001f, r.d);
                    d--;
                    continue;
                }

                if (d == 0 /* || specularBounce*/) {
                    s += throughput * si.brdf->emission();
                }
                
                // Compute Light contrib
                s += throughput * uniform_sample_one_light(r, si, scene, sampler);

                // Compute BRDF  contrib
                vec3 wi = si.to_local(-r.d);
                Brdf::Sample bs = si.brdf->sample(wi, sampler);

                if (bs.wo.z < 0.0001 || wi.z < 0.0001)
                    break;
                
                #if !defined(SAMPLE_OPTIM)
                Float wo_pdf = si.brdf->pdf(wi, bs.wo);
                Spectrum brdf_cos_weighted = si.brdf->eval(wi, bs.wo, sampler);
                throughput *= brdf_cos_weighted / wo_pdf;
                #else
                throughput *= bs.value;
                #endif

                // offset si.pos for next bounce
                vec3 p = si.pos - r.d * 0.00001f;
                r = Ray(p, si.to_world(bs.wo));

            } else {
                for (const auto& light : scene.infinite_lights)
                    s += throughput * light->eval(r.d);

                break;
            }
        }

        return s;
    }

    uint32_t max_depth; /**< Maximum depth of path tracing. */
protected:
    void link_params() 
    { 
        params.add("max_depth", Params::Type::INT, &max_depth);
    }
};

/**
 * @brief Ambient occlusion integrator class.
 */
class AOIntegrator : public Integrator {
public:
    AOIntegrator()
        : Integrator("AOIntegrator")
    {
        link_params();
    };

    Spectrum render_pixel(Ray& r, Scene& scene, Sampler& sampler)
    {
        SurfaceInteraction si;

        Spectrum s(0.);

        if (scene.intersect(r, si)) {
            Ray rs;
            rs.o = si.pos - 0.001f * r.d;

            int nSample = 1;

            for (int l = 0; l < nSample; l++) {
                vec3 wi = lt::square_to_uniform_hemisphere(sampler.next_float(),
                    sampler.next_float());

                rs.d = si.to_world(wi);

                if (!scene.shadow(rs))
                    s += Spectrum(1.) / (float(nSample));
            }
        }

        return s;
    }

protected:
    void link_params() { }
};


/**
 * @brief Direct lighting integrator class.
 */
class ReSTIRIntegrator : public Integrator {
    struct Sample {
        vec3 x; // path
        float W;// 1/pdf(x)
    };

    struct Reservoir {
       
        Sample s;
        float w_sum;
        /**
         * @brief Add sample to the reservoir.
         * @param x Input sample.
         * @param w Sampel weight.
         * @param u Uniform random variable.
         */
        void add_sample(const Sample& x, const float w, const float u) {
            w_sum += w;
            if (u < w / w_sum)
                s = x;
        }
    };

    int light_samples;
    int brdf_samples;

    Texture<Reservoir> reservoirs;
    void generateSample() {
        Reservoir r;
        for (int i = 0; i < light_samples; i++) {
            Light::Sample s = uniform_sample_one_light();
        }
        
    }
public:
    ReSTIRIntegrator()
        : Integrator("ReSTIRIntegrator")
    {
        link_params();
    };

    Spectrum render_pixel(Ray& r, Scene& scene, Sampler& sampler)
    {
        SurfaceInteraction si;

        Spectrum s(0.);

        if (scene.intersect(r, si)) {
            if (!si.brdf) {
                r = Ray(si.pos + r.d * 0.00001f, r.d);
                return render_pixel(r, scene, sampler);
            }

            if (si.brdf->is_emissive())
                return si.brdf->emission();

            s += uniform_sample_one_light(r, si, scene, sampler);
        }
        else {
            for (const auto& light : scene.infinite_lights)
                s += light->eval(r.d);
        }

        return s;
    }

protected:
    void link_params() { }
};


} // namespace LT_NAMESPACE