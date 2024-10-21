#include <iostream>
#include <lt/lt.h>

#include <glm/glm.hpp>

#ifdef __linux__ 
#include <GL/glew.h>
#else
#include <gl/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <lil_tracer_theme.h>

#include "lil_gl.h"

std::string exec_path;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool need_reset;
#define NEED_RESET(x) if(x){ need_reset = true; }

struct RenderSensor {

    std::shared_ptr<lt::Sensor> sensor = nullptr;
    bool initialized = false;

    enum Type
    {
        Spectrum = 0,
        Colormap = 1
    };

    // OpenGL stuff
    GLuint sensor_id;
    GLuint render_id;
    GLuint render_fb_id;
    //GLuint render_rb_id;
    GLuint quadVAO, quadVBO;
    GLuint shader_id;

    Type type;

    bool update_data() {

        if (!initialized) {
            return false;
        }
        // Push sensor data in opengl sensor texture
        glBindTexture(GL_TEXTURE_2D, sensor_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sensor->w, sensor->h, 0, GL_RGB, GL_FLOAT, sensor->value.data());

        // Process sensor
        // Apply tonemapping

        glBindFramebuffer(GL_FRAMEBUFFER, render_fb_id);
        glViewport(0, 0, sensor->w, sensor->h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT); // we're not using the stencil buffer now
        glDisable(GL_DEPTH_TEST);

        glUseProgram(shader_id);
        glUniform1i(glGetUniformLocation(shader_id, "type"), (int)type);
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, sensor_id);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return true;
    }

    bool initialize()
    {
        // Setup sensor OpenGL texture
        glGenTextures(1, &sensor_id);
        glBindTexture(GL_TEXTURE_2D, sensor_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Setup framebuffer for post process
        glGenFramebuffers(1, &render_fb_id);
        glBindFramebuffer(GL_FRAMEBUFFER, render_fb_id);

        glGenTextures(1, &render_id);
        glBindTexture(GL_TEXTURE_2D, render_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sensor->w, sensor->h, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_id, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        const char* vShaderCode = " #version 330 core\n"
            "layout(location = 0) in vec2 aPos;\n"
            "layout(location = 1) in vec2 aTexCoords;\n"
            "out vec2 TexCoords;\n"
            "void main()\n"

            "{\n"
            "    TexCoords = aTexCoords;\n"
            "    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
            "}";

        // https://www.shadertoy.com/view/Nd3fR2
        const char* fShaderCode = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec2 TexCoords;\n"
            "uniform sampler2D screenTexture;\n"
            "uniform int type;\n"
            "vec3 coolwarm(float t) {const vec3 c0 = vec3(0.227376,0.286898,0.752999); const vec3 c1 = vec3(1.204846,2.314886,1.563499); const vec3 c2 = vec3(0.102341,-7.369214,-1.860252);    const vec3 c3 = vec3(2.218624,32.578457,-1.643751);const vec3 c4 = vec3(-5.076863,-75.374676,-3.704589);const vec3 c5 = vec3(1.336276,73.453060,9.595678);const vec3 c6 = vec3(0.694723,-25.863102,-4.558659);return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));}\n"
            "void main()\n"
            "{\n"
            "   vec3 col = texture(screenTexture, TexCoords).rgb;\n"
            "   if(type == 0)\n"
            "       FragColor = vec4(pow(col,vec3(0.4545)), 1.0);\n"
            "   else\n"
            "       FragColor = vec4(vec3(coolwarm(clamp(col.x,-0.5,0.5)+0.5)), 1.0);\n"
            "}";

        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        // shader Program
        shader_id = glCreateProgram();
        glAttachShader(shader_id, vertex);
        glAttachShader(shader_id, fragment);
        glLinkProgram(shader_id);

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        initialized = true;
        return true;
    }

    GLuint id() {;
        return render_id;
    }

    ~RenderSensor() {
        if (initialized) {
            glDeleteTextures(1, &sensor_id);
        }
    }
};


struct RenderableScene {
    lt::Scene    scn;
    lt::RendererAsync ren;
    RenderSensor rsen;
    bool pause = false;
    std::string path;
};

static OpenglScene opengl_scene;

void update_opengl_scene(std::shared_ptr<RenderableScene> r, int scn_idx) {
    if (scn_idx == opengl_scene.idx) {
        return;
    }
    opengl_scene.meshes.clear();
    opengl_scene.idx = scn_idx;

    for (std::shared_ptr<lt::Geometry> g : r->scn.geometries) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        GLuint render_mode = GL_TRIANGLES;
        if (g->type == "Mesh") {
            std::shared_ptr<lt::Mesh> m = std::reinterpret_pointer_cast<lt::Mesh>(g);
            vertices.resize(m->vertex.size());
            indices.resize(m->triangle_indices.size() * 3);
            for (int i = 0; i < m->vertex.size(); i++) {
                vertices[i].Position = m->vertex[i];
                vertices[i].Normal = m->normal[i];
            }
            for (int i = 0; i < m->triangle_indices.size(); i++) {
                indices[3*i]     = m->triangle_indices[i].x;
                indices[3 * i+1] = m->triangle_indices[i].y;
                indices[3 * i+2] = m->triangle_indices[i].z;
            }
            render_mode = GL_TRIANGLES;
        }
        else if (g->type == "Sphere") {
            std::shared_ptr<lt::Sphere> s = std::reinterpret_pointer_cast<lt::Sphere>(g);;
            solid_sphere(vertices, indices, s->rad, 10, 10);
            render_mode = GL_TRIANGLE_STRIP;
        }
        opengl_scene.meshes.push_back(std::make_shared<Mesh>(vertices, indices, render_mode));
    }
    lt::Ray start = r->ren.camera->generate_ray(0, 0);
    opengl_scene.setup();
    opengl_scene.pos = start.o;
    opengl_scene.dir = start.d;
    opengl_scene.up = glm::vec3(0., -1., 0.);
    opengl_scene.right = glm::normalize(glm::cross(opengl_scene.dir, opengl_scene.up));
    opengl_scene.up = glm::normalize(glm::cross(opengl_scene.right, opengl_scene.dir));
    opengl_scene.dist = 1.;
}

