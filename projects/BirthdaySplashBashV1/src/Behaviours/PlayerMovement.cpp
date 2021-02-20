#include "PlayerMovement.h"
#include "Gameplay/Application.h"
#include "Gameplay/Transform.h"
#include "Gameplay/Timing.h"

#include "GLFW/glfw3.h"

PlayerMovement::PlayerMovement()
{//Defualt constructor
}

PlayerMovement::~PlayerMovement()
{//Default destrcotor
}

//Working player Movement
void PlayerMovement::player1and2move(Transform& Player1, Transform& Player2, float dt)
{
	GLFWwindow* window = Application::Instance().Window;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		Player1.MoveLocal(0.0f, 0.0f, 5.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		Player1.RotateLocal(0.0f, 225.0f * dt, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		Player1.MoveLocal(0.0f, 0.0f, -5.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		Player1.RotateLocal(0.0f, -225.0f * dt, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		Player2.MoveLocal(0.0f, 0.0f, 5.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
		Player2.RotateLocal(0.0f, 225.0f * dt, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		Player2.MoveLocal(0.0f, 0.0f, -5.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		Player2.RotateLocal(0.0f, -225.0f * dt, 0.0f);
	}
}

//Only called if colliding with a wall or player 2
void PlayerMovement::Player1vswall(Transform& Player1, float dt)
{
	GLFWwindow* window = Application::Instance().Window;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		Player1.MoveLocal(0.0f, 0.0f, -5.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		Player1.MoveLocal(0.0f, 0.0f, 5.0f * dt);
	}
}

void PlayerMovement::Player2vswall(Transform& Player2, float dt)
{
	GLFWwindow* window = Application::Instance().Window;

	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		Player2.MoveLocal(0.0f, 0.0f, -5.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		Player2.MoveLocal(0.0f, 0.0f, 5.0f * dt);
	}
}
