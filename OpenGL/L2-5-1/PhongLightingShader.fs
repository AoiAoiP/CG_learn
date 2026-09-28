#version 330 core
out vec4 FragColor;

struct Material {
    vec3 specular;    
    float shininess;
}; 

struct Light {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform sampler2D material_diffuse;
uniform sampler2D material_specular;
uniform sampler2D material_emission;
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(material_diffuse, TexCoords).rgb;
    // vec3 ambient = vec3(1.0f);
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material_diffuse, TexCoords).rgb;  
    // vec3 diffuse = light.diffuse * diff * vec3(1.0f);
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material_specular, TexCoords).rgb;  
    // vec3 specular = light.specular * spec * (vec3(1.0f)-texture(material_specular, TexCoords).rgb); 

    // emission
    vec3 emission = texture(material_emission, TexCoords).rgb;
        
    vec3 result = ambient + diffuse + specular + emission;
    FragColor = vec4(result, 1.0);
} 