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
    UINT, /**< Unsigned Int 32 type parameter. */
    FLOAT, /**< Float type parameter. */
    VEC3, /**< Vector3 type parameter. */
    MAT4, /**< Matrix 4x4 type parameter . */
    PATH, /**< Path type parameter. */
    FLOAT_TEX, /**< FloatTex type parameter. */
    SPECTRUM_TEX, /**< FloatTex type parameter. */
    IOR, /**< Eta and Kappa parameters. */
    RGB, /**< RGB Vector3 parameters. */
    TEXTURE, /**< Texture type parameter. */
    BRDF, /**< BRDF type parameter. */
    NONE
};

struct Param {
    void* ptr = nullptr; /**< Pointer to the parameter value. */
    void* min = nullptr; /**< Pointer to the minimum value. (can be nullptr is not assigned) */
    void* max = nullptr; /**< Pointer to the maximum value. (can be nullptr is not assigned) */
    ParamType type = ParamType::NONE; /**< Type of the parameter. */
    std::string name = "NONE"; /**< Name of the parameter. */
};

template<typename T>
struct Texture;


class Brdf;


/**
 * @brief Struct for storing parameters.
 */
struct Params {

    std::vector<Param> list;
    int count = 0; 

#define PTR(x) x
#define TYPE(x) constexpr (std::is_same_v<T, PTR(x)> )

    template<typename T>
    constexpr static ParamType getParamType() {
        if      TYPE(bool) return ParamType::BOOL;
        else if TYPE(uint32_t) return ParamType::UINT;
        else if TYPE(float) return ParamType::FLOAT;
        else if TYPE(vec3) return ParamType::VEC3;
        else if TYPE(glm::mat4) return ParamType::MAT4;
        else if TYPE(std::string) return ParamType::PATH;
        else if TYPE(std::shared_ptr<Texture<float>>) return ParamType::FLOAT_TEX;
        else if TYPE(std::shared_ptr<Texture<Spectrum>>) return ParamType::SPECTRUM_TEX;
        else if TYPE(vec3) return ParamType::IOR;
        else if TYPE(vec3) return ParamType::RGB;
        else if TYPE(std::shared_ptr<Texture<Spectrum>>) return ParamType::TEXTURE;
        else if TYPE(std::shared_ptr<Brdf>) return ParamType::BRDF;
        else return ParamType::NONE;
    }

    /**
     * @brief Add a parameter to the Params struct.
     * @param name The name of the parameter.
     * @param type The type of the parameter.
     * @param ptr A pointer to the parameter value.
     */
    template<typename T>
    void add(const std::string& name, T* ptr, T* min = (T*)nullptr, T* max = (T*)nullptr)
    {
        static_assert(getParamType<T>() != ParamType::NONE, "T is not a valid parameter type!");

        count++;
        Param p;
        p.name = name;
        p.type = getParamType<T>();
        p.ptr = static_cast<void*>(ptr);
        p.min = static_cast<void*>(min);
        p.max = static_cast<void*>(max);
        list.push_back(p);
    }

};

} // namespace LT_NAMESPACE