void draw_opengl_scene() {
    //opengl_scene.dist = ImGui::GetIO().MouseWheel;
    //opengl_scene.pos = ImGui::GetMouseDragDelta();
    glm::vec3 center = opengl_scene.pos + opengl_scene.dir * opengl_scene.dist;

    // Zoom
    float zoom_factor = ImGui::GetIO().MouseWheel * ImGui::GetIO().DeltaTime;
    opengl_scene.dist = glm::max(0.01f, opengl_scene.dist-zoom_factor);
    opengl_scene.pos = center - opengl_scene.dist * opengl_scene.dir;

    // Pan
    ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    float pan_factorX = drag.x * ImGui::GetIO().DeltaTime * 0.1;
    float pan_factorY = -drag.y * ImGui::GetIO().DeltaTime * 0.1;
    glm::vec3 panX = opengl_scene.right * pan_factorX;
    glm::vec3 panY = opengl_scene.up * pan_factorY;
    opengl_scene.pos += panX + panY;
    center += panX + panY;
    
    drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
    float angle_X_inc = drag.x * ImGui::GetIO().DeltaTime;
    float angle_Y_inc = drag.y * ImGui::GetIO().DeltaTime;

    glm::mat4 rot_mat_X = glm::rotate(glm::mat4(1.0f), glm::radians(angle_X_inc), opengl_scene.up);
    opengl_scene.dir = glm::vec3(glm::vec4(opengl_scene.dir, 1.0f) * rot_mat_X);
    opengl_scene.right = glm::vec3(glm::vec4(opengl_scene.right, 1.0f) * rot_mat_X);
   
    glm::mat4 rot_mat_Y = glm::rotate(glm::mat4(1.0f), glm::radians(angle_Y_inc), opengl_scene.right);
    opengl_scene.dir = glm::vec3(glm::vec4(opengl_scene.dir, 1.0f) * rot_mat_Y);
    opengl_scene.up = glm::vec3(glm::vec4(opengl_scene.up, 1.0f) * rot_mat_Y);

    opengl_scene.pos = center - opengl_scene.dist * opengl_scene.dir;

    opengl_scene.view = glm::lookAt(opengl_scene.pos, opengl_scene.pos + opengl_scene.dir * opengl_scene.dist, glm::vec3(0.,-1.,0.));
    opengl_scene.draw();
}

struct AppData {
    std::shared_ptr<lt::Sensor> s_brdf_slice;
    RenderSensor rs_brdf_slice;
    int current_brdf_idx;
    std::vector<std::shared_ptr<lt::Brdf>> brdfs;
    float theta_i = 0.5;
    float phi_i = 0.;

    int scn_idx;
    std::vector<std::shared_ptr<RenderableScene>> scenes;

    std::shared_ptr<lt::HemisphereSensor> s_brdf_sampling;
    RenderSensor rs_brdf_sampling;
    std::shared_ptr<lt::Sensor> s_brdf_sampling_pdf;
    RenderSensor rs_brdf_sampling_pdf;
    std::shared_ptr<lt::Sensor> s_brdf_sampling_diff;
    RenderSensor rs_brdf_sampling_diff;

    lt::Sampler sampler;

    lt::BrdfValidation validation;


