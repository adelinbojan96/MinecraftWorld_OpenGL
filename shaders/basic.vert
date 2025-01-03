#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;

out vec3 fPosition; 
out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;
out vec4 fragPosLS_Herobrine; 
out vec3 sunLightPosFrag;

uniform vec3 sunLightPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix; 

void main() 
{
    vec4 worldPos = model * vec4(vPosition, 1.0);
    fPosition = worldPos.xyz;

    fPosEye = view * worldPos;

    fNormal = normalize(mat3(transpose(inverse(model))) * vNormal);
    
    fTexCoords = vTexCoords;
    
    fragPosLS_Herobrine = lightSpaceMatrix * worldPos;
    
    gl_Position = projection * view * worldPos;

    sunLightPosFrag = vec3(view * vec4(sunLightPos, 1.0));
}
