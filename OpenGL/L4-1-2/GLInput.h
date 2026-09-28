#pragma once

#include <GLFW/glfw3.h>
#include "camera.hpp"

constexpr int SCREEN_WIDTH = 1080;
constexpr int SCREEN_HEIGHT = 960;

extern float lastX,lastY;
extern bool firstMouse;
extern Camera camera;

void processInput(GLFWwindow *window, float deltaTime);

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);