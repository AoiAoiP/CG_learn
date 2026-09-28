#include "Shader.hpp"
#include <cmath>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "GLInput.h"
#include "camera.hpp"
#include "model.hpp"

/*
    使用模板测试实现物体轮廓高亮效果，并实现鼠标悬停高亮边框
*/

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

unsigned int loadTexture(const char* path){
    unsigned int textureID;
    glGenTextures(1,&textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

    if(data){
        GLenum format;
        if(nrChannels == 1){
            format = GL_RED;
        }else if(nrChannels == 3){
            format = GL_RGB;
        }else if(nrChannels == 4){
            format = GL_RGBA;
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        std::cout<<"Texture: " <<path <<" loaded successfully."<<std::endl;
    }else{
        std::cout<<"Failed to load Texture."<<std::endl;
    }
    stbi_image_free(data);

    return textureID;
}

glm::vec3 ScreenPosToWorldPos(
    float mouseX,float mouseY,
    glm::mat4 projection,glm::mat4 view,
    int screenWidth = SCREEN_WIDTH,int screenHeight = SCREEN_HEIGHT
){
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    
    glm::vec4 ndcSpacePos = glm::vec4(x,y,-1.0f,1.0f);
    glm::vec4 cameraSpacePos = glm::inverse(projection) * ndcSpacePos;
    cameraSpacePos = glm::vec4(cameraSpacePos.x,cameraSpacePos.y,-1.0f,0.0f);

    glm::vec3 worldSpacePos = glm::normalize(glm::vec3(glm::inverse(view) * cameraSpacePos));
    return worldSpacePos;
}

bool RayIntersectsAABB(glm::vec3 rayOrigin, glm::vec3 rayDir,
                       glm::vec3 boxMin, glm::vec3 boxMax) {
    glm::vec3 invDir = 1.0f / rayDir;
    
    float t1 = (boxMin.x - rayOrigin.x) * invDir.x;
    float t2 = (boxMax.x - rayOrigin.x) * invDir.x;
    float t3 = (boxMin.y - rayOrigin.y) * invDir.y;
    float t4 = (boxMax.y - rayOrigin.y) * invDir.y;
    float t5 = (boxMin.z - rayOrigin.z) * invDir.z;
    float t6 = (boxMax.z - rayOrigin.z) * invDir.z;
    
    float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
    float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));
    
    // 如果tmax < 0，射线与AABB在反方向相交
    if (tmax < 0) return false;
    
    // 如果tmin > tmax，没有相交
    if (tmin > tmax) return false;
    
    return true;
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

    stbi_set_flip_vertically_on_load(true);

    // 设定全局OpenGL状态
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_ALWAYS); // 最后绘制的片段将会总是会渲染在之前绘制片段的上面
    glDepthFunc(GL_LESS); // 场景原样，即立方体放在地面上
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    // 参数1 (func): 测试函数 -> 总是通过(GL_ALWAYS)
    // 参数2 (ref): 参考值 -> 我们想写入的模板值，这里设为 1
    // 参数3 (mask): 掩码 -> 0xFF表示所有位都参与

    Shader shader("depth_testing.vs","depth_testing.fs");
    Shader shaderSingleColor("shaderSingleColor.vs","shaderSingleColor.fs");

    float cubeVertices[] = {
        // positions          // texture Coords
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
    float planeVertices[] = {
        // positions          // texture Coords 
         5.0f, -0.51f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.51f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.51f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.51f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.51f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.51f, -5.0f,  2.0f, 2.0f								
    };// 略微调低平面y轴，避免因为与立方体贴近出现深度冲突

    // 定义两个立方体的包围盒（在模型空间，然后通过模型矩阵变换）
    std::vector<glm::vec3> cubePositions = {
        glm::vec3(-1.0f, 0.0f, -1.0f),
        glm::vec3(2.0f, 0.0f, 0.0f)
    };
    
    // 检查每个立方体是否被鼠标射线选中
    std::vector<bool> cubeHovered(2, false);
    glm::vec3 boxMin = glm::vec3(-0.5f, -0.5f, -0.5f); // 立方体模型空间的包围盒最小值
    glm::vec3 boxMax = glm::vec3(0.5f, 0.5f, 0.5f);   // 立方体模型空间的包围盒最大值

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices),&cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0); 
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3*sizeof(float)));
    glBindVertexArray(0); 

    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices),&planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0); 
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3*sizeof(float)));
    glBindVertexArray(0); 

    unsigned int cubeTexture = loadTexture("../resources/textures/marble.jpg");
    unsigned int floorTexture = loadTexture("../resources/textures/metal.png");

    shader.use();
    shader.setInt("texture1",0);

    glm::mat4 model(1.0f), view(1.0f), projection(1.0f);
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    // timing
    float deltaTime = 0.0f; // 时间差
    float lastFrame = 0.0f; // 上一帧的时间

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
        glm::vec3 rayOrigin = camera.GetPosition();
        glm::vec3 rayDir = ScreenPosToWorldPos(mouseX,mouseY,projection,view);

        for(int i = 0;i<2;i++){
            glm::mat4 cubeModel = glm::mat4(1.0f);
            cubeModel = glm::translate(cubeModel, cubePositions[i]);

            glm::mat4 invModel = glm::inverse(cubeModel);
            glm::vec3 rayOriginModel = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
            glm::vec3 rayDirModel = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

            // 进行AABB包围盒相交测试
            cubeHovered[i] = RayIntersectsAABB(rayOriginModel, rayDirModel, boxMin, boxMax);
        }

        // 渲染指令
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // 别忘了添加对于模板缓冲的清除

        shader.use();
        shader.setMat4("view",view);
        shader.setMat4("projection",projection);

        glStencilMask(0x00); // 禁用模板缓冲，防止其写入地板，因为不需要对地板添加边框
        // floor
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        shader.setMat4("model", glm::mat4(1.0f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // 第一遍：照常绘制立方体，将其写入模板缓冲区
        glStencilMask(0xFF); // 启用模板缓冲写入
        // cubes
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,cubeTexture);
        for(int i = 0;i<2;i++){
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            if (cubeHovered[i]) {
                glStencilFunc(GL_ALWAYS, 1, 0xFF); // 悬停的写入模板值1
            } else {
                glStencilFunc(GL_ALWAYS, 0, 0xFF); // 未悬停的写入模板值0
            }
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        // 第二步：使用模板测试来绘制边框
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // 只绘制模板缓冲区中值不为1的像素
        glStencilMask(0x00); // 关闭模板缓冲写入，保证绘制不修改模板值
        glDisable(GL_DEPTH_TEST); // 禁用深度测试，因为边框不应该被深度测试遮挡

        shaderSingleColor.use();
        shaderSingleColor.setMat4("view", view);
        shaderSingleColor.setMat4("projection", projection);

        // 绘制放大的cubes
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);

        for(int i = 0;i<2;i++){
            if(cubeHovered[i]){
                model = glm::mat4(1.0f);
                model = glm::translate(model, cubePositions[i]);
                model = glm::scale(model, glm::vec3(1.1f));
                shaderSingleColor.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        glBindVertexArray(0);
        // 关闭模板测试(启用常规模板)，并重新启用深度测试
        glStencilMask(0xFF); // 启用模板缓冲写入
        glStencilFunc(GL_ALWAYS, 0, 0xFF); // 重新启用模板测试，将所有像素都绘制出来
        glEnable(GL_DEPTH_TEST); // 重新启用深度测试

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &planeVBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}