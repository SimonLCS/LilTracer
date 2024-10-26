#include <iostream>
#include <vector>

const char* v_3d = " #version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aNormal;\n"
"out vec3 FragPos;\n"
"out vec3 Normal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    FragPos = vec3(model * vec4(aPos,1.0));\n"
"    Normal = normalize(vec3(transpose(model) * vec4(aNormal,0.)));\n"
"    gl_Position = projection * view * vec4(FragPos,1.0);\n"
"}";

const char* f_3d = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"uniform vec3 lightDir;\n"
"void main()\n"
"{\n"
"   vec3 col = vec3(0.85,0.9,0.95) * max(dot(Normal, lightDir),0.);\n"
"   FragColor = vec4(mix(vec3(0.6,0.55,0.5),col,vec3(0.5)),1.);\n"
"}";

const char* v_guizmos = " #version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"out vec3 FragPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    FragPos = vec3(model * vec4(aPos,1.0));\n"
"    gl_Position = projection * view * vec4(FragPos,1.0);\n"
"}";


const char* f_guizmos = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 FragPos;\n"
"void main()\n"
"{\n"
"   vec3 t = FragPos*10.;\n"
"   vec3 a = vec3(0.8,0.5,0.4);\n"
"   vec3 b = vec3(0.2,0.4,0.2);\n"
"   vec3 c = vec3(2.0,1.0,1.0);\n"
"   vec3 d = vec3(0.0,0.25,0.25);\n"
"   vec3 col = a + b*cos( 6.283185*(c*t+d));\n"
"   FragColor = vec4(col,1.);\n"
"}";

namespace LT_NAMESPACE {
    namespace gl {

    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

    struct vertex {
        glm::vec3 pos;
        glm::vec3 nor;
    };


    GLuint new_program(const char* vs_code, const char* fs_code) {
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);  
        glShaderSource(vertex, 1, &vs_code, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fs_code, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // shader Program
        GLuint shader_id = glCreateProgram();
        glAttachShader(shader_id, vertex);
        glAttachShader(shader_id, fragment);
        glLinkProgram(shader_id);
        checkCompileErrors(shader_id, "PROGRAM");

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return shader_id;
    }

    class Line {
    public:
        glm::mat4 model = glm::mat4(1.);
        std::vector<glm::vec3> vertices = {};
        GLuint render_mode = GL_LINES;
        //  render data
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;

        Line(const std::vector<glm::vec3>& vertices_, GLuint render_mode_ = GL_LINES) {
            vertices = vertices_;
            render_mode = render_mode_;
        }

        ~Line() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }

        void draw(GLuint shader_id, GLenum polygone_mode = GL_FILL)
        {
            glPolygonMode(GL_FRONT_AND_BACK, polygone_mode);
            glUseProgram(shader_id);
            glUniformMatrix4fv(glGetUniformLocation(shader_id, "model"), 1, GL_FALSE, &model[0][0]);
            glBindVertexArray(VAO);
            glDrawArrays(render_mode, 0, vertices.size());
            glBindVertexArray(0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        void setup() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

            glBindVertexArray(0);
        }
    };

    class Mesh {
    public:
        glm::mat4 model;
        std::vector<vertex>       vertices;
        std::vector<unsigned int> indices;
        GLuint render_mode;
        //  render data
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;

        Mesh(std::vector<vertex> vertices_, std::vector<unsigned int> indices_, GLuint render_mode_ = GL_TRIANGLES) {
            vertices = vertices_;
            indices = indices_;
            render_mode = render_mode_;
            model = glm::mat4(1.);
        }

        ~Mesh() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        void draw(GLuint shader_id, GLenum polygone_mode = GL_FILL)
        {
            glLineWidth(3.);
            glPolygonMode(GL_FRONT_AND_BACK, polygone_mode);
            glUseProgram(shader_id);
            glUniformMatrix4fv(glGetUniformLocation(shader_id, "model"), 1, GL_FALSE, &model[0][0]);
            glBindVertexArray(VAO);
            glDrawElements(render_mode, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }



        void setup() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                &indices[0], GL_STATIC_DRAW);

