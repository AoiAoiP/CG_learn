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
    继承L1-6-3，实现使用lookAt矩阵设定摄像机，并让其随时间绕y轴旋转
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

    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f), 
        glm::vec3( 2.0f,  5.0f, -15.0f), 
        glm::vec3(-1.5f, -2.2f, -2.5f),  
        glm::vec3(-3.8f, -2.0f, -12.3f),  
        glm::vec3( 2.4f, -0.4f, -3.5f),  
        glm::vec3(-1.7f,  3.0f, -7.5f),  
        glm::vec3( 1.3f, -2.0f, -2.5f),  
        glm::vec3( 1.5f,  2.0f, -2.5f), 
        glm::vec3( 1.5f,  0.2f, -1.5f), 
        glm::vec3(-1.3f,  1.0f, -1.5f)  
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
    
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 初始化
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 设定VBO：顶点位置(location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // 设定VBO：纹理位置(location = 1)
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
    // model = glm::rotate(model,glm::radians(-55.0f),glm::vec3(1.0f,0.0f,0.0f)); // 绕着x轴转55°
    projection = glm::perspective(glm::radians(60.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

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
        
        ourShader1.use();
        float radius = 10.0f;
        float camX = sinf((float)glfwGetTime()) * radius;
        float camZ = cosf((float)glfwGetTime()) * radius;
        view = glm::lookAt(glm::vec3(camX,0.0f,camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // 相机环绕在原点
        ourShader1.setMat4("view", view);
        ourShader1.setMat4("projection", projection);
        glBindVertexArray(VAO);
        for(int i = 0;i < 10;i++){
            model = glm::mat4(1.0f);
            model = glm::translate(model,cubePositions[i]);
            if(i%3 == 0){
                float angle = 20.0f * i;
                model = glm::rotate(model,(float)glfwGetTime() * glm::radians(50.0f)+glm::radians(angle),glm::vec3(1.0f,0.3f,0.5f));
            }
            ourShader1.setMat4("model",model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        ourShader2.use();
        // 实现随时间变换的移动+旋转+放缩
        glm::mat4 trans(1.0f);
        trans = glm::translate(trans, glm::vec3((float)sinf(glfwGetTime()), (float)cosf(glfwGetTime()), 0.0f));
        trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0, 0.0, 1.0));
        trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));
        model = glm::mat4(1.0f);
        ourShader2.setMat4("model", model);
        ourShader2.setMat4("view", view);
        ourShader2.setMat4("projection", projection);
        ourShader2.setMat4("transform", trans);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}