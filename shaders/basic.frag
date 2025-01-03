#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

in vec4 fragPosLS_Herobrine;
in vec3 sunLightPosFrag;


out vec4 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3 lightPos2;
uniform vec3 lightColor2;

uniform vec3 torchLightPos1;
uniform vec3 torchLightPos2;
uniform vec3 torchLightPos3;
uniform vec3 torchLightColor;
uniform vec3 sunLightColor;

uniform vec3 lightPos3;
uniform vec3 lightColor3;

uniform vec3 mainSunLightPos;
uniform vec3 mainSunLightColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform vec3 fogColor; 
uniform float fogDensity;

uniform sampler2D shadowMap; 

float ambientStrength = 0.1;  
float specularStrength = 0.3; 

vec3 ambient;
vec3 diffuse;
vec3 specular;

vec3 diffuse2;
vec3 specular2;

vec3 diffuseTorchSum;
vec3 specularTorchSum;

vec3 diffuse3;
vec3 specular3;

void computeDirLight() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 lightDirN = normalize((view * vec4(lightDir, 0.0)).xyz);
    vec3 viewDir = normalize(-fPosEye.xyz);

    ambient += ambientStrength * lightColor;
    diffuse += min(max(dot(normalEye, lightDirN), 0.0) * lightColor, vec3(0.5));
    
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular += min(specularStrength * specCoeff * lightColor, vec3(0.3));
}
void computeSunLight() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    vec3 toLight = sunLightPosFrag - fPosEye.xyz;
    if (length(toLight) == 0.0) {
        toLight = vec3(0.0, 1.0, 0.0);
    }
    toLight = normalize(toLight);

    vec3 viewDir = normalize(-fPosEye.xyz);

    ambient += ambientStrength * sunLightColor;

    float diff = max(dot(normalEye, toLight), 0.0);
    diffuse += diff * sunLightColor;

    vec3 reflectDir = reflect(-toLight, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular += specularStrength * specCoeff * sunLightColor;
}

void computePointLight() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    vec3 lightPosEye = (view * vec4(lightPos2, 1.0)).xyz;
    vec3 toLight = normalize(lightPosEye - fPosEye.xyz);

    vec3 ambient2Local = ambientStrength * lightColor2;
    ambient += ambient2Local;

    float diff = max(dot(normalEye, toLight), 0.0);
    diffuse2 = min(diff * lightColor2, vec3(0.5));

    vec3 viewDir = normalize(-fPosEye.xyz);
    vec3 reflectDir = reflect(-toLight, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular2 = min(specularStrength * specCoeff * lightColor2, vec3(0.3));
}

void computeTorchLights() {
    diffuseTorchSum = vec3(0.0);
    specularTorchSum = vec3(0.0);

    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(-fPosEye.xyz);

    vec3 torchPositions[3] = vec3[3](torchLightPos1, torchLightPos2, torchLightPos3);
    for (int i = 0; i < 3; i++) {
        vec3 lp = (view * vec4(torchPositions[i], 1.0)).xyz;
        vec3 toLight = normalize(lp - fPosEye.xyz);

        float diff = max(dot(normalEye, toLight), 0.0);
        vec3 reflectDir = reflect(-toLight, normalEye);
        float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32);

        vec3 ambientTorch  = ambientStrength * torchLightColor;
        vec3 diffuseTorch  = min(diff * torchLightColor, vec3(0.5));
        vec3 specularTorch = min(specularStrength * specCoeff * torchLightColor, vec3(0.3));

        diffuseTorchSum  += ambientTorch + diffuseTorch;
        specularTorchSum += specularTorch;
    }
}

float computeShadow(in vec4 fragPosLS) {

    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

 
    if (projCoords.z > 1.0)
       return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int samples = 2; // Adjust as needed
    float offset = texelSize.x;

    for(int x = -samples; x <= samples; ++x){
        for(int y = -samples; y <= samples; ++y){
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            float currentDepth = projCoords.z;
            // Increased bias to push shadows closer
            float bias = max(0.015 * (1.0 - dot(normalize(fNormal), vec3(0, 0, -1))), 0.005);
            shadow += (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
        }
    }

    shadow /= float((2 * samples + 1) * (2 * samples + 1));

    return shadow;
}


float computeFogFactor(float distance) {
    float fogFactor = exp(-pow(distance * fogDensity, 2.0));
    return clamp(fogFactor, 0.0, 1.0);
}

void computePointLight3() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    vec3 lightPosEye3 = (view * vec4(lightPos3, 1.0)).xyz;
    vec3 toLight = normalize(lightPosEye3 - fPosEye.xyz);

    vec3 ambient3Local = ambientStrength * lightColor3;
    ambient += ambient3Local;

    float diff = max(dot(normalEye, toLight), 0.0);
    diffuse3 = min(diff * lightColor3, vec3(0.5));

    vec3 viewDir = normalize(-fPosEye.xyz);
    vec3 reflectDir = reflect(-toLight, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    specular3 = min(specularStrength * specCoeff * lightColor3, vec3(0.3));
}

void main() {
    vec3 normal = normalize(normalMatrix * fNormal);
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 viewDir = normalize(-fPosEye.xyz);

    ambient = vec3(0.0);
    diffuse = vec3(0.0);
    specular = vec3(0.0);
    diffuse2 = vec3(0.0);
    specular2 = vec3(0.0);
    diffuse3 = vec3(0.0);
    specular3 = vec3(0.0);


    computeDirLight();
    computePointLight();
    computeTorchLights();
    computePointLight3();
    computeSunLight();

    vec3 texDiff = texture(diffuseTexture, fTexCoords).rgb;
    vec3 texSpec = texture(specularTexture, fTexCoords).rgb;

    vec3 lightSum = (ambient + diffuse + specular) * texDiff
                  + (specular * texSpec)
                  + (diffuse2 + specular2) * texDiff
                  + (diffuse3 + specular3) * texDiff;

    vec3 torchResult = diffuseTorchSum * texDiff + specularTorchSum * texSpec;
    vec3 color = min(lightSum + torchResult, vec3(1.0));

    float shadowVal = computeShadow(fragPosLS_Herobrine);
    float shadowFactor = 1.0 - shadowVal;


    color *= shadowFactor;


    float fragmentDistance = length(fPosEye.xyz);
    float fogFactor = computeFogFactor(fragmentDistance);
    vec3 foggedColor = mix(fogColor, color, fogFactor);

    vec3 herobrineLightDir = normalize(mainSunLightPos - fPosition);
    float herobrineDiffuse = max(dot(normal, herobrineLightDir), 0.0);
    vec3 herobrineAmbient  = 0.05 * mainSunLightColor; // Reduced ambient
    vec3 herobrineDiffuseL = herobrineDiffuse * mainSunLightColor * 0.5; 
    vec3 herobrineLighting = herobrineAmbient + herobrineDiffuseL;

    vec3 finalColor = clamp(foggedColor + herobrineLighting * texDiff, 0.0, 1.0);

    fColor = vec4(finalColor, 1.0);
}