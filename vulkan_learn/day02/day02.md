## 在头文件直接引用<vulkan/vulkan.h>，与通过GLFW引用vulkan.h有何区别?
前者只引入 Vulkan 头文件；后者除了 Vulkan 头文件，还引入了 GLFW 自身的全部 API 声明，并让 GLFW 负责替你把 <vulkan/vulkan.h> 引入进来。
glfw3.h内部设计（示意）：
```
#if defined(GLFW_INCLUDE_VULKAN)
    #include <vulkan/vulkan.h>
#endif

/* 然后才是 GLFW 自身的全部声明 */
typedef struct GLFWwindow GLFWwindow;
GLFWAPI GLFWwindow* glfwCreateWindow(...);
GLFWAPI int glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, ...);
```

## 环境配置没问题，编译时也通过，但是vscode编译器无法识别与联想vulkan相关关键词怎么办？
`.vscode/c_cpp_properties.json`里配置一下即可。

## 为什么要创建实例？
与OpenGL的状态机本质不同，Vulkan存在全局Vulkan本身这一对象。Vulkan实例用于建立于驱动的连接、声明启用哪些全局扩展、显式开启校验层。创建时，需要明确指定成员类型。

## 同样是“创建实例”，appInfo与createInfo的具体区别是什么？为什么要有这二者的区分？
- `VkApplicationInfo`：关于"你这个程序是谁"。它回答的问题是：调用 Vulkan 的应用程序/引擎的元信息。字段全是身份描述——名字、版本、目标 API 版本。注意一个关键事实：这个结构体是可选的，创建实例时 createInfo.pApplicationInfo 可以填 nullptr（比如纯计算程序不在乎元数据）。它不控制任何 Vulkan 行为。
- `VkInstanceCreateInfo`：关于"这次创建要做什么"。它回答的问题是：这次 vkCreateInstance 调用本身的参数。它包含的才是真正影响行为的东西。
- 为什么要拆分？`appInfo -> createInfo -> specificInfo`算是一种Vulkan API的标准动作：
```
VkXXXCreateInfo createInfo{};   // "怎么创建"：扩展、层、标志、指向具体配置
VkXXXSpecificInfo specific{};   // "创建对象的详细规格"：被 createInfo 嵌套引用
vkCreateXXX(&createInfo, ...);
```
`VkApplicationInfo` 描述的东西（应用名、引擎名）只在实例这一层有意义；而逻辑设备、管线等创建时不需要重复申报"我是谁"。如果合成一个结构体，每个 vkCreateXXX 都得拖着一个不相关的元信息字段。

## 个人理解多平台交互需要扩展，所以glfw相关的接口为什么要这样写？
OpenGL的窗口是系统内置的，创建OpenGL上下文时天然自动绑定到一个窗口，渲染自动显示。而Vulkan甚至直接将窗口系统支持也剥离出了核心API。Vulkan核心只了解抽象的`VkSurfaceKHR`，至于下一步这个窗口如何从操作系统中变出来需要扩展——GLFW通常就是一个选择。
- Windows 上需要 VK_KHR_surface + VK_KHR_win32_surface
- Linux X11 上需要 VK_KHR_surface + VK_KHR_xlib_surface
- Wayland 上又换成 VK_KHR_wayland_surface

```
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(glfwExtensionCount); // GLFW内部根据当前平台和编译配置返回用于告知vulkan的字符串数组

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions; // pp开头字段的结构体都是字符串数组+长度的配对
     createInfo.enabledLayerCount = 0;
```
所以以上代码逻辑为：
- `glfwGetRequiredInstanceExtensions(glfwExtensionCount)`:GLFW内部根据当前平台和编译配置返回用于告知vulkan的字符串数组。
- 返回值类型是`const char**`，这意味着Vulkan结构体里凡是以pp开头的字段（pointer to pointer）默认都是字符串数组+长度的配对。

```
    // 错误案例：丧失可移植性
    // const char* exts[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
```
显然，这种硬编码不是不合理，而是丧失了可移植性。

## 如何理解Vulkan的验证层（Validation Layers）设计？
验证层是介于应用于Vulkan驱动之间的一组可选拦截器。当调用某个Vulkan函数时，调用会依次穿过启用的各个层。在验证层的每一层：
1. 检查参数是否合法，比如是否把宏指令传给了必须有效的句柄
2. 追踪对象生命周期，比如是否用了已销毁的 buffer
3. 检查同步错误，比如在 GPU 还在读这张图片时是否就转其布局
4. 通过日志或回调报告Reply发现的问题
之后才将调用传递到驱动层。

```
glBindTexture(GL_TEXTURE_2D, tex);
GLenum err = glGetError();   // 错误检查是驱动的一部分，永远在运行
```
在OpenGL中，错误检查是驱动的一部分。每次调用后(例如`glGetError()`)，驱动都在内部进行不可阻拦的校验，且发布版与调试版都会运行。一方面，这显然是个浪费，尤其是对于GPU驱动而言；另一方面，经过大量检验的成熟应用不需要这类内嵌。所以Vulkan将错误检查将驱动中剥离出来，设计成单独的验证层（Validation Layers）。
在开发期，Vulkan可以尽可能使用验证层，且由于与驱动剥离，错误检查比OpenGL更强大、更详细。而在发布期，这一层可以被完全抛弃。这也是Vulkan跑得比OpenGL快的原因之一。

### Vulkan的层设计
```
我的应用
  ↓
[验证层]        ← 调试时用，如 VK_LAYER_KHRONOS_validation
[其他层]        ← 如 API dump 层（记录所有调用）、截图层等
  ↓
Vulkan Loader  ← 加载器，负责把调用分发给层和驱动
  ↓
ICD（驱动）    ← 英伟达/AMD/Intel 的实际实现
  ↓
GPU 硬件
```
尽管Vulkan将错误检查与驱动剥离的思路是合理的，但另一方面这也意味着Vulkan核心驱动不做任何参数检查。不开验证层、不学习验证层在做什么，意味着崩溃、多平台不兼容等错误结果无从得知。

