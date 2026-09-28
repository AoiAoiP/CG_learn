/*
    学习OpenGL程序运行时的基本流程与初始化的必要操作
*/
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

// 需要对窗口注册一个回调函数，在每次窗口大小被调整时被调用
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // 这个函数用于调整视口尺寸大小，前两个参数控制窗口左下角位置，第三第四个参数分别控制渲染窗口的宽度和高度
    // 在幕后，OpenGL使用glViewport定义的位置与宽高进行2D坐标的转换，将OpenGL的位置坐标转换为屏幕坐标
}

// 在GLFW中实现一些输入控制
void processInput(GLFWwindow *window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window,true);
    }
}

int main()
{
    glfwInit(); // 初始化GLFW
    // glfwWindowHint()函数用于设置可以影响窗口/缓冲区/上下文的提示
    // GLFW_CONTEXT_VERSION_MAJOR 和 GLFW_CONTEXT_VERSION_MINOR 指定了创建上下文必须兼容的客户端 API 版本。这些提示的具体行为取决于所请求的客户端 API。
    // 这里是要告诉我们的OpenGL版本是3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 用于告诉GLFW使用核心模式（core-profile）
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // MacOS需要额外加这行代码
#endif

    // 在副显示器显示
    // int monitorCount;
    // GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    // GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", monitors[0], NULL);
    GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW Window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // OpenGL的上下文，就是状态机的一个完整示例，OpenGL需要一些API函数来知道自己该操作哪个状态机，这就是需要一个当前上下文来知道该使用或修改哪一套状态。
    // glfwMakeContextCurrent(window)的作用就是将指定的window窗口的OpenGL上下文设置为当前线程的当前上下文
    // 这样在window的所有OpenGL调用就不会影响别的窗口
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Render Loop:让GLFW退出前一直保持运行
    while(!glfwWindowShouldClose(window)){
        // glfwWindowShouldClose函数在我们每次循环的开始前检查一次GLFW是否被要求退出，如果是的话，该函数返回true，渲染循环将停止运行，之后我们就可以关闭应用程序。

        // 输入部分
        processInput(window); // 现在按下ESC可以关闭窗口

        // 渲染指令
        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // glClear函数来清空屏幕的颜色缓冲，它接受一个缓冲位(Buffer Bit)来指定要清空的缓冲.
        // 可能的缓冲位有GL_COLOR_BUFFER_BIT，GL_DEPTH_BUFFER_BIT和GL_STENCIL_BUFFER_BIT。由于现在我们只关心颜色值，所以我们只清空颜色缓冲。

        // 检查、调用事件，交换缓冲
        glfwSwapBuffers(window); // glfwSwapBuffers函数会交换颜色缓冲（它是一个储存着GLFW窗口每一个像素颜色值的大缓冲），它在这一迭代中被用来绘制，并且将会作为输出显示在屏幕上。
        // 单缓冲绘图的不真实感源于图像绘制存在顺序（从左往右、从上到下），所以不是瞬间呈现的。
        // 这里使用了双缓冲保证渲染的真实感。前缓冲保存最终输出的图像，所有的渲染指令在后缓冲上绘制，渲染指令执行完毕后Swap前后缓冲，这样就能立刻显示图像
        glfwPollEvents(); //glfwPollEvents函数检查有没有触发什么事件（比如键盘输入、鼠标移动等）、更新窗口状态，并调用对应的回调函数（可以通过回调方法手动设置）。
    }

    glfwTerminate(); // 释放与删除所有分配资源
    return 0;
}