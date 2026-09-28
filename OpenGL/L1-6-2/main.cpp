#include "Shader.hpp"
#include <cmath>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/*
    继承L1-5-EX2，并将正方形3D化为正方体
*/

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader ourShader1("shader1.vs", "shader1.fs");
    Shader ourShader2("shader2.vs", "shader2.fs");

    // 原始正方体空间坐标+纹理坐标，太大了
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    // float spaceVertices[] = {
    // // 前面（z = -0.5）的4个顶点
    // -0.5f, -0.5f, -0.5f,  // 0: 左-下-前
    //  0.5f, -0.5f, -0.5f,  // 1: 右-下-前
    //  0.5f,  0.5f, -0.5f,  // 2: 右-上-前
    // -0.5f,  0.5f, -0.5f,  // 3: 左-上-前
    // // 后面（z = 0.5）的4个顶点  
    // -0.5f, -0.5f,  0.5f,  // 4: 左-下-后
    //  0.5f, -0.5f,  0.5f,  // 5: 右-下-后
    //  0.5f,  0.5f,  0.5f,  // 6: 右-上-后
    // -0.5f,  0.5f,  0.5f   // 7: 左-上-后
    // };
    // float texCoords[] = {
    // // 前面 (使用顶点 0,1,2,3)
    // 0.0f, 0.0f,  // 左下 - 顶点0
    // 1.0f, 0.0f,  // 右下 - 顶点1  
    // 1.0f, 1.0f,  // 右上 - 顶点2
    // 0.0f, 1.0f,  // 左上 - 顶点3
    // // 后面 (使用顶点 4,5,6,7) - 注意镜像处理
    // 1.0f, 0.0f,  // 对应顶点4
    // 0.0f, 0.0f,  // 对应顶点5
    // 0.0f, 1.0f,  // 对应顶点6
    // 1.0f, 1.0f,  // 对应顶点7
    // // 左面 (使用顶点 4,0,3,7)
    // 0.0f, 0.0f,  // 左下 - 顶点4
    // 1.0f, 0.0f,  // 右下 - 顶点0
    // 1.0f, 1.0f,  // 右上 - 顶点3
    // 0.0f, 1.0f,  // 左上 - 顶点7
    // // 右面 (使用顶点 1,5,6,2)
    // 0.0f, 0.0f,  // 左下 - 顶点1
    // 1.0f, 0.0f,  // 右下 - 顶点5
    // 1.0f, 1.0f,  // 右上 - 顶点6
    // 0.0f, 1.0f,  // 左上 - 顶点2
    // // 下面 (使用顶点 4,5,1,0)
    // 0.0f, 0.0f,  // 左下 - 顶点4
    // 1.0f, 0.0f,  // 右下 - 顶点5
    // 1.0f, 1.0f,  // 右上 - 顶点1
    // 0.0f, 1.0f,  // 左上 - 顶点0
    // // 上面 (使用顶点 3,2,6,7)
    // 0.0f, 0.0f,  // 左下 - 顶点3
    // 1.0f, 0.0f,  // 右下 - 顶点2
    // 1.0f, 1.0f,  // 右上 - 顶点6
    // 0.0f, 1.0f   // 左上 - 顶点7
    // };
    
    unsigned VAOs[2], VBOs[2];
    glGenVertexArrays(2, VAOs);
    glGenBuffers(2, VBOs);

    // 第一个正方体
    glBindVertexArray(VAOs[0]);
    // 设定VBO：顶点位置(location = 0)
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // 设定VBO：纹理位置(location = 1)
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 第二个正方体
    glBindVertexArray(VAOs[1]);
    // 设定VBO：顶点位置(location = 0)
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // 设定VBO：纹理位置(location = 1)
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int texture1, texture2;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("/home/wzy/OpenGL/resources/container.jpg", &width, &height, &nrChannels, 0);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load Texture." << std::endl;
    }
    stbi_image_free(data);

    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    data = stbi_load("/home/wzy/OpenGL/resources/awesomeface.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load Texture." << std::endl;
    }
    stbi_image_free(data);

    ourShader1.use();
    ourShader1.setInt("texture1", 0);
    ourShader1.setInt("texture2", 1);

    ourShader2.use();
    ourShader2.setInt("texture1", 0);
    ourShader2.setInt("texture2", 1);

    glm::mat4 model(1.0f),view(1.0f),projection(1.0f);
    model = glm::rotate(model,glm::radians(-55.0f),glm::vec3(1.0f,0.0f,0.0f)); // 绕着x轴转55°
    view = glm::translate(view,glm::vec3(0.0f,0.0f,-3.0f)); // 相机后移3个单位
    projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // 渲染指令
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 绑定材质
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        // 实现随时间变换的移动+旋转+放缩
        glm::mat4 trans(1.0f);
        trans = glm::translate(trans, glm::vec3((float)sinf(glfwGetTime()), (float)cosf(glfwGetTime()), 0.0f));
        trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0, 0.0, 1.0));
        trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));

        ourShader1.use();
        unsigned int modelLoc1 = glGetUniformLocation(ourShader1.ID, "model");
        unsigned int viewLoc1 = glGetUniformLocation(ourShader1.ID, "view");
        unsigned int projectionLoc1 = glGetUniformLocation(ourShader1.ID, "projection");
        glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc1, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc1, 1, GL_FALSE, glm::value_ptr(projection));
        unsigned int transformLoc = glGetUniformLocation(ourShader1.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
        glBindVertexArray(VAOs[0]);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        ourShader2.use();
        unsigned int modelLoc2 = glGetUniformLocation(ourShader2.ID, "model");
        unsigned int viewLoc2 = glGetUniformLocation(ourShader2.ID, "view");
        unsigned int projectionLoc2 = glGetUniformLocation(ourShader2.ID, "projection");
        glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc2, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc2, 1, GL_FALSE, glm::value_ptr(projection));
        unsigned int transformLoc2 = glGetUniformLocation(ourShader2.ID, "transform");
        glUniformMatrix4fv(transformLoc2, 1, GL_FALSE, glm::value_ptr(trans));
        glBindVertexArray(VAOs[1]);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(2, VAOs);
    glDeleteBuffers(2, VBOs);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}