    void init() {
        need_reset = false;

        brdfs.push_back(lt::Factory<lt::Brdf>::create("Diffuse"));
        current_brdf_idx = 0;

        scenes.push_back(std::make_shared<RenderableScene>());
        scenes.push_back(std::make_shared<RenderableScene>());
        
        lt::dir_light(scenes[0]->scn, scenes[0]->ren);
        scenes[0]->rsen.sensor = scenes[0]->ren.sensor;
        scenes[0]->rsen.initialize();
        scenes[0]->rsen.type = RenderSensor::Type::Spectrum;
        scenes[0]->scn.geometries[0]->brdf = brdfs[current_brdf_idx];
        scenes[0]->path = "Direct";

        lt::generate_from_path(exec_path + "../../data/lte-orb/lte-orb.json", scenes[1]->scn, scenes[1]->ren);
        scenes[1]->rsen.sensor = scenes[1]->ren.sensor;
        scenes[1]->rsen.initialize();
        scenes[1]->rsen.type = RenderSensor::Type::Spectrum;
        scenes[1]->scn.geometries[1]->brdf = brdfs[current_brdf_idx];
        scenes[1]->scn.geometries[3]->brdf = brdfs[current_brdf_idx];
        scenes[1]->path = "Global";

        scn_idx = 1;
        update_opengl_scene(scenes[scn_idx], scn_idx);

        s_brdf_slice = std::make_shared<lt::Sensor>(256, 64);
        s_brdf_slice->init();
        rs_brdf_slice.sensor = s_brdf_slice;
        rs_brdf_slice.initialize();
        rs_brdf_slice.type = RenderSensor::Type::Spectrum;

        int res_theta_sampling = 64;
        int res_phi_sampling = 4 * res_theta_sampling;

        s_brdf_sampling = std::make_shared<lt::HemisphereSensor>(res_phi_sampling, res_theta_sampling);
        s_brdf_sampling->init();
        rs_brdf_sampling.sensor = s_brdf_sampling;
        rs_brdf_sampling.initialize();
        rs_brdf_sampling.type = RenderSensor::Type::Spectrum;


        s_brdf_sampling_pdf = std::make_shared<lt::Sensor>(res_phi_sampling, res_theta_sampling);
        s_brdf_sampling_pdf->init();
        rs_brdf_sampling_pdf.sensor = s_brdf_sampling_pdf;
        rs_brdf_sampling_pdf.initialize();
        rs_brdf_sampling_pdf.type = RenderSensor::Type::Spectrum;


        s_brdf_sampling_diff = std::make_shared<lt::Sensor>(res_phi_sampling, res_theta_sampling);
        s_brdf_sampling_diff->init();
        rs_brdf_sampling_diff.sensor = s_brdf_sampling_diff;
        rs_brdf_sampling_diff.initialize();
        rs_brdf_sampling_diff.type = RenderSensor::Type::Colormap;
    }
};

static AppData app_data;


void render_overlay(lt::RendererAsync& ren, const ImVec2& work_pos, bool& pause) {
    bool p_open = true;
    ImGuiWindowFlags window_flags = 0;// ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.25f); // Transparent background
    if (ImGui::BeginChild("overlay", ImVec2(125,110), &p_open, window_flags))
    {
        if (ImGui::Button("Save")) {
            lt::save_sensor_exr(*ren.sensor, "save.exr");
        }
        if (ImGui::Button(pause ? "Resume" : "Pause")) {
            pause = !pause;
        }
        ImGui::Text("%.0f ms/frame", ren.delta_time_ms);
        ImGui::Text("%d ssp", ren.sensor->n_sample(0,0));

        ImGui::EndChild();
    }
    
}

void render_polar_bg(const lt::vec3 wi) {
    static float xs[100];
    static float ys[100];
    for (int x = 0; x < 100; x++) {
        xs[x] = std::cos(3.141593 * float(x) / 99.);
        ys[x] = std::sin(3.141593 * float(x) / 99.);
    }
    ImPlot::PlotLine("", xs, ys, 100);
    xs[0] = 0;
    ys[0] = 0;
    xs[1] = -std::sqrt(1 - wi.z * wi.z);
    ys[1] = wi.z;
    ImPlot::PlotLine("", xs, ys, 2);
    xs[0] = -1.5;
    ys[0] = 0;
    xs[1] = 1.5;
    ys[1] = 0;
    ImPlot::PlotLine("", xs, ys, 2);
    xs[0] = 0;
    ys[0] = 0;
    xs[1] = 0;
    ys[1] = 1.5;
    ImPlot::PlotLine("", xs, ys, 2);
}

