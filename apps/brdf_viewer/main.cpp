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

    GLuint id() {
        //return sensor_id;
        return render_id;
    }

    ~RenderSensor() {
        if (initialized) {
            glDeleteTextures(1, &sensor_id);
        }
    }
};


struct AppData {
    std::shared_ptr<lt::Sensor> s_brdf_slice;
    RenderSensor rs_brdf_slice;
    int current_brdf_idx;
    std::vector<std::shared_ptr<lt::Brdf>> brdfs;
    float theta_i = 0.5;
    float phi_i = 0.;

    lt::Scene    scn_dir_light;
    lt::RendererAsync ren_dir_light;
    RenderSensor rsen_dir_light;

    lt::Scene    scn_glo_ill;
    lt::RendererAsync ren_glo_ill;
    RenderSensor rsen_glo_ill;

    std::shared_ptr<lt::HemisphereSensor> s_brdf_sampling;
    RenderSensor rs_brdf_sampling;
    std::shared_ptr<lt::Sensor> s_brdf_sampling_pdf;
    RenderSensor rs_brdf_sampling_pdf;
    std::shared_ptr<lt::Sensor> s_brdf_sampling_diff;
    RenderSensor rs_brdf_sampling_diff;

    lt::Sampler sampler;

    lt::BrdfValidation validation;
};

void AppInit(AppData& app_data) {
    need_reset = false;

    app_data.brdfs.push_back(lt::Factory<lt::Brdf>::create("Diffuse"));
    app_data.current_brdf_idx = 0;

    lt::dir_light(app_data.scn_dir_light, app_data.ren_dir_light);
    app_data.rsen_dir_light.sensor = app_data.ren_dir_light.sensor;
    app_data.rsen_dir_light.initialize();
    app_data.rsen_dir_light.type = RenderSensor::Type::Spectrum;
    app_data.scn_dir_light.geometries[0]->brdf = app_data.brdfs[app_data.current_brdf_idx];

    lt::generate_from_path(exec_path + "../../data/lte-orb/lte-orb.json", app_data.scn_glo_ill, app_data.ren_glo_ill);
    app_data.rsen_glo_ill.sensor = app_data.ren_glo_ill.sensor;
    app_data.rsen_glo_ill.initialize();
    app_data.rsen_glo_ill.type = RenderSensor::Type::Spectrum;
    app_data.scn_glo_ill.geometries[1]->brdf = app_data.brdfs[app_data.current_brdf_idx];
    app_data.scn_glo_ill.geometries[3]->brdf = app_data.brdfs[app_data.current_brdf_idx];

    app_data.s_brdf_slice = std::make_shared<lt::Sensor>(256, 64);
    app_data.s_brdf_slice->init();
    app_data.rs_brdf_slice.sensor = app_data.s_brdf_slice;
    app_data.rs_brdf_slice.initialize();
    app_data.rs_brdf_slice.type = RenderSensor::Type::Spectrum;

    int res_theta_sampling = 64;
    int res_phi_sampling = 4 * res_theta_sampling;

    app_data.s_brdf_sampling = std::make_shared<lt::HemisphereSensor>(res_phi_sampling, res_theta_sampling);
    app_data.s_brdf_sampling->init();
    app_data.rs_brdf_sampling.sensor = app_data.s_brdf_sampling;
    app_data.rs_brdf_sampling.initialize();
    app_data.rs_brdf_sampling.type = RenderSensor::Type::Spectrum;


    app_data.s_brdf_sampling_pdf = std::make_shared<lt::Sensor>(res_phi_sampling, res_theta_sampling);
    app_data.s_brdf_sampling_pdf->init();
    app_data.rs_brdf_sampling_pdf.sensor = app_data.s_brdf_sampling_pdf;
    app_data.rs_brdf_sampling_pdf.initialize();
    app_data.rs_brdf_sampling_pdf.type = RenderSensor::Type::Spectrum;


    app_data.s_brdf_sampling_diff = std::make_shared<lt::Sensor>(res_phi_sampling, res_theta_sampling);
    app_data.s_brdf_sampling_diff->init();
    app_data.rs_brdf_sampling_diff.sensor = app_data.s_brdf_sampling_diff;
    app_data.rs_brdf_sampling_diff.initialize();
    app_data.rs_brdf_sampling_diff.type = RenderSensor::Type::Colormap;

}

