#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "common/log.hpp"
#include "common/time.hpp"
#include "common/transform3.hpp"
#include "common/vector2.hpp"
#include "common/vector3.hpp"
#include "core/glfw_wrapper.hpp"
#include "graphics/shader.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype/stb_truetype.h"

using namespace notf;

static struct
{
    struct
    {
        const char* font = R"(
            #version 300 es
            #ifdef GL_FRAGMENT_PRECISION_HIGH
            precision highp float;
            #else
            precision mediump float;
            #endif
            in vec4 position;
            in vec2 texCoord0;
            uniform mat4 worldMatrix;
            uniform mat4 viewProjMatrix;
            out vec2 uv0;
            void main()
            {
                gl_Position = viewProjMatrix * worldMatrix * position;
                uv0 = texCoord0;
            }
        )";
    } vertex;

    struct
    {
        const char* font = R"(
            #version 300 es
            #ifdef GL_FRAGMENT_PRECISION_HIGH
            precision highp float;
            #else
            precision mediump float;
            #endif
            uniform sampler2D mainTex;
            in vec2 uv0;
            out vec4 fragColor;
            void main()
            {
                vec4 c = texture(mainTex, uv0);
                fragColor = vec4(c.r, c.r, c.r, c.r);
            }
        )";
    } fragment;
} shaders;

struct GlyphInfo {
    Vector3 positions[4];
    Vector2 uvs[4];
    float offsetX, offsetY;
};

std::vector<uint8_t> readFile(const char* path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        log_critical << "Failed to open file " << path;
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    auto bytes = std::vector<uint8_t>(size);
    file.read(reinterpret_cast<char*>(&bytes[0]), size);
    file.close();
    return bytes;
}

auto compileShader(GLenum type, const char* src) -> GLuint
{
    static std::unordered_map<GLuint, std::string> typeNames = {
        {GL_VERTEX_SHADER, "vertex"},
        {GL_FRAGMENT_SHADER, "fragment"}};

    auto shader = glCreateShader(type);

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        log_critical << "Failed to compile " << typeNames[type] << " shader: " << log.data();
        exit(-1);
    }

    return shader;
}

auto linkProgram(GLuint vs, GLuint fs) -> GLint
{
    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        glDeleteProgram(program);
        log_critical << "Failed to link program: " << log.data();
        exit(-1);
    }

    return program;
}

