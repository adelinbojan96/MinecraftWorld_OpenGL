#include <iostream>
#include <string>

#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstdlib>

gps::Window myWindow;
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::vec3 lightDir;
glm::vec3 lightColor;
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint fogDensityLoc;

gps::Camera myCamera(glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));
GLfloat cameraSpeed = 0.1f;
GLboolean pressedKeys[1024];

gps::Model3D mapModel;
gps::Model3D creeperModel;
gps::Model3D villagerModel;
gps::Model3D herobrineModel;

//triangles of the map model for collision detection
std::vector<glm::vec3> mapTriangles;

glm::vec3 creeperStartPos(-45.00f, -15.00f, 7.0f);
glm::vec3 creeperEndPos(-45.00f, -15.00f, 24.0f);

glm::mat4 lightSpaceMatrix;

static const glm::vec3 villagerWaypoints[] = {
    glm::vec3(-18.5554f, -1.1f, -12.0467f),
    glm::vec3(-18.6130f, -1.1f, -23.4756f),
    glm::vec3(-13.9174f, -1.1f, -23.4852f),
    glm::vec3(-14.0591f, -1.1f, -12.8980f),
    glm::vec3(-18.5554f, -1.1f, -12.0467f)
};
static const int villagerWaypointCount = 5;

int villagerCurrentIndex = 0;
int villagerNextIndex = 1;
float villagerAnimProgress = 0.0f;
float villagerAnimSpeed = 0.002f;
float villagerAngle = 180.0f;
glm::vec3 villagerPos = villagerWaypoints[0];

bool hasTeleportedBack = false;
const glm::vec3 teleportBackMin = glm::vec3(-9.02183f, -199.745f, 6.77691f);
const glm::vec3 teleportBackMax = glm::vec3(-6.74821f, -196.597f, 6.97805f);
const glm::vec3 overworldTeleportPos = glm::vec3(-14.2026f, 1.55484f, 13.8633f);

float creeperAnimProgress = 0.0f;
bool goingForward = true;
float creeperAngle = 0.0f;
float creeperAnimSpeed = 0.001f;

glm::vec3 herobrinePos = glm::vec3(-45.2326f, -16.00f, -36.8227f);
glm::vec3 mainSunLightPos = glm::vec3(43.8828f, 41.9042f, 17.4612f); //main light source
glm::vec3 mainSunLightColor = glm::vec3(2.0f, 2.0f, 2.0f);

GLfloat angle;
gps::Shader basicShader;

glm::vec3 glowstoneLightPos = glm::vec3(-3.5409f, -195.104f, -1.93046f);
glm::vec3 glowstoneLightColor = glm::vec3(2.0f, 2.0f, 2.0f);

glm::vec3 sunLightPos = glm::vec3(43.8828f, 41.9042f, 17.4612f);
glm::vec3 sunLightColor = glm::vec3(1.0f, 1.0f, 0.9f);

float orthoBoxSize = 5.0f;

float nearPlane = 1.0f;
float farPlane = 300.0f;
glm::mat4 lightProjection = glm::ortho(
    -orthoBoxSize, orthoBoxSize,
    -orthoBoxSize, orthoBoxSize,
    nearPlane, farPlane
);

GLuint shadowMapLoc;
GLint lightSpaceMatrixLoc;
glm::mat4 lastHerobrineLightLSM;

GLuint depthMapFBO;
GLuint depthMapTexture;
gps::Shader depthMapShader;
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

float creeperHeightScale = 1.0f; 
const float HEIGHT_SCALE_INCREMENT = 0.1f; 
const float MAX_HEIGHT_SCALE = 3.0f; 
const float MIN_HEIGHT_SCALE = 0.5f; 

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        if (errorCode == GL_INVALID_ENUM) error = "INVALID_ENUM";
        if (errorCode == GL_INVALID_VALUE) error = "INVALID_VALUE";
        if (errorCode == GL_INVALID_OPERATION) error = "INVALID_OPERATION";
        if (errorCode == GL_OUT_OF_MEMORY) error = "OUT_OF_MEMORY";
        if (errorCode == GL_INVALID_FRAMEBUFFER_OPERATION) error = "INVALID_FRAMEBUFFER_OPERATION";
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    float aspect = (float)width / (float)height;
    projection = glm::perspective(glm::radians(90.0f), aspect, 0.5f, 10000.0f);
    basicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
            if (key == GLFW_KEY_1) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            if (key == GLFW_KEY_2) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            if (key == GLFW_KEY_3) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                glPointSize(5.0f);
            }
        
            if (key == GLFW_KEY_I) {
                creeperHeightScale += HEIGHT_SCALE_INCREMENT;
                if (creeperHeightScale > MAX_HEIGHT_SCALE)
                {
                    creeperStartPos.y -= 0.2;
                    creeperEndPos.y -= 0.2;
                    creeperHeightScale = MAX_HEIGHT_SCALE;
                }
                creeperStartPos.y += 0.2;
                creeperEndPos.y += 0.2;
            }
            if (key == GLFW_KEY_K) { 
                creeperHeightScale -= HEIGHT_SCALE_INCREMENT;
                if (creeperHeightScale < MIN_HEIGHT_SCALE)
                {
                    creeperHeightScale = MIN_HEIGHT_SCALE;
                    creeperStartPos.y += 0.2;
                    creeperEndPos.y += 0.2;
                }
                creeperStartPos.y -= 0.2;
                creeperEndPos.y -= 0.2;
            }
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = 512.0f;
    static float lastY = 384.0f;
    static bool firstMouse = true;
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    myCamera.rotate(yoffset, xoffset);
}

