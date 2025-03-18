/**
 * @file sensor.h
 * @brief Defines the Sensor class for handling sensor data.
 */

#pragma once
#include <lt/lt_common.h>
#include <lt/serialize.h>
#include <lt/factory.h>
#include <atomic>

#include <atomic>

namespace LT_NAMESPACE {

/**
 * @brief Class for handling sensor data.
 *
 * The Sensor class represents a sensor used to capture light data in a scene.
 * It provides functionality for resetting the sensor, adding samples, setting
 * samples, and retrieving the number of samples at a specific pixel.
 */
class Sensor : public Serializable {
public:
    /**
	 * @brief Default constructor.
     */
    Sensor(const std::string& type = "Sensor", const uint32_t& w = 0, const uint32_t& h = 0)
        : Serializable(type)
        , w(w)
        , h(h)
    {
        link_params();
    }

    /**
     * @brief Parameterized constructor.
     * @param w Width of the sensor.
     * @param h Height of the sensor.
     */
    Sensor(const uint32_t& w, const uint32_t& h)
        : Serializable("Sensor")
        , w(w)
        , h(h)
    {
        link_params();
    }

    void init();

    /**
     * @brief Resets the sensor data.
     *
     * This function resets the accumulator, value, and count arrays of the
     * sensor.
     */
    virtual void reset();

    /**
     * @brief Adds a sample to the sensor data.
     *
     * @param x The x-coordinate of the sample.
     * @param y The y-coordinate of the sample.
     * @param s The spectrum of the sample.
     */
    virtual void add(const uint32_t& x, const uint32_t& y, Spectrum s);

    /**
     * @brief Sets a sample in the sensor data.
     *
     * @param x The x-coordinate of the sample.
     * @param y The y-coordinate of the sample.
     * @param s The spectrum of the sample.
     */
    virtual void set(const uint32_t& x, const uint32_t& y, Spectrum s);

    virtual Spectrum get(const uint32_t& x, const uint32_t& y);

    /**
     * @brief Gets the number of samples at a specific pixel.
     *
     * @param x The x-coordinate of the pixel.
     * @param y The y-coordinate of the pixel.
     * @return The number of samples at the specified pixel.
     */
    uint16_t n_sample(const uint32_t& x, const uint32_t& y);

    virtual void set_value(const uint32_t& idx, const uint32_t& x);

    uint32_t w; /**< Width of the sensor. */
    uint32_t h; /**< Height of the sensor. */
    std::vector<Spectrum> acculumator; /**< Accumulator array for sensor samples. */
    std::vector<Spectrum> value; /**< Value array for sensor samples. (accumulator[i] / count[i]) */
    std::vector<uint16_t> count; /**< Count array for the number of samples at each pixel. */
    std::atomic<uint32_t> sum_counts;
    std::vector<Float> u; /**< Vector representing the u-coordinates of the sensor pixels. */
    std::vector<Float> v; /**< Vector representing the v-coordinates of the sensor pixels. */


protected:
    void link_params()
    {
        params.add("width", &w);
        params.add("height", &h);
    }
};

class VarianceSensor : public Sensor
{
public:

    VarianceSensor()
        : Sensor("Variance")
    {};

    VarianceSensor(const uint32_t& w, const uint32_t& h)
        : Sensor("Variance", w, h)
    {};

    void init() {
        Sensor::init();
        acculumator_sqr.resize(w * h);
    }
    
    void reset() {
        Sensor::reset();
        memset(acculumator_sqr.data(), 0, sizeof(Spectrum) * acculumator_sqr.size());
    }
    
    void add(const uint32_t& x, const uint32_t& y, Spectrum s) 
    {
        uint32_t idx = y * w + x;
        acculumator[idx] += s;
        acculumator_sqr[idx] += s*s;
        count[idx]++;
        sum_counts++;
        set_value(idx, y);
    }

    void set(const uint32_t& x, const uint32_t& y, Spectrum s)
    {
        uint32_t idx = y * w + x;
        acculumator[idx] = s;
        acculumator_sqr[idx] = s*s;
        count[idx] = 1;
        set_value(idx, y);
    }

    void set_value(const uint32_t& idx, const uint32_t& y)
    {
        Spectrum mean = acculumator[idx] / (Float)count[idx];
        value[idx] = acculumator_sqr[idx] / (Float)count[idx] - mean * mean;
        value[idx] = value[idx] / ((Float)count[idx] - 1);
        assert(value[idx] == value[idx]);
    }

    void use_variance(const bool& mode ) {
        if (mode) {
            for (int i = 0; i < acculumator.size(); i++)
                set_value(i, 0);
        }
        else {
            for (int i = 0; i < acculumator.size(); i++)
                value[i] = acculumator[i] / (Float)count[i];
        }
    }

    std::vector<Spectrum> acculumator_sqr;
    

};


class HemisphereSensor : public Sensor
{
public:

    HemisphereSensor() 
        : Sensor("HemisphereSensor")
        , dtheta(-1.)
        , dphi(-1.)
    {};

    HemisphereSensor(const uint32_t& w, const uint32_t& h) 
        : Sensor("HemisphereSensor", w, h) 
    {
        init();
    };

    void init();
    void set_value(const uint32_t& idx, const uint32_t& y);

    std::vector<Float> solid_angle; /**< Vector representing the u-coordinates of the sensor pixels. */
    Float dtheta;
    Float dphi;

};


} // namespace LT_NAMESPACE