auto createProgram(const char* vs, const char* fs) -> GLuint
{
    auto vertex   = compileShader(GL_VERTEX_SHADER, vs);
    auto fragment = compileShader(GL_FRAGMENT_SHADER, fs);
    auto program  = linkProgram(vertex, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

struct Example {
    Example(int canvasWidth, int canvasHeight)
        : canvasWidth(canvasWidth)
        , canvasHeight(canvasHeight)
    {
    }

    void initProgram()
    {
        program.handle = createProgram(shaders.vertex.font, shaders.fragment.font);
        glUseProgram(program.handle);
    }

    void initFont()
    {
        auto fontData  = readFile("/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf");
        auto atlasData = std::make_unique<uint8_t[]>(font.atlasWidth * font.atlasHeight);

        font.charInfo = std::make_unique<stbtt_packedchar[]>(font.charCount);

        stbtt_pack_context context;
        if (!stbtt_PackBegin(&context, atlasData.get(), font.atlasWidth, font.atlasHeight, 0, 1, nullptr)) {
            log_critical << "Failed to initialize font";
            exit(-1);
        }

        stbtt_PackSetOversampling(&context, font.oversampleX, font.oversampleY);
        if (!stbtt_PackFontRange(&context, fontData.data(), 0, font.size, font.firstChar, font.charCount, font.charInfo.get())) {
            log_critical << "Failed to pack font";
            exit(-1);
        }

        stbtt_PackEnd(&context);

        glGenTextures(1, &font.texture);
        glBindTexture(GL_TEXTURE_2D, font.texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font.atlasWidth, font.atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasData.get());
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void initUniforms()
    {
        auto viewMatrix       = Transform3::identity();
        auto projectionMatrix = Transform3::perspective(60 * (PI / 180.f), 1.0f * canvasWidth / canvasHeight, 0.05f, 100.0f);
        viewProjMatrix        = projectionMatrix * viewMatrix;

        program.uniforms.viewProjMatrix = glGetUniformLocation(program.handle, "viewProjMatrix");
        program.uniforms.worldMatrix    = glGetUniformLocation(program.handle, "worldMatrix");
        program.uniforms.texture        = glGetUniformLocation(program.handle, "mainTex");
    }

    void initRotatingLabel()
    {
        const std::string text = "Rotating in world space";

        std::vector<Vector3> vertices;
        std::vector<Vector2> uvs;
        std::vector<uint16_t> indexes;

        uint16_t lastIndex = 0;
        float offsetX = 0, offsetY = 0;
        for (auto c : text) {
            auto glyphInfo = getGlyphInfo(c, offsetX, offsetY);
            offsetX        = glyphInfo.offsetX;
            offsetY        = glyphInfo.offsetY;

            vertices.emplace_back(glyphInfo.positions[0]);
            vertices.emplace_back(glyphInfo.positions[1]);
            vertices.emplace_back(glyphInfo.positions[2]);
            vertices.emplace_back(glyphInfo.positions[3]);
            uvs.emplace_back(glyphInfo.uvs[0]);
            uvs.emplace_back(glyphInfo.uvs[1]);
            uvs.emplace_back(glyphInfo.uvs[2]);
            uvs.emplace_back(glyphInfo.uvs[3]);
            indexes.push_back(lastIndex);
            indexes.push_back(lastIndex + 1);
            indexes.push_back(lastIndex + 2);
            indexes.push_back(lastIndex);
            indexes.push_back(lastIndex + 2);
            indexes.push_back(lastIndex + 3);

            lastIndex += 4;
        }

        glGenVertexArrays(1, &rotatingLabel.vao);
        glBindVertexArray(rotatingLabel.vao);

        glGenBuffers(1, &rotatingLabel.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, rotatingLabel.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &rotatingLabel.uvBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, rotatingLabel.uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * uvs.size(), uvs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);

        rotatingLabel.indexElementCount = indexes.size();
        glGenBuffers(1, &rotatingLabel.indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rotatingLabel.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * rotatingLabel.indexElementCount, indexes.data(), GL_STATIC_DRAW);
    }

    void initAtlasQuad()
    {
        const float vertices[] = {
            -1, -1, 0,
            -1, 1, 0,
            1, 1, 0,
            -1, -1, 0,
            1, 1, 0,
            1, -1, 0,
        };

        const float uvs[] = {
            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,
        };

        glGenVertexArrays(1, &atlasQuad.vao);
        glBindVertexArray(atlasQuad.vao);

        glGenBuffers(1, &atlasQuad.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, atlasQuad.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 18, vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &atlasQuad.uvBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, atlasQuad.uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 12, uvs, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
    }

    void renderRotatingLabel(float dt)
    {
        rotatingLabel.angle += dt;

        auto worldMatrix = Transform3::translation(Vector3{0, 5, -30});
        worldMatrix *= Transform3::rotation_y(rotatingLabel.angle);
        worldMatrix *= Transform3::scale(Vector3{0.05f, 0.05f, 1});
        glUniformMatrix4fv(program.uniforms.worldMatrix, 1, GL_FALSE, worldMatrix.as_ptr());

        glBindVertexArray(rotatingLabel.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rotatingLabel.indexBuffer);
        glDrawElements(GL_TRIANGLES, rotatingLabel.indexElementCount, GL_UNSIGNED_SHORT, nullptr);
    }

    void renderAtlasQuad(float dt)
    {
        atlasQuad.time += dt;
        auto distance = -10 - 5 * sinf(atlasQuad.time);

        auto worldMatrix = Transform3::translation(Vector3{0, -6, distance});
        worldMatrix *= Transform3::scale(Vector3{6, 6, 1});
        glUniformMatrix4fv(program.uniforms.worldMatrix, 1, GL_FALSE, worldMatrix.as_ptr());

        glBindVertexArray(atlasQuad.vao);
        glDrawArrays(GL_TRIANGLES, 0, 6); // 6 vertices
    }

    GlyphInfo getGlyphInfo(uint32_t character, float offsetX, float offsetY)
    {
        stbtt_aligned_quad quad;

        stbtt_GetPackedQuad(font.charInfo.get(), font.atlasWidth, font.atlasHeight, character - font.firstChar, &offsetX, &offsetY, &quad, 1);
        auto xmin = quad.x0;
        auto xmax = quad.x1;
        auto ymin = -quad.y1;
        auto ymax = -quad.y0;

        auto info         = GlyphInfo();
        info.offsetX      = offsetX;
        info.offsetY      = offsetY;
        info.positions[0] = {xmin, ymin, 0};
        info.positions[1] = {xmin, ymax, 0};
        info.positions[2] = {xmax, ymax, 0};
        info.positions[3] = {xmax, ymin, 0};
        info.uvs[0]       = {quad.s0, quad.t1};
        info.uvs[1]       = {quad.s0, quad.t0};
        info.uvs[2]       = {quad.s1, quad.t0};
        info.uvs[3]       = {quad.s1, quad.t1};

        return info;
    }

    void init()
    {
        initFont();
        initRotatingLabel();
        initAtlasQuad();
        initProgram();
        initUniforms();
    }

    void shutdown()
    {
        glDeleteVertexArrays(1, &rotatingLabel.vao);
        glDeleteBuffers(1, &rotatingLabel.vertexBuffer);
        glDeleteBuffers(1, &rotatingLabel.uvBuffer);
        glDeleteBuffers(1, &rotatingLabel.indexBuffer);
        glDeleteVertexArrays(1, &atlasQuad.vao);
        glDeleteBuffers(1, &atlasQuad.vertexBuffer);
        glDeleteBuffers(1, &atlasQuad.uvBuffer);
        glDeleteTextures(1, &font.texture);
        glDeleteProgram(program.handle);
    }

    void render(float dt)
    {
        glViewport(0, 0, canvasWidth, canvasHeight);
        glClearColor(0, 0.5f, 0.6f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setting some state
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(program.handle);

        glUniformMatrix4fv(program.uniforms.viewProjMatrix, 1, GL_FALSE, viewProjMatrix.as_ptr());

        glBindTexture(GL_TEXTURE_2D, font.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(program.uniforms.texture, 0);

        renderRotatingLabel(dt);
        renderAtlasQuad(dt);
    }

    struct
    {
        GLuint handle = 0;
        struct
        {
            GLuint viewProjMatrix = 0;
            GLuint worldMatrix    = 0;
            GLuint texture        = 0;
        } uniforms;
    } program;

    Transform3 viewProjMatrix;

    struct
    {
        GLuint vao                 = 0;
        GLuint vertexBuffer        = 0;
        GLuint uvBuffer            = 0;
        GLuint indexBuffer         = 0;
        uint16_t indexElementCount = 0;
        float angle                = 0;
    } rotatingLabel;

    struct
    {
        GLuint vao          = 0;
        GLuint vertexBuffer = 0;
        GLuint uvBuffer     = 0;
        float time          = 0;
    } atlasQuad;

    struct
    {
        const uint32_t size        = 40;
        const uint32_t atlasWidth  = 1024;
        const uint32_t atlasHeight = 1024;
        const uint32_t oversampleX = 2;
        const uint32_t oversampleY = 2;
        const uint32_t firstChar   = ' ';
        const uint32_t charCount   = '~' - ' ';
        std::unique_ptr<stbtt_packedchar[]> charInfo;
        GLuint texture = 0;
    } font;

    int canvasWidth;
    int canvasHeight;
};

int main(void)
{
    GLFWwindow* window;

    // install the log handler first, to catch errors right away
    std::unique_ptr<LogHandler> log_handler = std::make_unique<LogHandler>(128, 200);
    install_log_message_handler(std::bind(&LogHandler::push_log, log_handler.get(), std::placeholders::_1));
    log_handler->start();

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    Time::set_frequency(glfwGetTimerFrequency());

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    //    Shader shader = Shader::build("font_shader", shaders.vertex.font, shaders.fragment.font);
    //    if (!shader.is_valid()) {
    //        return -1;
    //    }
    //    shader.use();

    Example example(640, 480);
    example.init();

    Time last_time = Time::now();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        //        glClear(GL_COLOR_BUFFER_BIT);

        Time current_time = Time::now();
        Time delta        = current_time.since(last_time);
        last_time         = current_time;

        example.render(delta.in_seconds());

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    /* Shutdown */
    example.shutdown();
    log_handler->stop();
    log_handler->join();
    glfwTerminate();
    return 0;
}
