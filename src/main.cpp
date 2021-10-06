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

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello, World!", NULL, NULL);
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

    glfwSwapInterval(1);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    Shader modelShader("./resources/animate.vert", "./resources/animate.frag");
    Shader terrainShader("./resources/terrain.vert", "./resources/terrain.frag");
    Shader stillModelShader("./resources/still.vert", "./resources/still.frag");
    Shader fboQuadShader("./resources/fbo.vert", "./resources/fbo.frag");
    
    Terrain ground("./resources/terrain/testtopo.png");

    // load models
    Model ourModel("./resources/models/toothlessGLTF/scene.gltf", false, true);
    models.push_back(&ourModel);
    ourModel.position = glm::vec3(0.0f, -0.5f, -3.0f);

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
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    

    GLuint Tex1Location = glGetUniformLocation(fboQuadShader.ID, "colTex");//tex, tex2... sampler in the fragment shader
    GLuint Tex2Location = glGetUniformLocation(fboQuadShader.ID, "posTex");
    GLuint Tex3Location = glGetUniformLocation(fboQuadShader.ID, "norTex");
    GLuint Tex4Location = glGetUniformLocation(fboQuadShader.ID, "matTex");
    GLuint Tex5Location = glGetUniformLocation(fboQuadShader.ID, "depthTexture");
    // Then bind the uniform samplers to texture units:
    glUseProgram(fboQuadShader.ID);
    glUniform1i(Tex1Location, 0);
    glUniform1i(Tex2Location, 1);
    glUniform1i(Tex3Location, 2);
    glUniform1i(Tex4Location, 3);
    glUniform1i(Tex5Location, 4);


    // render to framebuffer for deferred shading
    int framebufferHeight, framebufferWidth;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color texture
    unsigned int texColBuffer;
    glGenTextures(1, &texColBuffer);
    glBindTexture(GL_TEXTURE_2D, texColBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColBuffer, 0);

    // create a position texture
    unsigned int texPosBuffer;
    glGenTextures(1, &texPosBuffer);
    glBindTexture(GL_TEXTURE_2D, texPosBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texPosBuffer, 0);

    // create a normal texture
    unsigned int texNorBuffer;
    glGenTextures(1, &texNorBuffer);
    glBindTexture(GL_TEXTURE_2D, texNorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texNorBuffer, 0);

    // create a material texture
    unsigned int texMatBuffer;
    glGenTextures(1, &texMatBuffer);
    glBindTexture(GL_TEXTURE_2D, texMatBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, texMatBuffer, 0);

    unsigned int textureDepthbuffer;
    glGenTextures(1, &textureDepthbuffer);
    glBindTexture(GL_TEXTURE_2D, textureDepthbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT32F, framebufferWidth, framebufferHeight, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepthbuffer, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        timeout--;
        processInput(window);
        
        ourModel.debugTime = debugTime;
        ourModel.updatePosition(deltaTime);
        
        camera.TrackModel(ourModel.position, ourModel.direction);
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 v = camera.GetViewMatrix();
        glm::mat4 view = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), ourModel.direction);
        view *= v;
        

        // render to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
        glDrawBuffers(4, buffers);
        glEnable(GL_DEPTH_TEST);
        
        //******************************************************************
        if (nightMode) {
            glClearColor(nightClear.x, nightClear.y, nightClear.z, 1.0);
        } else {
            glClearColor(sunClear.x, sunClear.y, sunClear.z, 1.0);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // draw the ground
        terrainShader.use();
        terrainShader.setMat4("projection", projection);
        terrainShader.setMat4("view", view);
        ground.Draw(terrainShader);
        
        modelShader.use();
        // view/projection transformations
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        // render the loaded model
        ourModel.Draw(modelShader, currentFrame);

        //*******************************************************************

        // render framebuffer to quad
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
        glClear(GL_COLOR_BUFFER_BIT);

        fboQuadShader.use();
        glBindVertexArray(quadVAO);
        
        fboQuadShader.setVec3("skyColor", nightMode ? nightClear : sunClear);
        fboQuadShader.setVec3("campos", camera.Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texPosBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texNorBuffer);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, texMatBuffer);
        //glUniform1i(glGetUniformLocation(fboQuadShader.ID, "screenTexture"),0);
        
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, textureDepthbuffer);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();

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