            // vertex positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
            // vertex normals
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, nor));

            glBindVertexArray(0);
        }


    private:
    };


    struct TextureDesc {
        int width = 1;
        int height = 1;
        int mag_filter = GL_LINEAR;
        int min_filter = GL_LINEAR;
    };

    struct Texture {
        TextureDesc desc = {};
        GLuint id = 0;
        void use() const {
            glBindTexture(GL_TEXTURE_2D, id);
        }
        void del() const {
            glDeleteTextures(1, &id);
        }
    };

    Texture new_texture(const TextureDesc& desc) {
        Texture tex;
        tex.desc = desc;
        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, desc.width, desc.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.mag_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc.min_filter);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    struct FramebufferDesc {
        const Texture& tex = {};
        bool add_depth_buffer = false;
    };

    struct Framebuffer {
        GLuint id = 0;
        GLuint depth_id = 0;
        void use() const {
            glBindFramebuffer(GL_FRAMEBUFFER, id);
        }
        void del() const {
            glDeleteFramebuffers(1, &id);
        }
    };

    Framebuffer new_framebuffer(const FramebufferDesc& desc) {
        // Create framebuffer
        Framebuffer fb;
        glGenFramebuffers(1, &fb.id);
        glBindFramebuffer(GL_FRAMEBUFFER, fb.id);

        desc.tex.use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, desc.tex.id, 0);

        // Add depth buffer
        if (desc.add_depth_buffer) {
            glGenRenderbuffers(1, &fb.depth_id);
            glBindRenderbuffer(GL_RENDERBUFFER, fb.depth_id);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, desc.tex.desc.width, desc.tex.desc.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.depth_id);
        }

        // Check if the framebuffer is valid
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return fb;
    }

    class Camera3D {
    public:
        glm::mat4 proj = glm::mat4(1.);
        glm::mat4 view = glm::mat4(1.);
        glm::vec3 pos  = glm::vec3( 0., 0.,-1.);
        glm::vec3 pivot  = glm::vec3( 0., 0., 1.);
    private:
        glm::vec3 right= glm::vec3( 1., 0., 0.);
        glm::vec3 up   = glm::vec3( 0., 1., 0.);
        float theta = 0.;
        float phi   = 0.;
        float dist  = 1.f;
        float aspect = 1.f;

        void update() {
            view = glm::lookAt(pos, pivot, glm::vec3(0., -1., 0.));
            proj = glm::perspective(glm::radians(40.f), -aspect, 0.1f, 100.f);

            glm::vec3 dir = glm::normalize(pivot - pos);
            up = glm::vec3(0., -1., 0.);
            right = glm::normalize(glm::cross(dir, up));
            up = glm::normalize(glm::cross(right, dir));
        }
    public:
        Camera3D() {};
        Camera3D(glm::vec3 position, glm::vec3 pivot, float aspect) : pivot(pivot), pos(position), aspect(aspect) {
            glm::vec3 inv_dir = glm::normalize(position - pivot);
            theta = std::acos(inv_dir.y);
            phi = std::atan2f(inv_dir.z, inv_dir.x);
            update();
        }

        void translate_screen_space(float trans_x, float trans_y) {
            pos += trans_x * right;
            pos += trans_y * up;
            pivot += trans_x * right;
            pivot += trans_y * up;
            update();
        }

        void rotate_around_pivot_point(float drag_x, float drag_y) {
            theta -= drag_y;
            theta = glm::clamp(theta, 0.0001f, 3.141492f);
            phi += drag_x;
            float dist = glm::distance(pos,pivot);
            pos = pivot + dist * glm::vec3(std::cos(phi) * std::sin(theta), std::cos(theta), std::sin(phi) * std::sin(theta));
            update();
        }

        void zoom_to_pivot(float zoom_factor) {
            up = glm::vec3(0., 1., 0.);

            dist = glm::max(0.01f, glm::distance(pos, pivot) + zoom_factor);
            glm::vec3 inv_dir = glm::normalize(pos - pivot);
            pos = pivot + dist * inv_dir;
            update();
        }

    };

    class Scene
    {
    public:
        int idx;
        int res_x, res_y;
        Texture tex;
        Framebuffer fb;
        Camera3D cam;
        GLuint shader_3Dmesh;
        GLuint shader_guizmos;
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Line>> lines;


        Scene(): 
            shader_3Dmesh(0)
            , shader_guizmos(0)
            , res_x(1024)
            , res_y(712)
            , idx(0)
        {
        }

        void setup() {
            setup_meshes();
            setup_lines();
        }

        void setup_meshes() {
            for (std::shared_ptr<Mesh> m : meshes) {
                m->setup();
            }
        }

        void setup_lines() {
            for (std::shared_ptr<Line> l : lines) {
                l->setup();
            }
        }

        void init(){
            tex = new_texture({ .width = res_x, .height = res_y });
            fb  = new_framebuffer({.tex = tex, .add_depth_buffer = true});
            shader_3Dmesh  = new_program(v_3d, f_3d);
            shader_guizmos = new_program(v_guizmos, f_guizmos);
        }

        void draw() {
            GLint aiViewport[4];
            glGetIntegerv(GL_VIEWPORT, aiViewport);

            fb.use();
            glViewport(0, 0, res_x, res_y);
            glClearColor(1., 0., 0., 0.);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
            glEnable(GL_DEPTH_TEST);
        
            glm::vec3 dir = glm::normalize(cam.pos-cam.pivot);

            glUseProgram(shader_3Dmesh);
            glUniformMatrix4fv(glGetUniformLocation(shader_3Dmesh, "view"), 1, GL_FALSE, &cam.view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(shader_3Dmesh, "projection"), 1, GL_FALSE, &cam.proj[0][0]);
            glUniform3f(glGetUniformLocation(shader_3Dmesh, "lightDir"), dir.x,dir.y,dir.z);
            for (std::shared_ptr<Mesh> m : meshes) {
                m->draw(shader_3Dmesh);
            }
  
            glUseProgram(shader_guizmos);
            glUniformMatrix4fv(glGetUniformLocation(shader_guizmos, "view"), 1, GL_FALSE, &cam.view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(shader_guizmos, "projection"), 1, GL_FALSE, &cam.proj[0][0]);
            for (std::shared_ptr<Line> l : lines) {
                l->draw(shader_guizmos);
            }


            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(aiViewport[0], aiViewport[1], (GLsizei)aiViewport[2], (GLsizei)aiViewport[3]);
        }

    private:
    };



    void solid_sphere(std::vector<vertex>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int rings, unsigned int sectors)
    {
        int i, j;
        vertices.clear();
        indices.clear();
        int indicator = 0;
        for (i = 0; i <= rings; i++) {
            double lat0 = glm::pi<double>() * (-0.5 + (double)(i - 1) / rings);
            double z0 = sin(lat0);
            double zr0 = cos(lat0);

            double lat1 = glm::pi<double>() * (-0.5 + (double)i / rings);
            double z1 = sin(lat1);
            double zr1 = cos(lat1);

            for (j = 0; j <= sectors; j++) {
                double lng = 2 * glm::pi<double>() * (double)(j - 1) / sectors;
                double x = cos(lng);
                double y = sin(lng);

                glm::vec3 n1(x * zr0, y * zr0, z0);
                glm::vec3 v1(radius * n1);
                vertices.push_back({ v1,n1 });
                indices.push_back(indicator);
                indicator++;
            
                glm::vec3 n2(x * zr1, y * zr1, z1);
                glm::vec3 v2(radius * n2);

                vertices.push_back({ v2,n2 });
                indices.push_back(indicator);
                indicator++;
            }
            indices.push_back(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        }
    }


    }
}