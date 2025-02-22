/**
 * @file
 * @brief Definitions of the Geometry, Mesh, and Sphere classes.
 */

#pragma once

#include <embree3/rtcore.h>
#include <fast_obj/fast_obj.h>
#include <lt/brdf_common.h>
#include <lt/lt_common.h>
#include <lt/ray.h>

namespace LT_NAMESPACE {

/**
 * @brief Abstract base class for geometric objects in the scene.
 */
class Geometry : public Serializable {
public:
    /**
     * @brief Constructor for Geometry.
     * @param type The type of geometry.
     */
    Geometry(const std::string& type)
        : Serializable(type) 
    {
        local_to_world = glm::mat4(1.);
        local_to_world[0][0] = 1;
        local_to_world[1][1] = 1;
        local_to_world[2][2] = 1;
    };

    /**
     * @brief Pure virtual function for computing the normal at a hit position.
     * @param rayhit Information about the ray hit.
     * @param hit_pos The position of the hit.
     * @return The normal vector at the hit position.
     */
    virtual vec3 get_normal(RTCRayHit rayhit, const vec3& hit_pos) = 0;

    virtual vec2 get_uv(RTCRayHit rayhit, const vec3& hit_pos) = 0;

    /**
     * @brief Pure virtual function for initializing Embree RTC geometry.
     * @param device The Embree RTC device.
     */
    virtual void init_rtc(RTCDevice device) = 0;

    virtual Bbox bbox() = 0;

    std::shared_ptr<Brdf>
        brdf; /**< Pointer to the BRDF associated with the geometry. */

    RTCGeometry rtc_geom; /**< Embree RTC geometry. */
    int rtc_id;
    glm::mat4 local_to_world;

};

/**
 * @brief Class representing a triangle mesh geometry.
 */
class TriangleMesh : public Geometry {
public:
    /**
     * @brief Default constructor for Mesh.
     * Initializes the geometry type and links parameters.
     */
    TriangleMesh(const std::string& type)
        : Geometry(type)
    {
    };

    /**
     * @brief Initialize the mesh geometry.
     */
    virtual void init() = 0;

    Bbox bbox() {
        Bbox b(vertex[0]);
        for (auto v : vertex)
            b.grow(v);
        return b;
    }

    /**
     * @brief Initialize the Embree RTC geometry for the mesh.
     * @param device The Embree RTC device.
     */
    void init_rtc(RTCDevice device)
    {
        rtc_geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

        float* vb = (float*)rtcSetNewGeometryBuffer(
            rtc_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
            3 * sizeof(float), vertex.size());
        for (int i = 0; i < vertex.size(); i++) {
            vb[3 * i] = vertex[i].x;
            vb[3 * i + 1] = vertex[i].y;
            vb[3 * i + 2] = vertex[i].z;
        }

        unsigned* ib = (unsigned*)rtcSetNewGeometryBuffer(
            rtc_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            3 * sizeof(unsigned), triangle_indices.size());
        for (int i = 0; i < triangle_indices.size(); i++) {
            ib[3 * i] = triangle_indices[i].x;
            ib[3 * i + 1] = triangle_indices[i].y;
            ib[3 * i + 2] = triangle_indices[i].z;
        }

    }

    /**
     * @brief Compute the normal at the hit position using barycentric
     * interpolation.
     * @param rayhit Information about the ray hit.
     * @param hit_pos The position of the hit.
     * @return The interpolated normal vector at the hit position.
     */
    vec3 get_normal(RTCRayHit rayhit, const vec3& hit_pos)
    {
        unsigned int prim_id = rayhit.hit.primID;
        glm::uvec3 face_nor = triangle_indices[prim_id];

        vec3 n1 = normal[face_nor.x];
        vec3 n2 = normal[face_nor.y];
        vec3 n3 = normal[face_nor.z];

        return glm::normalize(n2 * rayhit.hit.u + n3 * rayhit.hit.v + n1 * (1 - rayhit.hit.u - rayhit.hit.v));
    }

    vec2 get_uv(RTCRayHit rayhit, const vec3& hit_pos)
    {
        unsigned int prim_id = rayhit.hit.primID;
        glm::uvec3 indices = triangle_indices[prim_id];

        vec2 uv1 = uv[indices.x];
        vec2 uv2 = uv[indices.y];
        vec2 uv3 = uv[indices.z];

        return uv2 * rayhit.hit.u + uv3 * rayhit.hit.v + uv1 * (1 - rayhit.hit.u - rayhit.hit.v);
    }
    
    std::vector<vec3> normal; /**< Vertex normals. */
    std::vector<vec3> vertex; /**< Vertex positions. */
    std::vector<glm::uvec3> triangle_indices; /**< Indices of triangle vertices. */
    std::vector<vec2> uv; /**< Vertex UV. */

};

/**
 * @brief Class representing a triangle mesh geometry.
 */
class Mesh : public TriangleMesh {
public:
    /**
     * @brief Default constructor for Mesh.
     * Initializes the geometry type and links parameters.
     */
    Mesh()
        : TriangleMesh("Mesh")
    {
        link_params();
    };