void render_overlay(lt::RendererAsync& ren, const ImVec2& work_pos) {
    bool p_open = true;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f); // Transparent background
    if (ImGui::Begin("Example: Simple overlay", &p_open, window_flags))
    {
        if (ImGui::Button("Save")) {
            lt::save_sensor_exr(*ren.sensor, "save.exr");
        }
        ImGui::Text("%.0f ms/frame", ren.delta_time_ms);

    }
    ImGui::End();
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


static void draw_param_gui(const std::shared_ptr<lt::Brdf>& brdf,std::string prev="") {

    for (int i = 0; i < brdf->params.count; i++) {
        lt::Param p = brdf->params.list[i];
        std::string param_name = p.name + "##" + prev ;
        switch (p.type)
        {
        case lt::ParamType::BOOL:
            NEED_RESET(ImGui::Checkbox(param_name.c_str(), (bool*)p.ptr));
            break;
        case lt::ParamType::FLOAT:
            NEED_RESET(ImGui::DragFloat(param_name.c_str(), (float*)p.ptr, 0.01, 0.001, 3.));
            break;
        case lt::ParamType::VEC3:
            NEED_RESET(ImGui::ColorEdit3(param_name.c_str(), (float*)p.ptr));
            break;
        case lt::ParamType::IOR:
            NEED_RESET(ImGui::DragFloat3(param_name.c_str(), (float*)p.ptr, 0.01, 0.5, 10.));
            break;
        case lt::ParamType::BRDF:
            ImGui::Separator();
            ImGui::Text(p.name.c_str());
            draw_param_gui(*((std::shared_ptr<lt::Brdf>*)p.ptr),param_name);
            ImGui::Separator();
            break;
        default:
            break;
        }
    }

}


