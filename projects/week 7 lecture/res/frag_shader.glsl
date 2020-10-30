#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
<<<<<<< HEAD
//LECTURE 7
=======
//lecture 7
>>>>>>> parent of b0dabbb... CGMidtermstart
layout(location = 3) in vec2 UV;

uniform sampler2D textureSampler;

uniform vec3 LightPos;


out vec4 frag_color;


void main() {
	
<<<<<<< HEAD
	vec3 textureColor = texture(textureSampler, UV).xyz;


=======
>>>>>>> parent of b0dabbb... CGMidtermstart
	// Lecture 5
	vec3 lightColor = vec3(1.0, 1.0, 1.0);

	float ambientStrength = 0.1;
<<<<<<< HEAD
	vec3 ambient = ambientStrength * lightColor * textureColor;//inColor;
=======
	vec3 ambient = ambientStrength * lightColor * inColor;
>>>>>>> parent of b0dabbb... CGMidtermstart

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(LightPos - inPos);
	
	float dif = max(dot(N, lightDir), 0.0);
<<<<<<< HEAD
	vec3 diffuse = dif * textureColor;//inColor;// add diffuse intensity
=======
	vec3 diffuse = dif * inColor;// add diffuse intensity
>>>>>>> parent of b0dabbb... CGMidtermstart

	//Attenuation
	float dist = length(LightPos - inPos);
	diffuse = diffuse / dist; // (dist*dist)
	
	// Specular
	vec3 camPos = vec3(0.0, 0.0, 3.0) ;//Pass this as a uniform from your C++ code
	float specularStrength = 1.0; // this can be a uniform
	vec3 camDir = normalize(camPos - inPos);
	vec3 reflectDir = reflect(-lightDir, N);
	float spec = pow(max(dot(camDir, reflectDir), 0.0), 4); // Shininess coefficient (can be a uniform)
	vec3 specular = specularStrength * spec * lightColor; // Can also use a specular color

	vec3 result = (ambient + diffuse + specular);

<<<<<<< HEAD
	frag_color = texture(textureSampler, UV) * vec4(result, 1.0);
=======
	frag_color = vec4(result, 1.0) * texture(textureSampler, UV);
>>>>>>> parent of b0dabbb... CGMidtermstart
}