void brdf_slice(std::shared_ptr<lt::Brdf> brdf, float th_i, float ph_i, std::shared_ptr<lt::Sensor> sensor, lt::Sampler& sampler) {

#if 1
    std::vector<float> th = lt::linspace<float>(0, 0.5 * lt::pi, sensor->h);
    std::vector<float> ph = lt::linspace<float>(0, 2. * lt::pi, sensor->w);

    lt::vec3 wi = lt::polar_to_card(th_i, ph_i);
    for (int x = 0; x < sensor->w; x++) {
        for (int y = 0; y < sensor->h; y++) {
            lt::vec3 wo = lt::polar_to_card(th[y], ph[x]);
            sensor->value[y * sensor->w + x] = brdf->eval(wi, wo, sampler);
        }
    }
#endif
#if 0
    std::vector<float> th_d = lt::linspace<float>(0.5 * lt::pi, 0., sensor->h);
    std::vector<float> th_h = lt::linspace<float>(0, 0.5 * lt::pi, sensor->w);
    float ph = ph_i;

    for (int x = 0; x < sensor->w; x++) {
        for (int y = 0; y < sensor->h; y++) {

            float sin_th_d = std::sin(th_d[y]);
            float cos_th_d = std::cos(th_d[y]);

            float sin_th_h = std::sin(th_h[x]);
            float cos_th_h = std::cos(th_h[x]);

            float sin_ph = std::sin(ph);
            float cos_ph = std::cos(ph);

            lt::vec3 wd = lt::polar_to_card(th_d[y], 0);
            lt::vec3 wi = lt::vec3(cos_th_h * wd.x + sin_th_h * wd.z, 0., -sin_th_h * wd.x + cos_th_h * wd.z);
            wi = lt::vec3(cos_ph * wi.x + sin_ph * wi.y, -sin_ph * wi.x + cos_ph * wi.y, wi.z);

            lt::vec3 wh = lt::polar_to_card(th_h[x], 0.);

            lt::vec3 wo = glm::reflect(-wi, wh);

            sensor->value[y * sensor->w + x] = glm::pow(brdf->eval(wi, wo), lt::vec3(0.4545));
        }
    }
#endif
}

template<typename T>
static void draw_param_gui(const std::shared_ptr<T>& obj,std::string prev="") {

    for (int i = 0; i < obj->params.count; i++) {
        std::string param_name = obj->params.names[i] + "##" + prev ;
        switch (obj->params.types[i])
        {
        case lt::Params::Type::BOOL:
            NEED_RESET(ImGui::Checkbox(param_name.c_str(), (bool*)obj->params.ptrs[i]));
            break;
        case lt::Params::Type::INT:
            NEED_RESET(ImGui::DragInt(param_name.c_str(), (int*)obj->params.ptrs[i], 0, 1, 12));
            break;
        case lt::Params::Type::FLOAT:
            NEED_RESET(ImGui::DragFloat(param_name.c_str(), (float*)obj->params.ptrs[i], 0.01, 0.001, 3.));
            break;
        case lt::Params::Type::VEC3:
            NEED_RESET(ImGui::ColorEdit3(param_name.c_str(), (float*)obj->params.ptrs[i]));
            break;
        case lt::Params::Type::IOR:
            NEED_RESET(ImGui::DragFloat3(param_name.c_str(), (float*)obj->params.ptrs[i], 0.01, 0.5, 10.));
            break;
        case lt::Params::Type::BRDF:
            ImGui::Separator();
            ImGui::Text(obj->params.names[i].c_str());
            draw_param_gui(*((std::shared_ptr<lt::Brdf>*)obj->params.ptrs[i]),param_name);
            ImGui::Separator();
            break;
        default:
            break;
        }
    }

}

static void tab_brdf_slice(AppData& app_data, bool& open) {
    std::shared_ptr<lt::Brdf> cur_brdf = app_data.brdfs[app_data.current_brdf_idx];

    brdf_slice(cur_brdf, app_data.theta_i, app_data.phi_i, app_data.s_brdf_slice, app_data.sampler);
    app_data.rs_brdf_slice.update_data();

    if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, 0), ImPlotFlags_Equal, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {

        ImPlot::PlotImage("BRDF slice", (ImTextureID)app_data.rs_brdf_slice.id(), ImVec2(0, 0), ImVec2(app_data.s_brdf_slice->w, app_data.s_brdf_slice->h));
        ImPlot::EndPlot();
    }
    
}

