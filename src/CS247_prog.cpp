// CS 247 - Scientific Visualization, KAUST
//
// Programming Assignment #1

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

// framework includes
#include "glslprogram.h"

// window size
const unsigned int gWindowWidth = 512;
const unsigned int gWindowHeight = 512;

// Declarations
void loadData(const char* filename);
void downloadVolumeAsTexture();

unsigned short* data_array;
unsigned short vol_dim[3];
int current_axis;
int current_slice[3];
bool data_loaded;

// framebuffer size (updated on resize, in pixels)
int gFbWidth = gWindowWidth;
int gFbHeight = gWindowHeight;

GLuint texture;
GLuint VBO[2], VAO, EBO;
GLSLProgram program;
glm::mat4 model;

// Try the given relative path with a few common prefixes so the program works
// whether you launch it from the project root, the build dir, or a nested
// Debug/Release folder.
static std::string findFile(const std::string& rel) {
    const std::string prefixes[] = {
        "", "./", "../", "../../", "../../../", "../../../../"
    };
    for (const auto& p : prefixes) {
        std::string candidate = p + rel;
        FILE* fp = fopen(candidate.c_str(), "rb");
        if (fp) {
            fclose(fp);
            return candidate;
        }
    }
    return rel; // give up; let caller emit the error
}

// Cycle clear colors
static void nextClearColor(void)
{
    static int color = 0;

    switch(color++)
    {
        case 0:
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            break;
        case 1:
            glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
            break;
        default:
            glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
            color = 0;
            break;
    }
}

// callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    gFbWidth = width;
    gFbHeight = height;
    glViewport(0, 0, width, height);
}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                std::cout << "Bye :D" << std::endl;
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_W:
                if (data_loaded) {
                    current_slice[current_axis] = std::min((current_slice[current_axis] + 1),  vol_dim[current_axis] - 1);
                    fprintf(stderr, "increasing current slice: %i\n", current_slice[current_axis]);
                }
                break;
            case GLFW_KEY_S:
                if (data_loaded) {
                    current_slice[current_axis] = std::max((current_slice[current_axis] - 1), 0);
                    fprintf(stderr, "decreasing current slice: %i\n", current_slice[current_axis]);
                }
                break;
            case GLFW_KEY_A: // optional
                current_axis = ((current_axis + 1) % 3);
                fprintf(stderr, "toggling viewing axis to: %i\n", current_axis);
                break;
            case GLFW_KEY_1:
                loadData(findFile("data/lobster.dat").c_str());
                break;
            case GLFW_KEY_2:
                loadData(findFile("data/skewed_head.dat").c_str());
                break;
            case GLFW_KEY_B:
                nextClearColor();
                break;
            default:
                fprintf(stderr, "\nKeyboard commands:\n\n"
                                "b - Toggle among background clear colors\n"
                                "w - Increase current slice\n"
                                "s - Decrease current slice\n"
                                "a - Toggle viewing axis\n"
                                "1 - Load lobster dataset\n"
                                "2 - Load head dataset\n");
                break;

        }
    }
}


// glfw error callback
static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


// Data
void loadData(const char* filename)
{
    fprintf(stderr, "loading data %s\n", filename);

    FILE* fp;
    fp = fopen(filename, "rb");// open file, read only, binary mode
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file %s for reading.\n", filename);
        return;
    }

    //read volume dimension
    fread(&vol_dim[0], sizeof(unsigned short), 1, fp);
    fread(&vol_dim[1], sizeof(unsigned short), 1, fp);
    fread(&vol_dim[2], sizeof(unsigned short), 1, fp);

    fprintf(stderr, "volume dimensions: x: %i, y: %i, z:%i \n", vol_dim[0], vol_dim[1], vol_dim[2]);

    if (data_array != NULL) {
        delete[] data_array;
    }

    data_array = new unsigned short[vol_dim[0] * vol_dim[1] * vol_dim[2]]; //for intensity volume

    for(int z = 0; z < vol_dim[2]; z++) {
        for(int y = 0; y < vol_dim[1]; y++) {
            for(int x = 0; x < vol_dim[0]; x++) {

                fread(&data_array[x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1])], sizeof(unsigned short), 1, fp);
                data_array[x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1])] = data_array[x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1])] << 4;
            }
        }
    }

    fclose(fp);

    current_slice[0] = vol_dim[0] / 2;
    current_slice[1] = vol_dim[1] / 2;
    current_slice[2] = vol_dim[2] / 2;

    downloadVolumeAsTexture();

    data_loaded = true;
}

void downloadVolumeAsTexture()
{
    fprintf(stderr, "downloading volume to 3D texture\n");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, texture);

    // bilinear filtering inside a slice, clamp at the boundaries
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

    // single-channel 16-bit data — core profile has no GL_LUMINANCE
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16,
                 vol_dim[0], vol_dim[1], vol_dim[2], 0,
                 GL_RED, GL_UNSIGNED_SHORT, data_array);
}