static void AppLayout(GLFWwindow* window, AppData& app_data)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    ImGui::SetNextWindowSize(ImVec2(width, height)); // ensures ImGui fits the GLFW window
    ImGui::SetNextWindowPos(ImVec2(0, 0));


    if (need_reset) {
        app_data.ren_glo_ill.reset();
        app_data.ren_dir_light.reset();
        app_data.s_brdf_sampling->reset();
        app_data.s_brdf_sampling_pdf->reset();
        app_data.s_brdf_sampling_diff->reset();
        need_reset = false;
    }

    bool open = true;
    if (ImGui::Begin("Simple layout", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {

        {
            ImGui::BeginGroup();

            ImGui::BeginChild("top pane", ImVec2(250, ImGui::GetContentRegionAvail().y * 0.25), true);


            const std::vector<std::string>& brdf_names = lt::Factory<lt::Brdf>::names();

            if (ImGui::Button("New BRDF"))
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
                    app_data.scn_dir_light.geometries[0]->brdf = app_data.brdfs[i];
                    app_data.scn_glo_ill.geometries[1]->brdf = app_data.brdfs[i];
                    app_data.scn_glo_ill.geometries[3]->brdf = app_data.brdfs[i];
                    need_reset = true;
                }
            }

            ImGui::EndChild();
            ImGui::BeginChild("bottom pane", ImVec2(250, 0), true);

            std::shared_ptr<lt::Brdf> cur_brdf = app_data.brdfs[app_data.current_brdf_idx];

            draw_param_gui(cur_brdf);
            
            ImGui::EndChild();


            ImGui::EndGroup();
        }

        ImGui::SameLine();

        // Right
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("item view", ImVec2(0, ImGui::GetWindowHeight() * 0.9), true); // Leave room for 1 line below us

            if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("BRDF slice"))
                {

                    std::shared_ptr<lt::Brdf> cur_brdf = app_data.brdfs[app_data.current_brdf_idx];

                    brdf_slice(cur_brdf, app_data.theta_i, app_data.phi_i, app_data.s_brdf_slice, app_data.sampler);
                    app_data.rs_brdf_slice.update_data();

                    if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, 0), ImPlotFlags_Equal, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {

                        ImPlot::PlotImage("BRDF slice", (ImTextureID)app_data.rs_brdf_slice.id(), ImVec2(0, 0), ImVec2(app_data.s_brdf_slice->w, app_data.s_brdf_slice->h));
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Plot"))
                {
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
                                            xs[x] =-wo.x * l;
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
                                            xs[x] =-wo.x * rgb[x][c];
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
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Sampling"))
                {
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


                    //std::vector<float> th = lt::linspace<float>(0, 0.5 * lt::pi, app_data.s_brdf_sampling->h);
                    //std::vector<float> ph = lt::linspace<float>(0, 2. * lt::pi, app_data.s_brdf_sampling->w);
                    //for (int x = 0; x < app_data.s_brdf_sampling_pdf->w; x++) {
                    //    for (int y = 0; y < app_data.s_brdf_sampling_pdf->h; y++) {
                    //        float jw = 2. * lt::pi * (app_data.sampler.next_float() - 0.5) / (float)app_data.s_brdf_sampling_pdf->w;
                    //        float jh = 0.5 * lt::pi * (app_data.sampler.next_float() - 0.5) / (float)app_data.s_brdf_sampling_pdf->h;
                    //        
                    //        lt::vec3 wo = lt::polar_to_card(th[y] + jh, ph[x] + jw);
                    //        app_data.s_brdf_sampling_pdf->add(x,y, lt::Spectrum(app_data.brdfs[app_data.current_brdf_idx]->pdf(wi, wo)));
                    //        app_data.s_brdf_sampling_diff->set(x,y,glm::abs(app_data.s_brdf_sampling_pdf->get(x, y) - app_data.s_brdf_sampling->get(x, y)));
                    //    }
                    //}


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

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Directional Light"))
                {
                    bool ren_will_reset = app_data.ren_dir_light.need_reset;
                    if (app_data.ren_dir_light.render(app_data.scn_dir_light) && !ren_will_reset){
                        app_data.rsen_dir_light.update_data();
                    }

                    ImVec2 work_pos;
                    if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, -1), ImPlotFlags_Equal | ImPlotFlags_NoFrame, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {
                        ImPlot::PlotImage("", (ImTextureID)app_data.rsen_dir_light.id(), ImVec2(0, 0), ImVec2(app_data.ren_dir_light.sensor->w, app_data.ren_dir_light.sensor->h));
                        work_pos = ImPlot::GetPlotPos(); 
                        ImPlot::EndPlot();
                    }

                    render_overlay(app_data.ren_dir_light, work_pos);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Global Illumination"))
                {
                   
                    bool ren_will_reset = app_data.ren_glo_ill.need_reset;
                    if (app_data.ren_glo_ill.render(app_data.scn_glo_ill) && !ren_will_reset){
                        app_data.rsen_glo_ill.update_data();
                    }

                    ImVec2 work_pos;
                    if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, -1), ImPlotFlags_Equal | ImPlotFlags_NoFrame, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit)) {
                        ImPlot::PlotImage("", (ImTextureID)app_data.rsen_glo_ill.id(), ImVec2(0, 0), ImVec2(app_data.ren_glo_ill.sensor->w, app_data.ren_glo_ill.sensor->h));
                        work_pos =  ImPlot::GetPlotPos();
                        ImPlot::EndPlot();
                    }

                    render_overlay(app_data.ren_glo_ill, work_pos);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Validation"))
                {

                    if (ImGui::Button("Run validation of current brdf model")) {
                        app_data.validation = lt::BrdfValidation::validate(*app_data.brdfs[app_data.current_brdf_idx]);
                    }

                    ImGui::SameLine();
                    ImGui::PushItemWidth(100.);
                    ImGui::DragInt("Number of samples", &lt::BrdfValidation::number_of_sample);
                    ImGui::SameLine();
                    ImGui::DragInt("Number of thetas", &lt::BrdfValidation::number_of_theta);
                    ImGui::PopItemWidth();

                    ImGui::Separator();

                    ImGui::PushStyleColor(ImGuiCol_Text, app_data.validation.correct_sampling ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
                    ImGui::Text("Correct sampling");
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_Text, app_data.validation.found_nan ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
                    ImGui::Text("NaN");
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_Text, app_data.validation.negative_value ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
                    ImGui::Text("Negative values");
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_Text, app_data.validation.reciprocity ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
                    ImGui::Text("Reciprocity");
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_Text, app_data.validation.energy_conservative ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
                    ImGui::Text("Energy conservation");
                    ImGui::PopStyleColor();

                    if (ImPlot::BeginPlot("Directional albedo", "theta", "albedo", ImVec2(-1, 0), 0, ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock)) {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 1.1);
                        ImPlot::SetupAxisLimits(ImAxis_X1, 0, lt::pi / 2.);
                        ImPlot::PlotStems("", app_data.validation.thetas.data(), app_data.validation.directional_albedo.data(), app_data.validation.thetas.size(), 0.);
                        ImPlot::EndPlot();
                    }

                    if (ImPlot::BeginPlot("Sampling difference", "theta", "mean signed difference", ImVec2(-1, 0), 0, ImPlotAxisFlags_Lock, ImPlotAxisFlags_AutoFit)) {
                        ImPlot::SetupAxisLimits(ImAxis_X1, 0, lt::pi / 2.);
                        ImPlot::PlotStems("", app_data.validation.thetas.data(), app_data.validation.sampling_difference.data(), app_data.validation.thetas.size(), 0.);
                        ImPlot::EndPlot();
                    }


                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();

            }
            ImGui::EndChild();

            ImGui::BeginChild("scene param", ImVec2(0, 0), true);

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.45);
            NEED_RESET(ImGui::SliderAngle("th_i", &app_data.theta_i, 0, 90));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.45);
            NEED_RESET(ImGui::SliderAngle("ph_i", &app_data.phi_i, 0, 360));

            std::shared_ptr<lt::DirectionnalLight> dl = std::static_pointer_cast<lt::DirectionnalLight>(app_data.scn_dir_light.lights[0]);
            dl->dir = -lt::vec3(std::sin(app_data.theta_i) * std::cos(app_data.phi_i), std::cos(app_data.theta_i), std::sin(app_data.theta_i) * std::sin(app_data.phi_i));

            ImGui::EndChild();

            ImGui::EndGroup();
        }
    }
    ImGui::End();
}



// Main code
int main(int argc, char* argv[])
{   
    std::filesystem::path executable_path(argv[0]);
    std::filesystem::path current_path = std::filesystem::current_path();
    exec_path = std::filesystem::relative(executable_path, current_path).remove_filename().generic_string();

    lt::State::log_level = lt::logNoLabel;
    lt::State::exectuable_path = exec_path;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "BRDF Viewer", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync


    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        lt::Log(lt::logError) << "Error: " << glewGetErrorString(err);
        glfwTerminate();
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.Fonts->AddFontFromFileTTF( (exec_path + "./../../3rd_party/Cascadia.ttf").c_str(), 17.);

    //ImGui::StyleColorsDark();
    lil_tracer_theme();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.0f, 1.00f);


    AppData app_data;
    AppInit(app_data);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();


        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow();
        //ImPlot::ShowDemoWindow();

        AppLayout(window, app_data);


        int display_w, display_h;
        // Rendering
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }


    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();



    return 0;
}