static void tab_brdf_plot(AppData& app_data, bool& open) {
    if (ImGui::BeginTabBar("##TabsPolar", ImGuiTabBarFlags_None))
    {

        if (ImGui::BeginTabItem("Polar"))
        {
            static ImPlotSubplotFlags flags = ImPlotSubplotFlags_LinkRows | ImPlotSubplotFlags_LinkCols | ImPlotSubplotFlags_LinkAllX | ImPlotSubplotFlags_LinkAllY | ImPlotSubplotFlags_NoTitle;


            if (ImPlot::BeginSubplots("##AxisLinking", 2, 1, ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() * 0.95), flags)) {

                std::vector<float> th = lt::linspace<float>(-0.5 * lt::pi, 0.5 * lt::pi, 1001);

                if (ImPlot::BeginPlot("", " ", " ", ImVec2(0, -1), ImPlotFlags_Equal, ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoGridLines)) {

                    lt::vec3 wi = lt::polar_to_card(app_data.theta_i, app_data.phi_i);
                    render_polar_bg(wi);
                    for (int i = 0; i < app_data.brdfs.size(); i++) {
                        std::shared_ptr<lt::Brdf> brdf = app_data.brdfs[i];

                        static float xs[1001];
                        static float ys[1001];
                        for (int x = 0; x < 1001; x++) {
                            lt::vec3 wo = lt::polar_to_card(th[x], 0.);
                            lt::vec3 rgb = brdf->eval(wi, wo, app_data.sampler);
                            float l = (rgb.x + rgb.y + rgb.z) / 3.;
                            xs[x] = -wo.x * l;
                            ys[x] = wo.z * l;
                        }
                        ImPlot::PlotLine((brdf->type + "##" + std::to_string(i)).c_str(), xs, ys, 1001);

                    }
                    ImPlot::EndPlot();
                }

                if (ImPlot::BeginPlot(app_data.brdfs[app_data.current_brdf_idx]->type.c_str(), " ", " ", ImVec2(0, -1), ImPlotFlags_Equal, ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoGridLines)) {


                    lt::vec3 wi = lt::polar_to_card(app_data.theta_i, app_data.phi_i);
                    render_polar_bg(wi);

                    lt::vec3 rgb[1001];
                    for (int x = 0; x < 1001; x++) {
                        lt::vec3 wo = lt::polar_to_card(th[x], 0.);
                        rgb[x] = app_data.brdfs[app_data.current_brdf_idx]->eval(wi, wo, app_data.sampler);
                    }

                    const char* col_name[3] = { "r", "g", "b" };
                    const ImVec4 col[3] = { ImVec4(1., 0., 0., 1.), ImVec4(0., 1., 0., 1.), ImVec4(0., 0., 1., 1.) };
                    for (int c = 0; c < 3; c++) {
                        static float xs[1001];
                        static float ys[1001];
                        for (int x = 0; x < 1001; x++) {
                            lt::vec3 wo = lt::polar_to_card(th[x], 0.);
                            xs[x] = -wo.x * rgb[x][c];
                            ys[x] = wo.z * rgb[x][c];
                        }
                        ImPlot::SetNextLineStyle(col[c]);
                        ImPlot::PlotLine(col_name[c], xs, ys, 1001);

                    }

                    ImPlot::EndPlot();
                }

                ImPlot::EndSubplots();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Theta O"))
        {
            static ImPlotSubplotFlags flags = ImPlotSubplotFlags_LinkRows | ImPlotSubplotFlags_LinkCols | ImPlotSubplotFlags_LinkAllX | ImPlotSubplotFlags_LinkAllY;
            if (ImPlot::BeginSubplots("##AxisLinking", 2, 1, ImVec2(-1, -1), flags)) {


                std::vector<float> th = lt::linspace<float>(0., 0.5 * lt::pi, 1001);

                if (ImPlot::BeginPlot("", "Theta O ", " ", ImVec2(-1, 0), 0, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {

                    for (int i = 0; i < app_data.brdfs.size(); i++) {
                        std::shared_ptr<lt::Brdf> brdf = app_data.brdfs[i];

                        static float xs[1001];
                        for (int x = 0; x < 1001; x++) {
                            lt::vec3 wi = lt::polar_to_card(app_data.theta_i, app_data.phi_i);
                            lt::vec3 wo = lt::polar_to_card(th[x], 0.);
                            lt::vec3 rgb = brdf->eval(wi, wo, app_data.sampler);
                            xs[x] = (rgb.x + rgb.y + rgb.z) / 3.;

                        }
                        ImPlot::PlotLine((brdf->type + "##" + std::to_string(i)).c_str(), th.data(), xs, 1001);


                    }
                    ImPlot::EndPlot();
                }

                if (ImPlot::BeginPlot(app_data.brdfs[app_data.current_brdf_idx]->type.c_str(), "Theta O", " ")) {


                    static float r[1001];
                    static float g[1001];
                    static float b[1001];
                    for (int x = 0; x < 1001; x++) {
                        lt::vec3 wi = lt::polar_to_card(app_data.theta_i, app_data.phi_i);
                        lt::vec3 wo = lt::polar_to_card(th[x], 0.);
                        lt::vec3 rgb = app_data.brdfs[app_data.current_brdf_idx]->eval(wi, wo, app_data.sampler);
                        r[x] = rgb.x;
                        g[x] = rgb.y;
                        b[x] = rgb.z;

                    }
                    ImPlot::SetNextLineStyle(ImVec4(1., 0., 0., 1.));
                    ImPlot::PlotLine("r", th.data(), r, 1001);
                    ImPlot::SetNextLineStyle(ImVec4(0., 1., 0., 1.));
                    ImPlot::PlotLine("g", th.data(), g, 1001);
                    ImPlot::SetNextLineStyle(ImVec4(0., 0., 1., 1.));
                    ImPlot::PlotLine("b", th.data(), b, 1001);

                    ImPlot::EndPlot();
                }

                ImPlot::EndSubplots();
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

static void tab_brdf_sampling(AppData& app_data, bool& open) {
    lt::vec3 wi = lt::polar_to_card(app_data.theta_i, app_data.phi_i);


    for (int i = 0; i < 10000; i++) {
        lt::vec3 wo = app_data.brdfs[app_data.current_brdf_idx]->sample(wi, app_data.sampler).wo;
        float phi = std::atan2(wo.y, wo.x);
        phi = phi < 0 ? 2 * lt::pi + phi : phi;
        float x = phi / (2. * lt::pi) * (float)app_data.s_brdf_sampling->w;
        float y = std::acos(wo.z) / (0.5 * lt::pi) * (float)app_data.s_brdf_sampling->h;
        if (y < app_data.s_brdf_sampling->h)
            app_data.s_brdf_sampling->add(int(x), int(y), lt::Spectrum(1));
        else
            app_data.s_brdf_sampling->sum_counts++;
    }


    for (int i = 0; i < 10000; i++) {
        lt::vec3 wo = lt::square_to_cosine_hemisphere(app_data.sampler.next_float(), app_data.sampler.next_float());
        float phi = std::atan2(wo.y, wo.x);
        phi = phi < 0 ? 2 * lt::pi + phi : phi;
        int x = int(phi / (2. * lt::pi) * (float)app_data.s_brdf_sampling->w);
        int y = int(std::acos(wo.z) / (0.5 * lt::pi) * (float)app_data.s_brdf_sampling->h);

        if (y < app_data.s_brdf_sampling->h) {
            app_data.s_brdf_sampling_pdf->add(x, y, lt::Spectrum(app_data.brdfs[app_data.current_brdf_idx]->pdf(wi, wo)));
            app_data.s_brdf_sampling_diff->set(x, y, (app_data.s_brdf_sampling_pdf->get(x, y) - app_data.s_brdf_sampling->get(x, y)));
        }
        else {
            app_data.s_brdf_sampling_pdf->sum_counts++;
            app_data.s_brdf_sampling_diff->sum_counts++;
        }
    }


    app_data.rs_brdf_sampling.update_data();
    app_data.rs_brdf_sampling_pdf.update_data();
    app_data.rs_brdf_sampling_diff.update_data();

    if (ImPlot::BeginPlot("Sample density", "", "", ImVec2(ImGui::GetWindowWidth() * 0.5, 0.), ImPlotFlags_Equal, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {
        ImPlot::PlotImage("", (ImTextureID)app_data.rs_brdf_sampling.id(), ImVec2(0, 0), ImVec2(app_data.rs_brdf_sampling.sensor->w, app_data.rs_brdf_sampling.sensor->h));
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    if (ImPlot::BeginPlot("Mean pdf", "", "", ImVec2(ImGui::GetWindowWidth() * 0.5, 0.), ImPlotFlags_Equal, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {
        ImPlot::PlotImage("", (ImTextureID)app_data.rs_brdf_sampling_pdf.id(), ImVec2(0, 0), ImVec2(app_data.rs_brdf_sampling_pdf.sensor->w, app_data.rs_brdf_sampling_pdf.sensor->h));
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Signed difference between [-0.5,0.5]", "", "", ImVec2(-1, 0.), ImPlotFlags_Equal, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {
        ImPlot::PlotImage("", (ImTextureID)app_data.rs_brdf_sampling_diff.id(), ImVec2(0, 0), ImVec2(app_data.rs_brdf_sampling_diff.sensor->w, app_data.rs_brdf_sampling_diff.sensor->h));
        ImPlot::EndPlot();
    }

    if (ImGui::Button("Export EXR")) {
        lt::save_sensor_exr(*app_data.s_brdf_sampling, "brdf_sampling.exr");
        lt::save_sensor_exr(*app_data.s_brdf_sampling_pdf, "brdf_sampling_pdf.exr");
        lt::save_sensor_exr(*app_data.s_brdf_sampling_diff, "brdf_sampling_diff.exr");
    }
}

static void render_scn(std::shared_ptr<RenderableScene> r, bool& open) {

    if (!r->pause) {
        if (r->ren.render(r->scn) && !r->ren.need_reset) {
            r->rsen.update_data();
        }
    }

    ImVec2 work_pos;
    if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, -1), ImPlotFlags_Equal | ImPlotFlags_NoFrame, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {
        ImPlot::PlotImage("", (ImTextureID)r->rsen.id(), ImVec2(0, 0), ImVec2(r->ren.sensor->w, r->ren.sensor->h));
        work_pos = ImPlot::GetPlotPos();
        ImPlot::EndPlot();
    }

    render_overlay(r->ren, ImVec2(work_pos.x + 4, work_pos.y + 4), r->pause);
}

static void tab_brdf_dir_light(AppData& app_data, bool& open) {
    std::shared_ptr<RenderableScene> r = app_data.scenes[0];
    render_scn(r, open);
}

static void tab_brdf_global_illu(AppData& app_data, bool& open) {
    std::shared_ptr<RenderableScene> r = app_data.scenes[1];
    render_scn(r, open);
}

static void tab_brdf_left_panel(AppData& app_data, bool& open) {

    ImGui::BeginChild("direction", ImVec2(250, 68), true);
    NEED_RESET(ImGui::SliderAngle("th_i", &app_data.theta_i, 0, 90));
    NEED_RESET(ImGui::SliderAngle("ph_i", &app_data.phi_i, 0, 360));
    ImGui::EndChild();

    std::shared_ptr<lt::DirectionnalLight> dl = std::static_pointer_cast<lt::DirectionnalLight>(app_data.scenes[0]->scn.lights[0]);
    dl->dir = -lt::vec3(std::sin(app_data.theta_i) * std::cos(app_data.phi_i), std::cos(app_data.theta_i), std::sin(app_data.theta_i) * std::sin(app_data.phi_i));


    ImGui::BeginChild("top pane", ImVec2(250, ImGui::GetContentRegionAvail().y * 0.25), true);

    const std::vector<std::string>& brdf_names = lt::Factory<lt::Brdf>::names();

    if (ImGui::Button("add BRDF",ImVec2(235,25)))
        ImGui::OpenPopup("brdf_popup");

    if (ImGui::BeginPopup("brdf_popup"))
    {
        for (int i = 0; i < brdf_names.size(); i++)
            if (ImGui::Selectable(brdf_names[i].c_str()))
                app_data.brdfs.push_back(lt::Factory<lt::Brdf>::create(brdf_names[i]));
        ImGui::EndPopup();
    }

    ImGui::Separator();

    for (int i = 0; i < app_data.brdfs.size(); i++)
    {
        if (ImGui::Selectable((app_data.brdfs[i]->type + "##" + std::to_string(app_data.brdfs[i]->id)).c_str(), app_data.current_brdf_idx == i)) {
            app_data.current_brdf_idx = i;
            app_data.scenes[0]->scn.geometries[0]->brdf = app_data.brdfs[i];
            app_data.scenes[1]->scn.geometries[1]->brdf = app_data.brdfs[i];
            app_data.scenes[1]->scn.geometries[3]->brdf = app_data.brdfs[i];
            need_reset = true;
        }
    }

    ImGui::EndChild();
    ImGui::BeginChild("bottom pane", ImVec2(250, 0), true);

    std::shared_ptr<lt::Brdf> cur_brdf = app_data.brdfs[app_data.current_brdf_idx];

    draw_param_gui(cur_brdf);
    ImGui::EndChild();
}

static void tab_scene_render( int scn_idx, AppData& app_data, bool& open, bool update_gl) {
    if (app_data.scenes.size() < 1 || scn_idx > app_data.scenes.size() - 1 || scn_idx < 0)
        return;

    std::shared_ptr<RenderableScene> r = app_data.scenes[scn_idx];
    update_opengl_scene(r,scn_idx);
    draw_opengl_scene();
    if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, -1), ImPlotFlags_Equal | ImPlotFlags_NoFrame, ImPlotAxisFlags_AutoFit| ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations)) {
        ImPlot::PlotImage("", (ImTextureID)opengl_scene.tex_id, ImVec2(0, 0), ImVec2(opengl_scene.res_x, opengl_scene.res_y));
        ImPlot::EndPlot();
    }

    if (ImGui::Begin(app_data.scenes[scn_idx]->ren.camera->type.c_str(), &open)) {
        render_scn(r, open);
        ImGui::End();
    }
}

static void tab_scene_left_panel(AppData& app_data, bool& open) {

    ImGui::BeginChild("top pane", ImVec2(250, ImGui::GetContentRegionAvail().y * 1.), true);

    std::shared_ptr<RenderableScene> r = app_data.scenes[app_data.scn_idx];
    const std::vector<std::string>& intergrator_names = lt::Factory<lt::Integrator>::names();

    if (ImGui::Button(r->ren.integrator->type.c_str(), ImVec2(235, 25)))
        ImGui::OpenPopup("integrator_popup");

    if (ImGui::BeginPopup("integrator_popup"))
    {
        for (int i = 0; i < intergrator_names.size(); i++)
            if (ImGui::Selectable(intergrator_names[i].c_str()))
                r->ren.integrator = lt::Factory<lt::Integrator>::create(intergrator_names[i]);
        ImGui::EndPopup();
        NEED_RESET(true);
    }

    ImGui::Separator();
    draw_param_gui(r->ren.integrator);

    ImGui::EndChild();

}

static void app_brdf(AppData& app_data, bool& open) {
    
    int tab = 0;
    ImGui::Dummy(ImVec2(100.,0.));
    ImGui::SameLine();
    if (ImGui::BeginTabBar("##TabsBRDF"))
    {
        if (ImGui::BeginTabItem("BRDF slice"))
        {
            tab = 0;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Plot"))
        {
            tab = 1;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sampling"))
        {   
            tab = 2;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Directional light"))
        {
            tab = 3;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global illumination"))
        {
            tab = 4;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::BeginGroup();
    tab_brdf_left_panel(app_data, open);
    ImGui::EndGroup();
    ImGui::SameLine();
    
    ImGui::BeginGroup();
    ImGui::BeginChild("item view", ImVec2(0, 0), true);
    switch (tab)
    {
    case 0:
        tab_brdf_slice(app_data, open);
        break;
    case 1:
        tab_brdf_plot(app_data, open);
        break;
    case 2:
        tab_brdf_sampling(app_data, open);
        break;
    case 3:
        tab_brdf_dir_light(app_data, open);
        break;
    case 4:
        tab_brdf_global_illu(app_data, open);
        break;
    default:
        break;
    }
    ImGui::EndChild();
    ImGui::EndGroup();

}

static void app_scene(AppData& app_data, bool& open) {
    int tab = 0;
    ImGui::Dummy(ImVec2(100., 0.));
    ImGui::SameLine();
    if (ImGui::BeginTabBar("##TabsBRDF"))
    {
        for (int i = 0; i < app_data.scenes.size(); i++) {
            if (ImGui::BeginTabItem( app_data.scenes[i]->path.c_str() ) )
            {
                tab = i;
                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("+"))
        {
            tab = -1;
            ImGui::Text("Drag and Drop .json scene file in the application");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::BeginGroup();
    tab_scene_left_panel(app_data, open);
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    tab_scene_render(tab, app_data, open, false);
    ImGui::EndGroup();
}

static void app_layout(glm::ivec4& win_frame, AppData& app_data, bool& open)
{
    ImGui::SetNextWindowSize(ImVec2(win_frame.x, win_frame.y));

    if (need_reset) {
        app_data.scenes[0]->ren.reset();
        app_data.scenes[1]->ren.reset();
        app_data.s_brdf_sampling->reset();
        app_data.s_brdf_sampling_pdf->reset();
        app_data.s_brdf_sampling_diff->reset();
        need_reset = false;
    }

    if (ImGui::Begin("LIL VIEWER", &open, ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Brdf"))
            {
                ImGui::SameLine();
                app_brdf(app_data, open);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Scene"))
            {
                ImGui::SameLine();
                app_scene(app_data, open);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
       
    }
    ImGui::End();

}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    int i;
    for (i = 0; i < count; i++){
        app_data.scenes.push_back(std::make_shared<RenderableScene>());
        std::shared_ptr<RenderableScene> r = app_data.scenes[app_data.scenes.size() - 1];   
        lt::generate_from_path(paths[i], r->scn, r->ren);
        r->rsen.sensor = r->ren.sensor;
        r->rsen.initialize();
        r->rsen.type = RenderSensor::Type::Spectrum;
        r->path = paths[i];
        std::cout << paths[i] << std::endl;
    }
}

GLFWwindow* init() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return nullptr;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_DECORATED, 0);
    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Lil Viewer", nullptr, nullptr);
    if (window == nullptr)
        return nullptr;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        lt::Log(lt::logError) << "Error: " << glewGetErrorString(err);
        glfwTerminate();
        return nullptr;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.Fonts->AddFontFromFileTTF((exec_path + "./../../3rd_party/Cascadia.ttf").c_str(), 17.);

    //ImGui::StyleColorsDark();
    lil_tracer_theme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return window;
}

void display(GLFWwindow* window, glm::ivec4 win_frame) {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void terminate(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void new_frame(GLFWwindow* window, glm::ivec4& win_frame) {
    glfwPollEvents();

    glfwGetWindowSize(window, &win_frame.x, &win_frame.y);
    glViewport(0, 0, win_frame.x, win_frame.y);

    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.0f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// Main code
int main(int argc, char* argv[])
{   
    std::filesystem::path executable_path(argv[0]);
    std::filesystem::path current_path = std::filesystem::current_path();
    exec_path = std::filesystem::relative(executable_path, current_path).remove_filename().generic_string();

    lt::State::log_level = lt::logNoLabel;
    lt::State::exectuable_path = exec_path;

    GLFWwindow*  window = init();
    if (window == nullptr)
        return 0;
    
    app_data.init();
    opengl_scene.init();
    
    //static auto drop_binding = [&](GLFWwindow* window, int path_count, const char* paths[]) { drop_callback( app_data, window, path_count, paths); };
    glfwSetDropCallback(window, drop_callback);
    
    bool open = true;
    glm::ivec4 win_frame{};
    while (!glfwWindowShouldClose(window) && open)
    {

        new_frame(window, win_frame);

        //ImGui::ShowDemoWindow();
        //ImPlot::ShowDemoWindow();
        app_layout( win_frame, app_data, open);

        // Rendering
        display(window, win_frame);

    }

    terminate(window);


    return 0;
}