void processMovement() {
    glm::vec3 boundingBoxMin = glm::vec3(-200.0f, -10.0f, -200.0f);
    glm::vec3 boundingBoxMax = glm::vec3(200.0f, 40.0f, 200.0f);

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed, boundingBoxMin, boundingBoxMax, mapTriangles);
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed, boundingBoxMin, boundingBoxMax, mapTriangles);
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed, boundingBoxMin, boundingBoxMax, mapTriangles);
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed, boundingBoxMin, boundingBoxMax, mapTriangles);
    }
    float rotationSpeed = 1.0f;
    if (pressedKeys[GLFW_KEY_UP]) {
        myCamera.rotate(rotationSpeed, 0.0f);
    }
    if (pressedKeys[GLFW_KEY_DOWN]) {
        myCamera.rotate(-rotationSpeed, 0.0f);
    }
    if (pressedKeys[GLFW_KEY_LEFT]) {
        myCamera.rotate(0.0f, -rotationSpeed);
    }
    if (pressedKeys[GLFW_KEY_RIGHT]) {
        myCamera.rotate(0.0f, rotationSpeed);
    }
    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
    }
    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
    }
    float scaleFactor = 1.0f;
    if (pressedKeys[GLFW_KEY_U]) { 
        scaleFactor += 0.1f;
        if (scaleFactor > 10.0f) {
            scaleFactor = 10.0f;
        }
    }
    if (pressedKeys[GLFW_KEY_O]) {
        glm::vec3 camPos = myCamera.getPosition();
        std::cout << "Camera Position: ("
            << camPos.x << ", "
            << camPos.y << ", "
            << camPos.z << ")" << std::endl;
    }
    model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(scaleFactor));

    glm::vec3 currentCameraPosition = myCamera.getPosition();
}

