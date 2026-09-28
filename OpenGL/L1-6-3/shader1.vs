#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

// uniform float xOffset;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // gl_Position = transform * vec4(aPos,1.0f);
    // 注意顺序
    gl_Position = projection * view * model * vec4(aPos,1.0f);
    TexCoord = aTexCoord;
}