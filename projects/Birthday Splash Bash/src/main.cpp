#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Camera.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexArrayObject.h"
#include "MeshBuilder.h"
#include "MeshFactory.h"
#include "OBJLoader.h"
#include "VertexTypes.h"
#include "Shad.h"

//initializes the window
GLFWwindow* firstWindow;
//Makes the Camera
Camera::sptr camera = nullptr;

bool GLFW()
{
	//checks if glfw died
	if (glfwInit() == GLFW_FALSE)
	{
		std::cout << "GLFW fail" << std::endl;
		return false;
	}
	//								 size			 Title			 monitor?  share?
	firstWindow = glfwCreateWindow(800, 800, "Birthday Splash Bash", nullptr, nullptr);
	glfwMakeContextCurrent(firstWindow);

	return true;
}

bool GLAD()
{
	//checks if glad died
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
	{
		std::cout << "glad failed" << std::endl;
		return false;
	}
}

int main()
{
	//initialize GLFW
	if (!GLFW())
		return -1;

	//initialize GLAD
	if (!GLAD())
		return -1;

	//Loads in the obj
<<<<<<< HEAD:projects/CGMidterm/src/main.cpp
	VertexArrayObject::sptr MainCharacter = OBJLoader::LoadFile("Paddle.obj");
=======
	VertexArrayObject::sptr MainCharacter = OBJLoader::LoadFile("monkey.obj");
>>>>>>> master:projects/Birthday Splash Bash/src/main.cpp

	Shader::sptr shader = Shader::Create();
	shader->LoadShaderPartFromFile("vertex_shader.glsl", GL_VERTEX_SHADER);
	shader->LoadShaderPartFromFile("frag_blinn_phong.glsl", GL_FRAGMENT_SHADER);
	shader->Link();

	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 2.0f);
	glm::vec3 lightCol = glm::vec3(0.3f, 0.2f, 0.5f);
	float     lightAmbientPow = 0.05f;
	float     lightSpecularPow = 1.0f;
	glm::vec3 ambientCol = glm::vec3(1.0f);
	float     ambientPow = 0.1f;
	float     shininess = 4.0f;

	shader->SetUniform("u_LightPos", lightPos);
	shader->SetUniform("u_LightCol", lightCol);
	shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
	shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
	shader->SetUniform("u_AmbientCol", ambientCol);
	shader->SetUniform("u_AmbientStrength", ambientPow);
	shader->SetUniform("u_Shininess", shininess);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glm::mat4 transform = glm::mat4(1.0f);

	camera = Camera::Create();//camera initialized
<<<<<<< HEAD:projects/CGMidterm/src/main.cpp
	camera->SetPosition(glm::vec3(0, 3, 6)); // Sets the position
=======
	camera->SetPosition(glm::vec3(0, 3, 3)); // Sets the position
>>>>>>> master:projects/Birthday Splash Bash/src/main.cpp
	camera->SetUp(glm::vec3(0, 0, 1)); // Use a z-up coordinate system
	camera->LookAt(glm::vec3(0.0f)); // Sets where its looking
	camera->SetFovDegrees(90.0f); // Set the FOV
	
	double lastFrame = glfwGetTime();
<<<<<<< HEAD:projects/CGMidterm/src/main.cpp
	glm::vec3 l;

	//transform = glm::translate(transform, glm::vec3(0.0f, -2.0f, 0.0f));
=======
>>>>>>> master:projects/Birthday Splash Bash/src/main.cpp

	//Game loop/makes the window//
	while (!glfwWindowShouldClose(firstWindow))
	{
		//lets the user click on the window, maybe?
		glfwPollEvents();

		double thisFrame = glfwGetTime();

		float dt = static_cast<float>(thisFrame - lastFrame);

<<<<<<< HEAD:projects/CGMidterm/src/main.cpp
		transform = glm::rotate(glm::mat4(1.0f), static_cast<float>(thisFrame), glm::vec3(0, 0, 1));

		if (glfwGetKey(firstWindow, GLFW_KEY_LEFT) == GLFW_PRESS) {
			transform = glm::translate(transform, glm::vec3(-0.001f, 0.0f, 0.0f));//gains speed for some reason when using dt
=======
		if (glfwGetKey(firstWindow, GLFW_KEY_A) == GLFW_PRESS) {
			transform = glm::translate(transform, glm::vec3(0.01f, 0.0f, 0.0f));//gains speed for some reason when using dt
		}
		if (glfwGetKey(firstWindow, GLFW_KEY_D) == GLFW_PRESS) {
			transform = glm::translate(transform, glm::vec3(-0.01f, 0.0f, 0.0f));//gains speed for some reason when using dt
		}
		if (glfwGetKey(firstWindow, GLFW_KEY_W) == GLFW_PRESS) {
			transform = glm::translate(transform, glm::vec3(0.0f, -0.01f, 0.0f));//gains speed for some reason when using dt
>>>>>>> master:projects/Birthday Splash Bash/src/main.cpp
		}
		if (glfwGetKey(firstWindow, GLFW_KEY_S) == GLFW_PRESS) {
			transform = glm::translate(transform, glm::vec3(0.0f, 0.01f, 0.0f));//gains speed for some reason when using dt
		}
<<<<<<< HEAD:projects/CGMidterm/src/main.cpp
		
=======
>>>>>>> master:projects/Birthday Splash Bash/src/main.cpp

		//Changes Colour of our background
		glClearColor(0.1f, 0.0f, 0.1f, 1.0f);//RGB, Alpha? dark purple right now 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->Bind();

		shader->SetUniform("u_CamPos", camera->GetPosition());

		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transform);
		shader->SetUniformMatrix("u_Model", transform);
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transform));
		MainCharacter->Render();

		glfwSwapBuffers(firstWindow);
	}
	Logger::Uninitialize();
	return 0;
} 