void initOpenGLWindow() {
    myWindow.Create(1280, 720, "OpenGL Minecraft World");
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initModels() {
    mapModel.LoadModel("models/fullMap/MinecraftMap.obj");
    creeperModel.LoadModel("models/movingCreeper/creeper.obj");
    villagerModel.LoadModel("models/movingVillager/villager.obj");
    herobrineModel.LoadModel("models/herobrine/herobrine.obj");

    mapTriangles = mapModel.GetTriangles();
}

void initShaders() {
    basicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");

}
void initUniforms() {
    basicShader.useShaderProgram();
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(basicShader.shaderProgram, "model");
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(basicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "normalMatrix");

    float fov = 60.0f;
    float aspect = (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height;
    float nearPlaneMain = 0.1f;
    float farPlaneMain = 10000.0f;

    projection = glm::perspective(glm::radians(fov), aspect, nearPlaneMain, farPlaneMain);
    projectionLoc = glGetUniformLocation(basicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //torch lights
    glm::vec3 torchLightPos1 = glm::vec3(-23.1485f, -6.02295f, 8.95726f);
    glm::vec3 torchLightPos2 = glm::vec3(-23.1617f, -5.98696f, -0.233071f);
    glm::vec3 torchLightPos3 = glm::vec3(-23.183f, -4.35429f, -4.85121f);
    glm::vec3 torchLightColor = glm::vec3(1.5f, 0.5f, 0.2f);

    GLint torchLightPos1Loc = glGetUniformLocation(basicShader.shaderProgram, "torchLightPos1");
    GLint torchLightPos2Loc = glGetUniformLocation(basicShader.shaderProgram, "torchLightPos2");
    GLint torchLightPos3Loc = glGetUniformLocation(basicShader.shaderProgram, "torchLightPos3");
    GLint torchLightColorLoc = glGetUniformLocation(basicShader.shaderProgram, "torchLightColor");
    shadowMapLoc = glGetUniformLocation(basicShader.shaderProgram, "shadowMap");
    lightSpaceMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "lightSpaceMatrix");

    if (torchLightPos1Loc != -1) {
        glUniform3fv(torchLightPos1Loc, 1, glm::value_ptr(torchLightPos1));
    }
    else {
        std::cerr << "torchLightPos1 uniform location not found." << std::endl;
    }
    if (torchLightPos2Loc != -1) {
        glUniform3fv(torchLightPos2Loc, 1, glm::value_ptr(torchLightPos2));
    }
    else {
        std::cerr << "torchLightPos2 uniform location not found." << std::endl;
    }
    if (torchLightPos3Loc != -1) {
        glUniform3fv(torchLightPos3Loc, 1, glm::value_ptr(torchLightPos3));
    }
    else {
        std::cerr << "torchLightPos3 uniform location not found." << std::endl;
    }
    if (torchLightColorLoc != -1) {
        glUniform3fv(torchLightColorLoc, 1, glm::value_ptr(torchLightColor));
    }
    else {
        std::cerr << "torchLightColor uniform location not found." << std::endl;
    }

    if (shadowMapLoc != -1) {
        glUniform1i(shadowMapLoc, 1);
    }
    else {
        std::cerr << "shadowMap uniform not found." << std::endl;
    }

    GLint diffuseLoc = glGetUniformLocation(basicShader.shaderProgram, "diffuseTexture");
    GLint specularLoc = glGetUniformLocation(basicShader.shaderProgram, "specularTexture");

    if (diffuseLoc != -1)
        glUniform1i(diffuseLoc, 0);
    else
        std::cerr << "diffuseTexture uniform not found." << std::endl;

    if (specularLoc != -1)
        glUniform1i(specularLoc, 1);
    else
        std::cerr << "specularTexture uniform not found." << std::endl;

    glm::vec3 fogColor(0.4f, 0.4f, 0.4f);
    float initialFogDensity = 0.050f;

    GLint fogColorLoc = glGetUniformLocation(basicShader.shaderProgram, "fogColor");
    if (fogColorLoc != -1) {
        glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
    }
    else {
        std::cerr << "fog color uniform location not found." << std::endl;
    }

    fogDensityLoc = glGetUniformLocation(basicShader.shaderProgram, "fogDensity");
    if (fogDensityLoc != -1) {
        glUniform1f(fogDensityLoc, initialFogDensity);
    }
    else {
        std::cerr << "fog density uniform location not found." << std::endl;
    }

    GLint sunLightPosLoc = glGetUniformLocation(basicShader.shaderProgram, "sunLightPos");
    GLint sunLightColorLoc = glGetUniformLocation(basicShader.shaderProgram, "sunLightColor");

    if (sunLightPosLoc != -1 && sunLightColorLoc != -1) {
        glUniform3fv(sunLightPosLoc, 1, glm::value_ptr(sunLightPos));
        glUniform3fv(sunLightColorLoc, 1, glm::value_ptr(sunLightColor));
    }
    else {
        std::cerr << "sunlight uniform locations not found." << std::endl;
    }

    lightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(basicShader.shaderProgram, "lightDir");
    lightColorLoc = glGetUniformLocation(basicShader.shaderProgram, "lightColor");

    if (lightDirLoc != -1) {
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    }
    else {
        std::cerr << "lightDir uniform location not found." << std::endl;
    }

    if (lightColorLoc != -1) {
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    }
    else {
        std::cerr << "lightColor uniform location not found." << std::endl;
    }

    GLint herobrineLightPosLoc = glGetUniformLocation(basicShader.shaderProgram, "mainSunLightPos");
    GLint herobrineLightColorLoc = glGetUniformLocation(basicShader.shaderProgram, "mainSunLightColor");
    glUniform3fv(herobrineLightPosLoc, 1, glm::value_ptr(mainSunLightPos));
    glUniform3fv(herobrineLightColorLoc, 1, glm::value_ptr(mainSunLightColor));

    lightSpaceMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "lightSpaceMatrix");
    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    GLint shadowMapLoc = glGetUniformLocation(basicShader.shaderProgram, "shadowMap");
    glUniform1i(shadowMapLoc, 1); 
}
glm::mat4 computeSunLightSpaceMatrix() {
  
    float orthoSize = 50.0f; 
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 100.0f);

    glm::mat4 lightView = glm::lookAt(
        mainSunLightPos,
        glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    return lightSpaceMatrix;
}


void initFog() {

    glm::vec3 fogColor(0.4f, 0.4f, 0.4f);
    float fogDensity = 0.050f;

    GLint fogColorLoc = glGetUniformLocation(basicShader.shaderProgram, "fogColor");
    GLint fogDensityLocLocal = glGetUniformLocation(basicShader.shaderProgram, "fogDensity");

    if (fogColorLoc != -1) {
        glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
    }
    else {
        std::cerr << "fog color uniform location not found in initFog." << std::endl;
    }

    if (fogDensityLocLocal != -1) {
        glUniform1f(fogDensityLocLocal, fogDensity);
    }
    else {
        std::cerr << "fog density uniform location not found in initFog." << std::endl;
    }
}

static glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + t * (b - a);
}