// init application
// - load application specific data
// - set application specific parameters
// - initialize stuff
bool initApplication(int argc, char **argv)
{

    std::string version((const char *)glGetString(GL_VERSION));
    std::stringstream stream(version);
    unsigned major, minor;
    char dot;

    stream >> major >> dot >> minor;

    assert(dot == '.');
    if (major > 3 || (major == 2 && minor >= 0)) {
        std::cout << "OpenGL Version " << major << "." << minor << std::endl;
    } else {
        std::cout << "The minimum required OpenGL version is not supported on this machine. Supported is only " << major << "." << minor << std::endl;
        return false;
    }

    // default initialization
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, gFbWidth, gFbHeight);

    return true;
}


// set up the scene: shaders, VAO, ..etc
void setup() {
    // compile & link the shaders
    program.compileShader(findFile("shaders/vertex.vs").c_str());
    program.compileShader(findFile("shaders/fragment.fs").c_str());
    program.link();

    // A full-screen quad in NDC (z = 0). Each vertex carries a 2D-ish texture
    // coordinate in [0,1]; the third component is unused (set to 0) and the
    // 3D sampling location is built per-frame from a texture-transform matrix.
    float quadVertices[] = {
        // pos              // tex
        -1.f, -1.f, 0.f,    0.f, 0.f, 0.f,
         1.f, -1.f, 0.f,    1.f, 0.f, 0.f,
         1.f,  1.f, 0.f,    1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f,    0.f, 1.f, 0.f,
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 3D volume texture handle (data uploaded later in loadData)
    glGenTextures(1, &texture);
}

// Build the aspect-correcting matrix that scales the unit quad so the slice
// fills the window without distorting its data aspect ratio.
static glm::mat4 buildModelMatrix() {
    if (!data_loaded) return glm::mat4(1.0f);

    float sliceW = 1.f, sliceH = 1.f;
    switch (current_axis) {
        case 0: sliceW = vol_dim[1]; sliceH = vol_dim[2]; break; // YZ
        case 1: sliceW = vol_dim[0]; sliceH = vol_dim[2]; break; // XZ
        case 2: sliceW = vol_dim[0]; sliceH = vol_dim[1]; break; // XY
    }
    const float sliceAspect = sliceW / sliceH;
    const float winAspect   = (float)gFbWidth / (float)gFbHeight;

    float sx = 1.f, sy = 1.f;
    if (winAspect > sliceAspect) sx = sliceAspect / winAspect;
    else                          sy = winAspect   / sliceAspect;

    return glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, 1.0f));
}

// Build the texture-coordinate transform. The incoming attribute carries
// (u, v, 0, 1) over the quad; we map it to a 3D sample location whose
// constant component is the current slice (in [0,1]).
static glm::mat4 buildTexMatrix() {
    if (!data_loaded) return glm::mat4(1.0f);

    // sample at voxel center
    const float sx = (current_slice[0] + 0.5f) / (float)vol_dim[0];
    const float sy = (current_slice[1] + 0.5f) / (float)vol_dim[1];
    const float sz = (current_slice[2] + 0.5f) / (float)vol_dim[2];

    glm::mat4 m(0.0f);
    m[3][3] = 1.0f;
    switch (current_axis) {
        case 0: // YZ slice: tex = (sliceX, u, v)
            m[0][1] = 1.0f;          // u -> y
            m[1][2] = 1.0f;          // v -> z
            m[3][0] = sx;            // const x
            break;
        case 1: // XZ slice: tex = (u, sliceY, v)
            m[0][0] = 1.0f;          // u -> x
            m[1][2] = 1.0f;          // v -> z
            m[3][1] = sy;            // const y
            break;
        case 2: // XY slice: tex = (u, v, sliceZ)
            m[0][0] = 1.0f;          // u -> x
            m[1][1] = 1.0f;          // v -> y
            m[3][2] = sz;            // const z
            break;
    }
    return m;
}

// render a frame
void render() {
    if (!data_loaded) return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, texture);

    program.use();
    program.setUniform("model",        buildModelMatrix());
    program.setUniform("texTransform", buildTexMatrix());
    program.setUniform("volume",       0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// entry point
int main(int argc, char** argv)
{
    // initialize variables
    data_array = NULL;
    current_slice[0] = 0;
    current_slice[1] = 0;
    current_slice[2] = 0;
    current_axis = 2;
    data_loaded = false;


    // set glfw error callback
    glfwSetErrorCallback(errorCallback);

    // init glfw
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    // Request an OpenGL 4.1 core context — the highest macOS supports — and a
    // forward-compatible profile so deprecated entry points are off.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // init glfw window
    GLFWwindow* window;
    window = glfwCreateWindow(gWindowWidth, gWindowHeight, "CS247 - Scientific Visualization - Slice Viewer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // set GLFW callback functions
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // make context current (once is sufficient)
    glfwMakeContextCurrent(window);

    // get the frame buffer size
    glfwGetFramebufferSize(window, &gFbWidth, &gFbHeight);

    // init the OpenGL API (we need to do this once before any calls to the OpenGL API)
    gladLoadGL();

    // init our application
    if (!initApplication(argc, argv)) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }


    // set up the scene
    setup();

    // print menu
    keyCallback(window, GLFW_KEY_BACKSLASH, 0, GLFW_PRESS, 0);

    // load a default dataset so the window isn't empty on startup
    loadData(findFile("data/lobster.dat").c_str());

    // start traversing the main loop
    // loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // clear frame buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render one frame
        render();

        // swap front and back buffers
        glfwSwapBuffers(window);

        // poll and process input events (keyboard, mouse, window, ...)
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
