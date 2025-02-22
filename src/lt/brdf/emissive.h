/**
 * @file brdf.h
 * @brief Defines classes related to Bidirectional Reflectance Distribution
 * Functions (BRDFs).
 */

#pragma once

#include "brdf.h"

namespace LT_NAMESPACE {

class Emissive : public Brdf {
public:
    SpectrumTex intensity; /**< Intensity of emission. */

    Emissive()
        : Brdf("Emissive")
        , intensity(Spectrum(1.))
    {
        flags = Flags::emissive;
        link_params();
    }

    Spectrum emission();

protected:
    void link_params()
    {
        params.add("intensity", ParamType::RGB, &intensity);
    }
};


} // namespace LT_NAMESPACE