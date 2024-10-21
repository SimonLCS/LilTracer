#include <iostream>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Mesh {
public:
    glm::mat4 model;
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    GLuint render_mode;

    Mesh(std::vector<Vertex> vertices_, std::vector<unsigned int> indices_, GLuint render_mode_ = GL_TRIANGLES) {
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

    void draw(GLuint shader_id)
    {
        glUseProgram(shader_id);
        glUniformMatrix4fv(glGetUniformLocation(shader_id, "model"), 1, GL_FALSE, &model[0][0]);
        glBindVertexArray(VAO);
        glDrawElements(render_mode, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    //  render data
    unsigned int VAO, VBO, EBO;

    void setup_mesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
            &indices[0], GL_STATIC_DRAW);

        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glBindVertexArray(0);

    } 
private:
};


class OpenglScene
{
public:
    int idx;
    int res_x, res_y;
    GLuint fb_id, depth_id, tex_id;
    glm::mat4 proj;
    glm::mat4 view;
    GLuint shader_id;
    std::vector<std::shared_ptr<Mesh>> meshes;
    // Camera
    glm::vec3 pos;
    glm::vec3 dir;
    glm::vec3 right;
    glm::vec3 up;
    float dist;


    OpenglScene():shader_id(0), idx(0) {
        res_x = 1024;
        res_y = 712;
        proj = glm::perspective(glm::radians(40.f), -float(res_x)/float(res_y), 0.1f, 100.f);
        pos = glm::vec3(6.);
        dir = glm::vec3(-1.);
        dist = 1.;
        up = glm::vec3(0., 0., 1.);
        right = glm::normalize(glm::cross(dir, up));
        up = glm::normalize(glm::cross(right, dir));
        view = glm::lookAt(glm::vec3(6.), glm::vec3(0.), up);
    }

    void setup() {
        for (std::shared_ptr<Mesh> m : meshes) {
            m->setup_mesh();
        }
    }

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

    void init(){

        // Setup framebuffer for post process
        glGenFramebuffers(1, &fb_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_id);

        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res_x, res_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
        

        glGenRenderbuffers(1, &depth_id);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, res_x, res_y); // use a single renderbuffer object for both a depth AND stencil buffer.
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_id);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        const char* vShaderCode = " #version 330 core\n"
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
            "    Normal = vec3(transpose(model) * vec4(aNormal,1.));\n"
            "    gl_Position = projection * view * vec4(FragPos,1.0);\n"
            "}";

        const char* fShaderCode = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec3 FragPos;\n"
            "in vec3 Normal;\n"
            "uniform vec3 lightDir;\n"
            "void main()\n"
            "{\n"
            "   vec3 col = vec3(1.) * max(dot(Normal, -lightDir),0.);\n"
            "   FragColor = vec4(Normal,1.);\n"
            "}";

        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // shader Program
        shader_id = glCreateProgram();
        glAttachShader(shader_id, vertex);
        glAttachShader(shader_id, fragment);
        glLinkProgram(shader_id);
        checkCompileErrors(shader_id, "PROGRAM");

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);

    }

    void draw() {
        GLint aiViewport[4];
        glGetIntegerv(GL_VIEWPORT, aiViewport);

        glBindFramebuffer(GL_FRAMEBUFFER,fb_id);
        glViewport(0, 0, res_x, res_y);
        glClearColor(1., 0., 0., 0.);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
        glEnable(GL_DEPTH_TEST);
        glUseProgram(shader_id);
        glUniformMatrix4fv(glGetUniformLocation(shader_id, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader_id, "projection"), 1, GL_FALSE, &proj[0][0]);
        glUniform3f(glGetUniformLocation(shader_id, "lightDir"), 0.,0.,-1.);
        
        for (std::shared_ptr<Mesh> m : meshes) {
            m->draw(shader_id);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(aiViewport[0], aiViewport[1], (GLsizei)aiViewport[2], (GLsizei)aiViewport[3]);
    }

private:
};





void solid_sphere(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int rings, unsigned int sectors)
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