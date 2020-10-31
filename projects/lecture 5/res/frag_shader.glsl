#version 410


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

uniform vec3 LightPos;

out vec4 frag_color;


void main() {
    
    //lec5
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    float ambientStrength = 0.05;
    vec3 ambient = ambientStrength * lightColor * inColor;

    //diffuse
    vec3 N = normalize(inNormal);
    vec3 lightDir = normalize(LightPos - inPos);

    float dif = max(dot(N, lightDir), 0.0);
    vec3 diffuse = dif * inColor;// add diffuse intensity

    //attenuation
    float dist = length(LightPos - inPos);
    diffuse = diffuse / dist; // (dist*dist)

    // spectular
    vec3 camPos = vec3(0.0, 0.0, 3.0);//pass this as a uniform from your c++ code
    float specularStrength = 1.0;//can be uniform
    vec3 camDir = normalize(camPos - inPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(camDir, reflectDir), 0.0), 128); //shininess coefficent, should be uniform
    vec3 specular = specularStrength * spec * lightColor;//can also use a specular color

    vec3 result = (ambient + diffuse + specular);

    frag_color = vec4(result, 1.0);
}