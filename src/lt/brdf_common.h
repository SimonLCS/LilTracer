#pragma once

#include <lt/factory.h>
#include <lt/lt_common.h>

#include <lt/brdf/brdf.h>
#include <lt/brdf/lambert.h>
#include <lt/brdf/emissive.h>
#include <lt/brdf/test.h>
#include <lt/brdf/ggx.h>
#include <lt/brdf/beckmann.h>
#include <lt/brdf/mix.h>
#include <lt/brdf/micrograin.h>

#include <lt/sensor.h>

namespace LT_NAMESPACE {


    struct BrdfValidation {
        static int number_of_sample;
        static int number_of_theta;

        std::vector<Float> thetas;
        std::vector<Float> directional_albedo;
        std::vector<Float> sampling_difference;
        bool energy_conservative; // all  directionnal_albedo <  1
        bool correct_sampling;    // sample = pdf
        bool reciprocity;
        bool found_nan;
        bool negative_value;

        BrdfValidation() :
            energy_conservative(false),
            correct_sampling(false),
            reciprocity(true),
            found_nan(false),
            negative_value(false)
        {}

        static BrdfValidation validate(Brdf& brdf) {
            Log(logInfo) << "validate " << brdf.type;// << std::endl;

            SurfaceInteraction si;
            BrdfValidation validation;
            validation.directional_albedo.resize(number_of_theta);
            validation.sampling_difference.resize(number_of_theta);
            validation.thetas = lt::linspace<float>(0, 0.5 * lt::pi, number_of_theta);
            validation.energy_conservative = true;

            // wo Buffer resolution
            const int res_theta_sampling = 64;
            const int res_phi_sampling = 4 * res_theta_sampling;
            const float dtheta = 0.5 * lt::pi / res_theta_sampling;
            const float dphi = 2. * lt::pi / res_phi_sampling;


            // Loop over all incident angles
            for (int i = 0; i < number_of_theta; i++) {
                
                // Initiate data
                Float theta_i = validation.thetas[i];
                Float phi_i = 0.;
                lt::vec3 wi = lt::polar_to_card(theta_i, phi_i);
            
                
                std::vector<float> th = lt::linspace<float>(0, 0.5 * lt::pi, res_theta_sampling);
                std::vector<float> ph = lt::linspace<float>(0, 2. * lt::pi, res_phi_sampling);

                HemisphereSensor accu_buf(res_phi_sampling, res_theta_sampling);
                accu_buf.init();

                Sensor pdf_buf(res_phi_sampling, res_theta_sampling);
                pdf_buf.init();

                Sensor diff_buf(res_phi_sampling, res_theta_sampling);
                diff_buf.init();


                Sampler sampler;

                Float directional_albedo = Float(0.);
                
                // Check if there is negative values
                // Test specific directions to found nan
                // wi = wo
                // Test reciprocity
                // Test sampling


                //Validate the energy
                for (int j = 0; j < number_of_sample; j++) {
                    Brdf::Sample sample = brdf.sample(wi, si, sampler);
                    vec3 wo = sample.wo;
                    float phi = std::atan2(wo.y, wo.x);
                    phi = phi < 0 ? 2 * lt::pi + phi : phi;
                    float x = phi / (2. * lt::pi) * (float)res_phi_sampling;
                    float y = std::acos(wo.z) / (0.5 * lt::pi) * (float)res_theta_sampling;
                    if (y < res_theta_sampling){
                        accu_buf.add(int(x), int(y), lt::Spectrum(1));
                        directional_albedo += sample.value.x;
                    }
                    else{
                        accu_buf.sum_counts++;
                    }
                }
                
                directional_albedo /= number_of_sample;
                // Check if brdf is energy conservative
                if (directional_albedo > 1.)
                    validation.energy_conservative = false;


                //Validate the sampling
                for (int j = 0; j < number_of_sample; j++) {
                    lt::vec3 wo = lt::square_to_cosine_hemisphere(sampler.next_float(), sampler.next_float());
                    float phi = std::atan2(wo.y, wo.x);
                    phi = phi < 0 ? 2 * lt::pi + phi : phi;
                    int x = int(phi / (2. * lt::pi) * (float)res_phi_sampling);
                    int y = int(std::acos(wo.z) / (0.5 * lt::pi) * (float)res_theta_sampling);

                    if (y < res_theta_sampling) {
                        pdf_buf.add(x, y, lt::Spectrum(brdf.pdf(wi, wo, si)));
                        diff_buf.set(x, y, (pdf_buf.get(x, y) - accu_buf.get(x, y)));
                    }
                    else {
                        pdf_buf.sum_counts++;
                        diff_buf.sum_counts++;
                    }
                }


                float sum_diff = 0.;
                for (int x = 0; x < res_phi_sampling; x++) {
                    for (int y = 0; y < res_theta_sampling; y++) {
                        lt::vec3 wo = lt::polar_to_card(th[y], ph[x] );
                        float theta = 0.5 * lt::pi * (float(y) + 0.5) / res_theta_sampling;
                        sum_diff += diff_buf.get(x, y).x * std::sin(theta) * dtheta * dphi;
                    }
                }

                float T = 0;
                for (int y = 0; y < res_theta_sampling; y++) {
                    for (int x = 0; x < res_phi_sampling; x++) {
                        float approx_pdf = accu_buf.get(x, y).x;
                        float pdf        = pdf_buf.get(x, y).x;
                        if(pdf > 0.)
                            T += (approx_pdf - pdf) * (approx_pdf - pdf) / pdf ;
                    }
                    std::cout << std::endl;
                }
                int Dof = res_theta_sampling * res_phi_sampling - 1;
                float chi2 = chisqr(Dof, T);
                
                validation.correct_sampling = false;
                
                validation.sampling_difference[i] = chi2;

                validation.directional_albedo[i] = directional_albedo;
            }


            return validation;
        }
    };


} // end  namespace LT_NAMESPACE