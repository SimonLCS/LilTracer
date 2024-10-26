/**
 * @file
 * @brief Definition of the Renderer class.
 */

#pragma once
#include <lt/camera.h>
#include <lt/integrator.h>
#include <lt/lt_common.h>
#include <lt/sampler.h>
#include <lt/scene.h>
#include <lt/sensor.h>

#include <thread>
#include <atomic>

namespace LT_NAMESPACE {

// struct RendererDesc {
//     std::shared_ptr<Sampler> sampler = nullptr;
//     std::shared_ptr<Sensor> sensor = nullptr;
//     std::shared_ptr<Camera> camera = nullptr;
//     std::shared_ptr<Integrator> Integrator = nullptr;
//     bool async = false;
//     int max_render_pass = 1;
// };


/**
 * @brief Class for managing rendering process.
 */
class Renderer {
public:
    std::shared_ptr<Sampler> sampler; /**< Pointer to the sampler. */
    std::shared_ptr<Sensor> sensor; /**< Pointer to the sensor. */
    std::shared_ptr<Camera> camera; /**< Pointer to the camera. */
    std::shared_ptr<Integrator> integrator; /**< Pointer to the integrator. */
    int max_sample;

    Renderer() : max_sample(1) {}

    float render(Scene& scene)
    {
        return integrator->render(camera, sensor, scene, *sampler);
    }

    void reset() { sensor->reset(); }
};

/**
 * @brief Class for managing async rendering process.
 */
class RendererAsync : public Renderer {
public:
    std::thread thr;
    bool need_reset;
    std::atomic<bool> done;
    bool start;
    float delta_time_ms;

    RendererAsync() { need_reset = false; done = true; start = true; delta_time_ms = 0.; }

    ~RendererAsync()
    {

        try {
            if (thr.joinable()) {
                thr.join();
            }
        } catch (std::exception& ex) {
            Log(logError) << ex.what();
        }
    }

    void reset() { need_reset = true; }

    bool render(Scene& scene)
    {
        if (!done) {
            return false;
        } else {
            done = false;
            if (start)
                start = false;
            else
                thr.join();

            if (need_reset) {
                Renderer::reset();
                need_reset = false;
            }
            
            thr = std::thread(
                [&](auto s) {
                    delta_time_ms = Renderer::render(s);
                    done = true;
                },
                scene);
            
            return true;
        }
    }
};

} // namespace LT_NAMESPACE