#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Terrain.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
vector<Model *> models;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool wireframe = false;
bool nightMode = false;
glm::vec3 sunClear = glm::vec3(1.0f, 0.99f, 0.96f);//glm::vec3(1.0f, 0.894f, 0.859f);
glm::vec3 nightClear = glm::vec3(0.098f, 0.098f, 0.4392f);
int timeout = 10;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float sunX = 0.0f;
float debugTime = 0.0f;


class Application
{
public:
    GLFWwindow* window = nullptr;
    Model* toothless = nullptr;
    GLuint fb_screen;
    Shader* terrainShader, *modelShader, *lightingShader, *bloomShader, *screenShader, *prog_bloom_pass;
    Terrain *ground;
    glm::mat4 projection, view;
    float currentFrame;
    GLuint texColBuffer, texColBuffer2, texPosBuffer, texNorBuffer, texMatBuffer, texDepthbuffer;
    GLuint FBO_bloom, FBO_bloom_pass[2], texFBO_bloom_pass[2], texFBO_bloom_rest;
    GLuint quadVAO, quadVBO;
    int framebufferHeight, framebufferWidth;

    int init()
    {

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1;
        }

        cout << glGetString(GL_VERSION) << endl;

        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);

        // build and compile shaders
        // -------------------------
        modelShader = new Shader("./resources/animate.vert", "./resources/animate.frag");
        terrainShader = new Shader("./resources/terrain.vert", "./resources/terrain.frag");
        //Shader stillModelShader("./resources/still.vert", "./resources/still.frag");
        lightingShader = new Shader("./resources/fbo.vert", "./resources/fbo.frag");
        screenShader = new Shader("./resources/general.vert", "./resources/screen.frag");
        prog_bloom_pass = new Shader("./resources/general.vert", "./resources/bloom_pass.frag");

        ground = new Terrain("./resources/terrain/testtopo.png");

        // load models
        toothless = new Model("./resources/models/toothlessGLTF/scene.gltf", false, true);
        models.push_back(toothless);
        toothless->position = glm::vec3(0.0f, -0.5f, -3.0f);

        // quad for framebuffer
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


        GLuint Tex1Location = glGetUniformLocation(lightingShader->ID, "colTex");//tex, tex2... sampler in the fragment shader
        GLuint Tex2Location = glGetUniformLocation(lightingShader->ID, "posTex");
        GLuint Tex3Location = glGetUniformLocation(lightingShader->ID, "norTex");
        GLuint Tex4Location = glGetUniformLocation(lightingShader->ID, "matTex");
        GLuint Tex5Location = glGetUniformLocation(lightingShader->ID, "depthTexture");
        // Then bind the uniform samplers to texture units:
        glUseProgram(lightingShader->ID);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);
        glUniform1i(Tex3Location, 2);
        glUniform1i(Tex4Location, 3);
        glUniform1i(Tex5Location, 4);

        Tex1Location = glGetUniformLocation(lightingShader->ID, "colTex");//tex, tex2... sampler in the fragment shader
        Tex2Location = glGetUniformLocation(lightingShader->ID, "bloomTex");
        // Then bind the uniform samplers to texture units:
        glUseProgram(screenShader->ID);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);


        // render to framebuffer for deferred shading
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        glGenFramebuffers(1, &fb_screen);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_screen);
        // create a color texture
        glGenTextures(1, &texColBuffer);
        glBindTexture(GL_TEXTURE_2D, texColBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColBuffer, 0);

        // create a position texture
        glGenTextures(1, &texPosBuffer);
        glBindTexture(GL_TEXTURE_2D, texPosBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texPosBuffer, 0);

        // create a normal texture
        glGenTextures(1, &texNorBuffer);
        glBindTexture(GL_TEXTURE_2D, texNorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texNorBuffer, 0);

        // create a material texture
        glGenTextures(1, &texMatBuffer);
        glBindTexture(GL_TEXTURE_2D, texMatBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, texMatBuffer, 0);

        glGenTextures(1, &texDepthbuffer);
        glBindTexture(GL_TEXTURE_2D, texDepthbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, framebufferWidth, framebufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texDepthbuffer, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        generate_bloom_FBO();
    }

    void generate_bloom_FBO()
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        //RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &texFBO_bloom_pass[0]);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_pass[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);


        glGenTextures(1, &texFBO_bloom_pass[1]);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_pass[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &texFBO_bloom_rest);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_rest);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint depth_rbloc;
        //-------------------------
        glGenFramebuffers(1, &FBO_bloom);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_bloom);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texFBO_bloom_rest, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texFBO_bloom_pass[0], 0);
        glGenRenderbuffers(1, &depth_rbloc);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbloc);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbloc);

        //-------------------------
        for (int i = 0; i < 2; i++)
        {
            glGenFramebuffers(1, &FBO_bloom_pass[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO_bloom_pass[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texFBO_bloom_pass[i], 0);
            glGenRenderbuffers(1, &depth_rbloc);
            glBindRenderbuffer(GL_RENDERBUFFER, depth_rbloc);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbloc);
        }


    }

    void setup_render()
    {
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // per-frame time logic
        // --------------------
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        timeout--;
        processInput(window);

        toothless->debugTime = debugTime;
        toothless->updatePosition(deltaTime);

        camera.TrackModel(toothless->position, toothless->direction);

        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 v = camera.GetViewMatrix();
        view = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), toothless->direction);
        view *= v;
    }

    void render_to_texture()
    {
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_screen);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, buffers);
        glEnable(GL_DEPTH_TEST);

        //******************************************************************
        if (nightMode) {
            glClearColor(nightClear.x, nightClear.y, nightClear.z, 1.0);
        }
        else {
            glClearColor(sunClear.x, sunClear.y, sunClear.z, 1.0);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the ground
        terrainShader->use();
        terrainShader->setMat4("projection", projection);
        terrainShader->setMat4("view", view);
        ground->Draw(terrainShader);

        modelShader->use();
        // view/projection transformations
        modelShader->setMat4("projection", projection);
        modelShader->setMat4("view", view);
        // render the loaded model
        toothless->Draw(modelShader, currentFrame);
    }

    void render_lighting()
    {
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_bloom);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
        glClear(GL_COLOR_BUFFER_BIT);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, buffers);

        lightingShader->use();
        glBindVertexArray(quadVAO);

        lightingShader->setVec3("skyColor", nightMode ? nightClear : sunClear);
        lightingShader->setVec3("campos", camera.Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texPosBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texNorBuffer);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, texMatBuffer);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, texDepthbuffer);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_rest);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_pass[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void render_to_screen()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0, 1.0, 0.0, 1.0);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        screenShader->use();
        glBindVertexArray(quadVAO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_rest);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_pass[0]);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void render_bloompass(int pass)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_bloom_pass[pass]);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(1, buffers);
        // Get current frame buffer size.

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        prog_bloom_pass->use();
        prog_bloom_pass->setInt("horizontal", pass);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_pass[!pass]);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, texFBO_bloom_pass[pass]);
        glGenerateMipmap(GL_TEXTURE_2D);


    }
};


int main()
{
    // glfw window creation
    // --------------------
    if (!glfwInit()) {
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(1600, 900, "Hello, World!", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);


    Application* app = new Application();
    app->window = window;
    app->init();
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glfwSwapBuffers(window);
        glfwPollEvents();

        app->setup_render();
        app->render_to_texture();
        app->render_lighting();
        app->render_bloompass(0);
        app->render_bloompass(1);
        app->render_bloompass(0);
        app->render_bloompass(1);
        app->render_to_screen();

    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        sunX += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        if (timeout <= 0) {
            nightMode = !nightMode;
            timeout = 10;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        sunX -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        debugTime += 0.1;
        //wing rotation problem happens between 6.5 and 7.5 seconds...
        cout << "dtime: " << debugTime << endl;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    //camera.ProcessMouseMovement(xoffset, yoffset);
    for (auto it = models.begin(); it != models.end(); it++) {
        (*it)->ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
