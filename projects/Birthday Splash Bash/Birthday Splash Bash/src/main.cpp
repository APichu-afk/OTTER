#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>

using namespace glm;

//initializes the window
GLFWwindow* firstWindow;

bool GLFW()
{
	//checks if glfw died
	if (glfwInit() == GLFW_FALSE)
	{
		std::cout << "GLFW fail" << std::endl;
		return false;
	}
	//								size			Title			 monitor?  share?
	firstWindow = glfwCreateWindow(500, 500, "Birthday Splash Bash", nullptr, nullptr);
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
	{
		return -1;
	}

	//Game loop/makes the window//
	while (!glfwWindowShouldClose(firstWindow))
	{
		//clears the screen no flickering
		glClear(GL_COLOR_BUFFER_BIT);

		//swaps buffers
		glfwSwapBuffers(firstWindow);
		glfwPollEvents();
	}

	return 0;
} 
