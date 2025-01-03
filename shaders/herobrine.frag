#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3 cameraPos;

uniform vec3 fogColor;
uniform float fogDensity;

uniform vec3 emissiveColor;
uniform float emissiveStrength;

uniform float time;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(fs_in.Normal, -lightDir)), 0.005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
   
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 viewDir = normalize(cameraPos - fs_in.FragPos);

    
    vec3 ambient = 0.15 * color;
    
   
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor * color;
    
   
    vec3 reflectDir = reflect(lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = spec * texture(specularTexture, fs_in.TexCoords).rgb;
    
   
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));

 
    vec3 emissive = emissiveColor * emissiveStrength;

    float distance = length(cameraPos - fs_in.FragPos);
    float fogFactor = 1.0 / exp(distance * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    lighting = mix(fogColor, lighting, fogFactor);
    
    FragColor = vec4(lighting + emissive, 1.0);
}
