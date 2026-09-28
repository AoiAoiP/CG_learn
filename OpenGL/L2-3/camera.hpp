#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 设定默认参数
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float MOVEMENT_SPEED = 2.5f;
const float MOUSE_SENSITIVITY = 0.1f;
const float ZOOM = 75.0f;

class Camera{
private:
    // Camera属性
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;
    glm::vec3 WorldUp;
    // Euler角
    float Yaw;
    float Pitch;
    // Camera设置
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    void updateCameraVectors(){
        glm::vec3 front = glm::vec3(
            cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
            sin(glm::radians(Pitch)),
            sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))
        );
        cameraFront = glm::normalize(front);
        cameraRight = glm::normalize(glm::cross(cameraFront, WorldUp));
        cameraUp    = glm::normalize(glm::cross(cameraRight, cameraFront));
    }
public:
    Camera(){};
    Camera(glm::vec3 cameraPos=glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f),float Yaw = YAW,float Pitch = PITCH):cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),MovementSpeed(MOVEMENT_SPEED),MouseSensitivity(MOUSE_SENSITIVITY),Zoom(ZOOM)
    {
        this->cameraPos = cameraPos;
        this->WorldUp = cameraUp;
        this->Yaw = Yaw;
        this->Pitch = Pitch;
        updateCameraVectors();
    };
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(MOVEMENT_SPEED), MouseSensitivity(MOUSE_SENSITIVITY), Zoom(ZOOM)
    {
        cameraPos = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix(){
        return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }

    float GetZoom(){
        return Zoom;
    }

    glm::vec3 GetPosition(){
        return cameraPos;
    }

    void ProcessKeyboardInput(char direction, float deltaTime){
        float velocity = MovementSpeed * deltaTime;
        if (direction == 'W')
            cameraPos += cameraFront * velocity;
        if (direction == 'S')
            cameraPos -= cameraFront * velocity;
        if (direction == 'A')
            cameraPos -= cameraRight * velocity;
        if (direction == 'D')
            cameraPos += cameraRight * velocity;
        if (direction == ' ')
            cameraPos += cameraUp * velocity;
        if (direction == 'C')
            cameraPos -= cameraUp * velocity;
        // cameraPos.y = 0.0f; // 相机高度固定为0
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true){
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset){
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 85.0f)
            Zoom = 85.0f;
    }
};