void initShadowMap() {
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: shadow framebuffer is not complete.." << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void updateVillager() {
    villagerAnimProgress += villagerAnimSpeed;
    if (villagerAnimProgress >= 1.0f) {
        villagerAnimProgress = 0.0f;

        villagerCurrentIndex = villagerNextIndex;
        villagerNextIndex++;

        if (villagerNextIndex >= villagerWaypointCount) {
            villagerNextIndex = 1;
            villagerCurrentIndex = 0;
        }

        villagerAngle -= 90.0f;
    }
    glm::vec3 startPos = villagerWaypoints[villagerCurrentIndex];
    glm::vec3 endPos = villagerWaypoints[villagerNextIndex];
    villagerPos = lerp(startPos, endPos, villagerAnimProgress);
}

void updateWorldConfigurations() {
    glm::vec3 cameraPosition = myCamera.getPosition();

    float fogDensity = 0.050f;

    glm::vec3 herobrineMin = herobrinePos - glm::vec3(1.0f, 1.0f, 1.0f); 
    glm::vec3 herobrineMax = herobrinePos + glm::vec3(1.0f, 1.0f, 1.0f);

    bool isTouchingHerobrine =
        cameraPosition.x >= herobrineMin.x && cameraPosition.x <= herobrineMax.x &&
        cameraPosition.y >= herobrineMin.y - 3 && cameraPosition.y <= herobrineMax.y - 3 &&
        cameraPosition.z >= herobrineMin.z && cameraPosition.z <= herobrineMax.z;

    if (isTouchingHerobrine) {
        std::cout << "Player has touched Herobrine! Closing the program." << std::endl;
        glfwSetWindowShouldClose(myWindow.getWindow(), GL_TRUE);
        return;
    }


    if (cameraPosition.y < -119.0f) {
        fogDensity = 0.0f;
    }
    else {
        float xMin1 = -15.0f, xMax1 = -12.5f;
        float yMin1 = -0.80f, yMax1 = 2.69f;
        float zTarget1 = 19.6f;
        float zTolerance1 = 0.5f;

        bool withinTeleportRegion1 = (cameraPosition.x >= xMin1) && (cameraPosition.x <= xMax1) &&
            (cameraPosition.y >= yMin1) && (cameraPosition.y <= yMax1) &&
            (cameraPosition.z >= (zTarget1 - zTolerance1)) && (cameraPosition.z <= (zTarget1 + zTolerance1));

        float xMin2 = -9.29024f, xMax2 = -6.73424f;
        float yMin2 = -200.297f, yMax2 = -196.597f;
        float zMin2 = 6.78935f, zMax2 = 6.89306f;

        bool withinTeleportRegion2 = (cameraPosition.x >= xMin2) && (cameraPosition.x <= xMax2) &&
            (cameraPosition.y >= yMin2) && (cameraPosition.y <= yMax2) &&
            (cameraPosition.z >= zMin2) && (cameraPosition.z <= zMax2);

        if (withinTeleportRegion1) {
            myCamera.setPosition(glm::vec3(-6.59847f, -199.594f, 9.15959f));
            fogDensity = 0.0f;
        }
        else if (withinTeleportRegion2) {
            myCamera.setPosition(glm::vec3(-13.3951f, 0.918087f, 15.6772f));
            fogDensity = 0.050f;
        }
    }

    bool isWithinTeleportBackRegion = (
        cameraPosition.x >= teleportBackMin.x && cameraPosition.x <= teleportBackMax.x &&
        cameraPosition.y >= teleportBackMin.y && cameraPosition.y <= teleportBackMax.y &&
        cameraPosition.z >= teleportBackMin.z && cameraPosition.z <= teleportBackMax.z
        );

    if (isWithinTeleportBackRegion && !hasTeleportedBack) {
        myCamera.setPosition(overworldTeleportPos);
        fogDensity = 0.050f;
        hasTeleportedBack = true;
    }
    else if (!isWithinTeleportBackRegion && hasTeleportedBack) {
        hasTeleportedBack = false;
    }

    if (fogDensityLoc >= 0) {
        glUniform1f(fogDensityLoc, fogDensity);
    }
    else {
        std::cerr << "Error: fog density uniform location not found." << std::endl;
    }
}

