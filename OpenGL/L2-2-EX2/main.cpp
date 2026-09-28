#include "Shader.hpp"
#include <cmath>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "camera.hpp"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/*
    尝试使用Gouraud着色。（改变着色器即可）
    按下B键切换两种光照模型。
*/

Camera camera(glm::vec3(0.8f, 0.5f, 3.0f));
float lastX = SCREEN_WIDTH / 2;
float lastY = SCREEN_HEIGHT / 2;
bool firstMouse = true;

bool usePhong = true;
bool keyBPressedLastFrame = false;

// timing
float deltaTime = 0.0f; // 时间差
float lastFrame = 0.0f; // 上一帧的时间

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.ProcessKeyboardInput('W',deltaTime);
    }
    if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.ProcessKeyboardInput('S',deltaTime);
    }
    if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.ProcessKeyboardInput('A',deltaTime);
    }
    if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS)
    {
        camera.ProcessKeyboardInput('D',deltaTime);
    }
    if(glfwGetKey(window,GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        camera.ProcessKeyboardInput(' ',deltaTime);
    }
    if(glfwGetKey(window,GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        camera.ProcessKeyboardInput('C',deltaTime);
    }

    bool keyBCurrentFrame = (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS);
    
    // 如果这一帧按下，但上一帧没按下，说明是新的按键按下
    if (keyBCurrentFrame && !keyBPressedLastFrame) {
        usePhong = !usePhong;  
        std::cout << "切换到：" << (usePhong ? "Phong着色" : "Gouraud着色") << std::endl;
    }
    
    // 更新按键状态记录
    keyBPressedLastFrame = keyBCurrentFrame;
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        firstMouse = false;
        return;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // 注意这里是相反的，因为y坐标是从底部往顶部依次增大的
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW Window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 告诉GLFW是否捕获光标
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader GouraudCubeShader("GouraudLightingShader.vs", "GouraudLightingShader.fs");
    Shader PhongCubeShader("PhongLightingShader.vs", "PhongLightingShader.fs");
    Shader lightShader("lightingCubeShader.vs", "lightingCubeShader.fs");

    // 原始正方体空间坐标+法向量
    float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
   
    unsigned int cubeVAO, VBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0); // 我们仅关心作为灯的立方体的位置，所以VBO也无需更改
    glEnableVertexAttribArray(0);

    glm::mat4 model(1.0f),view(1.0f),projection(1.0f);
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // 渲染指令
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 设定立方体的颜色
        if(usePhong){
            PhongCubeShader.use();  
            PhongCubeShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
            PhongCubeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            PhongCubeShader.setVec3("lightPos", lightPos);
            PhongCubeShader.setVec3("viewPos", camera.GetPosition());
            view = camera.GetViewMatrix(); // 相机环绕在原点
            projection = glm::perspective(glm::radians(camera.GetZoom()), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
            PhongCubeShader.setMat4("view", view);
            PhongCubeShader.setMat4("projection", projection);
            model = glm::mat4(1.0f);
            PhongCubeShader.setMat4("model",model);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }else{
            GouraudCubeShader.use();  
            GouraudCubeShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
            GouraudCubeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            GouraudCubeShader.setVec3("lightPos", lightPos);
            GouraudCubeShader.setVec3("viewPos", camera.GetPosition());
            view = camera.GetViewMatrix(); // 相机环绕在原点
            projection = glm::perspective(glm::radians(camera.GetZoom()), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
            GouraudCubeShader.setMat4("view", view);
            GouraudCubeShader.setMat4("projection", projection);
            model = glm::mat4(1.0f);
            GouraudCubeShader.setMat4("model",model);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 设定光源
        lightShader.use();
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightShader.setMat4("model", model);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}