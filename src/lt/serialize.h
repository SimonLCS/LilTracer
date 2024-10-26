/**
 * @file
 * @brief Definition of the Serializable class.
 */

#pragma once
#include <lt/lt_common.h>
#include <lt/params.h>

namespace LT_NAMESPACE {

/**
 * @brief Interface class for serializable objects.
 */
class Serializable {
private:
    static int item_count; /**< Static member to count the number of serializable
                              objects. */
public:
    /**
     * @brief Constructor for Serializable.
     * @param type The type of the serializable object.
     * Initializes the type and assigns an id to the serializable object.
     * Increment item_count
     */
    Serializable(const std::string& type)
        : type(type)
    {
        id = item_count;
        item_count++;
    };

    /**
     * @brief Virtual function for initializing the serializable object.
     */
    virtual void init() {};

    /**
     * @brief Apply modification when parameters are changed.
     */
    virtual void on_changes() { init(); }

    int id; /**< Unique identifier for the serializable object. */
    Params params; /**< Parameters associated with the serializable object. */
    std::string type; /**< Type of the serializable object. */
protected:
    /**
     * @brief Pure virtual function for linking parameters.
     * Subclasses must implement this function to link parameters \ref params with
     * the serializable object.
     */
    virtual void link_params() = 0;

};

} // namespace LT_NAMESPACE