void renderSceneDepth(gps::Shader& depthShader) {
    glm::mat4 mapM = glm::mat4(1.0f);
    GLint uModelLoc = glGetUniformLocation(depthShader.shaderProgram, "uModel");
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(mapM));
    mapModel.Draw(depthShader);
}

void renderDepthMapFromHerobrineLight() {
    lightSpaceMatrix = computeSunLightSpaceMatrix();

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    renderSceneDepth(depthMapShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int screenWidth = myWindow.getWindowDimensions().width;
    int screenHeight = myWindow.getWindowDimensions().height;
    glViewport(0, 0, screenWidth, screenHeight);

    lastHerobrineLightLSM = lightSpaceMatrix;
}

const float originalCreeperHeight = 2.0f; 

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 mapMatrix = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mapMatrix));
    glm::mat3 mapNormalMatrix = glm::mat3(glm::inverseTranspose(view * mapMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(mapNormalMatrix));
    mapModel.Draw(basicShader);

    if (goingForward) {
        creeperAnimProgress += creeperAnimSpeed;
        if (creeperAnimProgress >= 1.0f) {
            creeperAnimProgress = 1.0f;
            goingForward = false;
            creeperAngle = 180.0f;
        }
    }
    else {
        creeperAnimProgress -= creeperAnimSpeed;
        if (creeperAnimProgress <= 0.0f) {
            creeperAnimProgress = 0.0f;
            goingForward = true;
            creeperAngle = 0.0f;
        }
    }

    if (fogDensityLoc >= 0) {
        glUniform1f(fogDensityLoc, 0.017f);
    }

    glm::vec3 creeperPos = glm::mix(creeperStartPos, creeperEndPos, creeperAnimProgress);

    glm::mat4 creeperMatrix = glm::mat4(1.0f);

    creeperMatrix = glm::translate(creeperMatrix, creeperPos);

    creeperMatrix = glm::translate(creeperMatrix, glm::vec3(0.0f, -originalCreeperHeight / 2.0f, 0.0f));

    creeperMatrix = glm::scale(creeperMatrix, glm::vec3(1.0f, creeperHeightScale, 1.0f));

    creeperMatrix = glm::translate(creeperMatrix, glm::vec3(0.0f, originalCreeperHeight / 2.0f, 0.0f));

    creeperMatrix = glm::rotate(creeperMatrix, glm::radians(creeperAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(creeperMatrix));
    glm::mat3 creeperNormalMatrix = glm::mat3(glm::inverseTranspose(view * creeperMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(creeperNormalMatrix));

    creeperModel.Draw(basicShader);

    if (fogDensityLoc >= 0) {
        glUniform1f(fogDensityLoc, 0.050f);
    }

    glm::mat4 villagerMatrix = glm::mat4(1.0f);
    villagerMatrix = glm::translate(villagerMatrix, villagerPos);
    villagerMatrix = glm::rotate(villagerMatrix, glm::radians(villagerAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(villagerMatrix));

    glm::mat3 villagerNormalMatrix = glm::mat3(glm::inverseTranspose(view * villagerMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(villagerNormalMatrix));

    villagerModel.Draw(basicShader);

    if (fogDensityLoc >= 0) {
        glUniform1f(fogDensityLoc, 0.012f); 
    }
    glm::mat4 herobrineMatrix = glm::translate(glm::mat4(1.0f), herobrinePos);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(herobrineMatrix));
    glm::mat3 herobrineNormalMatrix = glm::mat3(glm::inverseTranspose(view * herobrineMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(herobrineNormalMatrix));
    herobrineModel.Draw(basicShader);

    if (fogDensityLoc >= 0) {
        glUniform1f(fogDensityLoc, 0.050f);
    }
}



void cleanup() {
    myWindow.Delete();
}

int main(int argc, const char* argv[]) {
    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initShadowMap();
    initModels();
    initShaders();
    initUniforms();
    initFog();
    setWindowCallbacks();

    glCheckError();

    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        glfwPollEvents();
        processMovement();

        basicShader.useShaderProgram();
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        updateWorldConfigurations();

        lightSpaceMatrix = computeSunLightSpaceMatrix();
        glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(basicShader.shaderProgram, "shadowMap"), 1);

        updateVillager();
        renderScene();
           
        glfwSwapBuffers(myWindow.getWindow());
        glCheckError();
    }
    cleanup();
    return EXIT_SUCCESS;
}
