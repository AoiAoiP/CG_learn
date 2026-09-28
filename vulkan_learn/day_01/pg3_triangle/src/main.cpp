// Vulkan实例的学习

#include <vulkan/vulkan.h> 
// 事实上裸vulkan头也行，只是GLFW官方包含了包含了vulkan的用法
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class TriangleAPP{
public:
    void run(){
        initWindow();
        initVulkan();
        mainLoop();
        exit();
    }
private:
    GLFWwindow* window;
    VkInstance instance;

    void initWindow(){
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);

        window = glfwCreateWindow(WIDTH,HEIGHT,"VulkanTriangle",nullptr,nullptr);
    }

    void createInstance(){
        // 创建实例：与OpenGL的状态机本质不同，Vulkan存在全局Vulkan本身这一对象
        // Vulkan实例用于建立于驱动的连接、声明启用哪些全局扩展、显式开启校验层
        // 创建时，需要明确指定成员类型
        VkApplicationInfo appInfo{}; // VkAppInfo用于说明这个程序是谁，不控制任何vulkan行为
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // 还需要一个结构来提供创建实例的信息。
        // Vulkan中的很多信息可以直接通过结构体传递。
        VkInstanceCreateInfo createInfo{}; // vkCreateInfo用于说明这次实例创建要做什么
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // 由于vulkan与平台无关，所以需要用于窗口交互的扩展。
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // 错误案例：丧失可移植性
        // const char* exts[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW内部根据当前平台和编译配置返回用于告知vulkan的字符串数组

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions; // pp开头字段的结构体都是字符串数组+长度的配对
        createInfo.enabledLayerCount = 0;
        /*
            到底如何理解Vulkan的平台无关性？
            Vulkan不像opengl，甚至没有窗口的概念，窗口是剥离出核心api的。这个窗口如何从当前设备变出来是由扩展负责。
        */

        VkResult result = vkCreateInstance(&createInfo,nullptr,&instance);// 通常返回错误代码
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create instance!");
        }
    }

    void initVulkan(){
        // 创建一个Vulkan实例
        createInstance();
    }

    void mainLoop(){
        while(!glfwWindowShouldClose(window)){
            glfwPollEvents();
        }
    }

    void exit(){
        vkDestroyInstance(instance,nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main(){
    TriangleAPP app;
    try
    {
        app.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    
    return 0;
}