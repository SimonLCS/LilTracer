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
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <implot.h>

std::string exec_path;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool need_reset;
#define NEED_RESET(x) if(x){ need_reset = true; }

struct RenderTexture {
    GLuint texture_id;
    lt::Texture<lt::Spectrum>* texture = nullptr;
    bool initialized = false;


    bool update_data() {

        if (!initialized) {
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->w, texture->h, 0, GL_RGB, GL_FLOAT, (float*)(texture->data.get()));
        return true;
    }

    bool initialize()
    {

        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        initialized = true;
        return true;
    }

    ~RenderTexture() {
        if (initialized) {
            glDeleteTextures(1, &texture_id);
        }
    }
};


struct AppData {
    std::shared_ptr<lt::EnvironmentLight> envmap;
    RenderTexture rt;
    std::vector<lt::vec3> samples;
    std::vector<float> x;
    std::vector<float> y;
    int nsample;
    lt::Sampler sampler;

};

void AppInit(AppData& app_data) {
    need_reset = false;

    app_data.envmap = std::make_shared<lt::EnvironmentLight>();
    lt::load_texture_exr(exec_path +"./../../data/envmaps/hallstatt4_hd.exr", app_data.envmap->envmap);
    app_data.envmap->init();
    app_data.rt.texture = app_data.envmap->envmap.get();
    app_data.rt.initialize();

    app_data.sampler.seed(0);

    app_data.nsample = 20000;
    app_data.samples.resize(app_data.nsample);
    app_data.x.resize(app_data.nsample);
    app_data.y.resize(app_data.nsample);
    for (int i = 0; i < app_data.nsample; i++) {
        lt::Light::Sample env_sample = app_data.envmap->sample(lt::SurfaceInteraction(), app_data.sampler);
        app_data.samples[i] = -env_sample.direction;
        
        assert(env_sample.pdf > 0.);
        float env_pdf = app_data.envmap->pdf(lt::vec3(0.), env_sample.direction);
        // assert(env_sample.pdf == env_pdf);
        
        if (env_sample.pdf <= 0.) {
            lt::Log(lt::logError) << "invalid sample pdf";
        }
        
        float phi = std::atan2(app_data.samples[i].z, app_data.samples[i].x);
        phi = phi > 0. ? phi : 2 * lt::pi + phi;
        app_data.x[i] = phi / (2. * lt::pi) * (float)app_data.envmap->envmap->w;
        app_data.y[i] = std::acos(app_data.samples[i].y) / lt::pi * (float)app_data.envmap->envmap->h;
        app_data.y[i] = (float)app_data.envmap->envmap->h - app_data.y[i] - 1;

    }

}


static void AppLayout(GLFWwindow* window, AppData& app_data)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    ImGui::SetNextWindowSize(ImVec2(width, height)); // ensures ImGui fits the GLFW window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    

    if (need_reset) {
        need_reset = false;
    }
    bool open = true;
    if (ImGui::Begin("Simple layout", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
    {
        app_data.rt.update_data();

        if (ImPlot::BeginPlot("##image", "", "", ImVec2(-1, -1), ImPlotFlags_Equal)) {

            ImPlot::PlotImage("BRDF slice", (ImTextureID)app_data.rt.texture_id, ImVec2(0, 0), ImVec2(app_data.envmap->envmap->w, app_data.envmap->envmap->h));
            ImPlot::PlotScatter("Samples", app_data.x.data(), app_data.y.data(), app_data.nsample);
            ImPlot::EndPlot();

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
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Envmap sampling", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.Fonts->AddFontFromFileTTF( (exec_path + "./../../3rd_party/Cascadia.ttf").c_str(), 17.);
    
    ImGui::StyleColorsDark();
    
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

        // Rendering
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
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