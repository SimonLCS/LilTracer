/**
 * @file
 * @brief Definition of the Params class.
 */

#pragma once
#include <lt/lt_common.h>

namespace LT_NAMESPACE {

/**
* @brief Enum defining the types of parameters.
*/
enum class ParamType {
    BOOL, /**< BOOL type parameter. */
    FLOAT, /**< Float type parameter. */
    INT, /**< Int type parameter. */
    VEC3, /**< Vector3 type parameter. */
    MAT4, /**< Matrix 4x4 type parameter . */
    IOR, /**< Eta and Kappa parameters. */
    RGB, /**< RGB Vector3 parameters. */
    SH, /**< Spherical Harmonics type parameter. */
    PATH, /**< Path type parameter. */
    BRDF, /**< BRDF type parameter. */
    TEXTURE, /**< Texture type parameter. */
    NONE
};

struct Param {
    void* ptr = nullptr; /**< Pointer to the parameter value. */
    void* min = nullptr; /**< Pointer to the minimum value. (can be nullptr is not assigned) */
    void* max = nullptr; /**< Pointer to the maximum value. (can be nullptr is not assigned) */
    ParamType type = ParamType::NONE; /**< Type of the parameter. */
    std::string name = "NONE"; /**< Name of the parameter. */
};

/**
 * @brief Struct for storing parameters.
 */
struct Params {

    std::vector<Param> list;
    int count = 0; 

    /**
     * @brief Add a parameter to the Params struct.
     * @param name The name of the parameter.
     * @param type The type of the parameter.
     * @param ptr A pointer to the parameter value.
     */
    template<typename T>
    void add(const std::string& name, ParamType type, T* ptr, T* min = (T*)nullptr, T* max = (T*)nullptr)
    {
        count++;
        Param p;
        p.name = name;
        p.type = type;
        p.ptr = (void*)ptr;
        p.min = (void*)min;
        p.max = (void*)max;
        list.push_back(p);
    }

};

} // namespace LT_NAMESPACE