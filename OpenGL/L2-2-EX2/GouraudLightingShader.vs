#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 LightingColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos,1.0f);

    vec3 Position = vec3(model * vec4(aPos,1.0f));
    vec3 Normal = mat3(transpose(inverse(model))) * aNormal;

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - Position);
    vec3 diffuse = max(dot(norm, lightDir), 0.0) * lightColor;

    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - Position);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = specularStrength * spec * lightColor;

    LightingColor = ambient + diffuse + specular ;
}