    /**
     * @brief Initialize the mesh geometry by parsing an OBJ file.
     */
    void init()
    {
        // Parse obj
        fastObjMesh* fobj = fast_obj_read(filename.c_str());
        if (!fobj) {
            Log(logError) << "Could not read : " << filename;
            return;
        }

        glm::mat4 inv_tra_local_to_world = glm::inverse(glm::transpose(local_to_world));

        std::map<std::pair<uint32_t, uint32_t>, uint32_t> exist;
        for (int i = 0; i < fobj->face_count; i++) {
            glm::uvec3 idx;

            for (int j = 0; j < 3; j++) {
                int iv = fobj->indices[3 * i + j].p;
                int in = fobj->indices[3 * i + j].n;

                std::map<std::pair<uint32_t, uint32_t>, uint32_t>::iterator iter = exist.find({ iv, in });
                if (iter != exist.end()) {
                    idx[j] = iter->second;
                }
                else {
                    idx[j] = vertex.size();
                    exist[{ iv, in }] = vertex.size();
                    glm::vec4 local_vertex = glm::vec4(fobj->positions[3 * iv],
                        fobj->positions[3 * iv + 1],
                        fobj->positions[3 * iv + 2],
                        1);
                    glm::vec4 local_normal = glm::vec4(fobj->normals[3 * in],
                        fobj->normals[3 * in + 1],
                        fobj->normals[3 * in + 2],
                        0);
                    vertex.push_back(vec3(local_to_world * local_vertex));
                    normal.push_back(vec3(inv_tra_local_to_world * local_normal));

                    uv.push_back(vec2(fobj->texcoords[2 * in], fobj->texcoords[2 * in + 1]));
                }
            }

            triangle_indices.push_back(idx);
        }

        fast_obj_destroy(fobj);
    };


    std::string filename; /**< Filename of the OBJ file. */
   
protected:
    /**
     * @brief Link parameters with the Params struct.
     */
    void link_params()
    {
        params.add("filename", ParamType::PATH, &filename);
        params.add("brdf", ParamType::BRDF, &brdf);
        params.add("local_to_world", ParamType::MAT4, &local_to_world);
    }
};




/**
 * @brief Class representing a Rectangle geometry.
 */
class Rectangle : public TriangleMesh {
public:
    Rectangle()
        : TriangleMesh("Rectangle")
    {
        link_params();

        this->brdf = std::shared_ptr<Brdf>(nullptr);
    };

    /**
     * @brief Initialize the sphere geometry.
     */
    void init() {
        glm::mat4 inv_tra_local_to_world = glm::inverse(glm::transpose(local_to_world));
        normal.push_back(vec3(inv_tra_local_to_world * glm::vec4(0, 0, 1, 0)));
        normal.push_back(vec3(inv_tra_local_to_world * glm::vec4(0, 0, 1, 0)));
        normal.push_back(vec3(inv_tra_local_to_world * glm::vec4(0, 0, 1, 0)));
        normal.push_back(vec3(inv_tra_local_to_world * glm::vec4(0, 0, 1, 0)));
        vertex.push_back(vec3(local_to_world * glm::vec4(1, 1, 0, 1)));
        vertex.push_back(vec3(local_to_world * glm::vec4(1, -1, 0, 1)));
        vertex.push_back(vec3(local_to_world * glm::vec4(-1, -1, 0, 1)));
        vertex.push_back(vec3(local_to_world * glm::vec4(-1, 1, 0, 1)));
        uv.push_back(vec2(1., 1.));
        uv.push_back(vec2(1., 0.));
        uv.push_back(vec2(0., 0.));
        uv.push_back(vec2(0., 1.));
        triangle_indices.push_back(glm::uvec3(0, 1, 2));
        triangle_indices.push_back(glm::uvec3(0, 2, 3));
    };


protected:
    /**
     * @brief Link parameters with the Params struct.
     */
    void link_params()
    {
        params.add("brdf", ParamType::BRDF, &brdf);
        params.add("local_to_world", ParamType::MAT4, &local_to_world);
    }
};



/**
 * @brief Class representing a sphere geometry.
 */
class Sphere : public Geometry {
public:
    /**
     * @brief Default constructor for Sphere.
     * Initializes the geometry type, position, and radius.
     */
    Sphere()
        : Geometry("Sphere")
        , pos(vec3(0.))
        , rad(1.)
    {
        link_params();

        this->brdf = std::shared_ptr<Brdf>(nullptr);
    };

    /**
     * @brief Initialize the sphere geometry.
     */
    void init() {};

    Bbox bbox() {
        Bbox b(pos);
        b.grow(pos + rad);
        b.grow(pos - rad);
        return b;
    }

    /**
     * @brief Initialize the Embree RTC geometry for the sphere.
     * @param device The Embree RTC device.
     */
    void init_rtc(RTCDevice device)
    {
        // Set embree
        rtc_geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);

        float* vb = (float*)rtcSetNewGeometryBuffer(
            rtc_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4,
            4 * sizeof(float), 1);

        vb[0] = pos.x;
        vb[1] = pos.y;
        vb[2] = pos.z;
        vb[3] = rad;
    }

    /**
     * @brief Compute the normal at the hit position for the sphere.
     * @param rayhit Information about the ray hit.
     * @param hit_pos The position of the hit.
     * @return The normal vector at the hit position.
     */
    vec3 get_normal(RTCRayHit rayhit, const vec3& hit_pos)
    {
        return (hit_pos - pos) / rad;
    }

    vec2 get_uv(RTCRayHit rayhit, const vec3& hit_pos)
    {
        vec3 n = (hit_pos - pos) / rad;
        float u = glm::acos(n.y) / pi;
        float v = (glm::atan(n.z,n.x) + pi * 0.5) / pi;
        return vec2(u, v);
    }

    vec3 pos; /**< Center position of the sphere. */
    float rad; /**< Radius of the sphere. */

protected:
    /**
     * @brief Link parameters with the Params struct.
     */
    void link_params()
    {
        params.add("pos", ParamType::VEC3, &pos);
        params.add("rad", ParamType::FLOAT, &rad);
        params.add("brdf", ParamType::BRDF, &brdf);
        params.add("local_to_world", ParamType::MAT4, &local_to_world);
    }
};


} // namespace LT_NAMESPACE
