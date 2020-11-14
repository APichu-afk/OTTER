//Alexander Chow - 100745472
//Joseph Carrillo - 100746949
//Base code from Computer Graphics Tutorial 7
#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Gameplay/Camera.h"
#include "Gameplay/Transform.h"
#include "Graphics/Texture2D.h"
#include "Graphics/Texture2DData.h"
#include "Utilities/InputHelpers.h"
#include "Utilities/MeshBuilder.h"
#include "Utilities/MeshFactory.h"
#include "Utilities/NotObjLoader.h"
#include "Utilities/ObjLoader.h"
#include "Utilities/VertexTypes.h"

#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
		#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
		#endif
	default: break;
	}
}

GLFWwindow* window;
Camera::sptr camera = nullptr;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	camera->ResizeWindow(width, height);
}

bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	
	//Create a new GLFW window
	window = glfwCreateWindow(800, 800, "Birthday Splash Bash", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

void RenderVAO(
	const Shader::sptr& shader,
	const VertexArrayObject::sptr& vao,
	const Camera::sptr& camera,
	const Transform::sptr& transform)
{
	shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transform->LocalTransform());
	shader->SetUniformMatrix("u_Model", transform->LocalTransform());
	shader->SetUniformMatrix("u_NormalMatrix", transform->NormalMatrix());
	vao->Render();
}

void ManipulateTransformWithInput(const Transform::sptr& transform, const Transform::sptr& transform1, float dt) {
	//first player
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		transform->RotateLocal(0.0f, 0.3f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		transform->RotateLocal(0.0f, -0.3f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		transform->MoveLocal(0.0f, 0.0f, -9.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		transform->MoveLocal(0.0f, 0.0f, 9.0f * dt); 
	}

	//second player
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
		transform1->RotateLocal(0.0f, 0.3f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		transform1->RotateLocal(0.0f, -0.3f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		transform1->MoveLocal(0.0f, 0.0f, -9.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		transform1->MoveLocal(0.0f, 0.0f, 9.0f * dt); 
	}
}

// Borrowed collision from https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection
bool Collision(const Transform::sptr& transform1, const Transform::sptr& transform2)
{
	bool colX = transform1->GetLocalPosition().x  + transform1->GetLocalScale().x  >= transform2->GetLocalPosition().x
		&& transform2->GetLocalPosition().x + transform2->GetLocalScale().x *5>= transform1->GetLocalPosition().x;

	bool colY = transform1->GetLocalPosition().y + transform1->GetLocalScale().y >= transform2->GetLocalPosition().y
		&& transform2->GetLocalPosition().y + transform2->GetLocalScale().y >= transform1->GetLocalPosition().y;
	return colX && colY;
}

// Borrowed collision from https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection
bool Collision_right_left(const Transform::sptr& transform1, const Transform::sptr& transform2)
{
	bool colX = transform1->GetLocalPosition().x + transform1->GetLocalScale().x >= transform2->GetLocalPosition().x
		&& transform2->GetLocalPosition().x + transform2->GetLocalScale().x >= transform1->GetLocalPosition().x;

	bool colY = transform1->GetLocalPosition().y + transform1->GetLocalScale().y +20 >= transform2->GetLocalPosition().y
		&& transform2->GetLocalPosition().y + transform2->GetLocalScale().y >= transform1->GetLocalPosition().y;
	return colX && colY;
}

// Borrowed collision from https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection
bool Collision_top_wall(const Transform::sptr& transform1, const Transform::sptr& transform2)
{
	bool colX = transform1->GetLocalPosition().x + transform1->GetLocalScale().x +20 >= transform2->GetLocalPosition().x
		&& transform2->GetLocalPosition().x + transform2->GetLocalScale().x >= transform1->GetLocalPosition().x;

	bool colY = transform1->GetLocalPosition().y + transform1->GetLocalScale().y >= transform2->GetLocalPosition().y
		&& transform2->GetLocalPosition().y + transform2->GetLocalScale().y >= transform1->GetLocalPosition().y;
	return colX && colY;
}

struct Material
{
	Texture2D::sptr Albedo;
	Texture2D::sptr Albedo2;
	Texture2D::sptr Specular;
	float           Shininess;
	float			TextureMix;
};

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	
	//Vaos
	VertexArrayObject::sptr vaoplayer = ObjLoader::LoadFromFile("models/Dunce.obj");//player
	VertexArrayObject::sptr vaobackground = ObjLoader::LoadFromFile("models/Ground.obj");//Background
	VertexArrayObject::sptr vaoball = ObjLoader::LoadFromFile("models/MonkeyBar.obj");//MonkeyBars
	VertexArrayObject::sptr vaoleft = ObjLoader::LoadFromFile("models/LeftSide.obj");//wall left
	VertexArrayObject::sptr vaoup = ObjLoader::LoadFromFile("models/TopSide.obj");//wall top
	VertexArrayObject::sptr vaoright = ObjLoader::LoadFromFile("models/RightSide.obj");//wall right
	VertexArrayObject::sptr vao0 = ObjLoader::LoadFromFile("models/ZERO.obj");//0
	VertexArrayObject::sptr vao1 = ObjLoader::LoadFromFile("models/ONE.obj");//1
	VertexArrayObject::sptr vao2 = ObjLoader::LoadFromFile("models/TWO.obj");//2
	VertexArrayObject::sptr vao3 = ObjLoader::LoadFromFile("models/THREE.obj");//3
	VertexArrayObject::sptr vao4 = ObjLoader::LoadFromFile("models/FOUR.obj");//4
	VertexArrayObject::sptr vao5 = ObjLoader::LoadFromFile("models/FIVE.obj");//5
	VertexArrayObject::sptr vao6 = ObjLoader::LoadFromFile("models/SIX.obj");//6
	VertexArrayObject::sptr vaoballword = ObjLoader::LoadFromFile("models/BallsWord.obj");//balls
	VertexArrayObject::sptr vaoscore = ObjLoader::LoadFromFile("models/ScoreWord.obj");//score
	VertexArrayObject::sptr vaowin = ObjLoader::LoadFromFile("models/Win.obj");//win
	VertexArrayObject::sptr vaolose = ObjLoader::LoadFromFile("models/Lose.obj");//lose
	VertexArrayObject::sptr vaosandbox = ObjLoader::LoadFromFile("models/SandBox.obj");//SandBox
	VertexArrayObject::sptr vaoslide = ObjLoader::LoadFromFile("models/Slide.obj");//Slide
	VertexArrayObject::sptr vaoround = ObjLoader::LoadFromFile("models/RoundAbout.obj");//roundabout
	VertexArrayObject::sptr vaoswing = ObjLoader::LoadFromFile("models/Swing.obj");//swing
	VertexArrayObject::sptr vaoplayer2 = ObjLoader::LoadFromFile("models/Duncet.obj");//Second player
		
	// Load our shaders
	Shader::sptr shader = Shader::Create();
	shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
	shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);  
	shader->Link();  

	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 30.0f);
	glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	float     lightAmbientPow = 25.0f;
	float     lightSpecularPow = 1.0f;
	glm::vec3 ambientCol = glm::vec3(1.0f);
	float     ambientPow = 0.1f;
	float     shininess = 4.0f;
	float     lightLinearFalloff = 0.09f;
	float     lightQuadraticFalloff = 0.032f;
	
	// These are our application / scene level uniforms that don't necessarily update
	// every frame
	shader->SetUniform("u_LightPos", lightPos);
	shader->SetUniform("u_LightCol", lightCol);
	shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
	shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
	shader->SetUniform("u_AmbientCol", ambientCol);
	shader->SetUniform("u_AmbientStrength", ambientPow);
	shader->SetUniform("u_Shininess", shininess);
	shader->SetUniform("u_LightAttenuationConstant", 1.0f);
	shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
	shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

	// GL states
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// NEW STUFF

	// Create some transforms and initialize them
	Transform::sptr transforms[33];
	transforms[0] = Transform::Create();
	transforms[1] = Transform::Create();
	transforms[2] = Transform::Create();
	transforms[3] = Transform::Create();
	transforms[4] = Transform::Create();
	transforms[5] = Transform::Create();
	transforms[6] = Transform::Create();
	transforms[7] = Transform::Create();
	transforms[8] = Transform::Create();
	transforms[9] = Transform::Create();
	transforms[10] = Transform::Create();
	transforms[11] = Transform::Create();
	transforms[12] = Transform::Create();
	transforms[13] = Transform::Create();
	transforms[14] = Transform::Create();
	transforms[15] = Transform::Create();
	transforms[16] = Transform::Create();
	transforms[17] = Transform::Create();
	transforms[18] = Transform::Create();
	transforms[19] = Transform::Create();
	transforms[20] = Transform::Create();
	transforms[21] = Transform::Create();
	transforms[22] = Transform::Create();
	transforms[23] = Transform::Create();
	transforms[24] = Transform::Create();
	transforms[25] = Transform::Create();
	transforms[26] = Transform::Create();
	transforms[27] = Transform::Create();
	transforms[28] = Transform::Create();
	transforms[29] = Transform::Create();

	// We can use operator chaining, since our Set* methods return a pointer to the instance, neat!
	transforms[0]->SetLocalPosition(0.0f, -8.0f, 1.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Player
	transforms[1]->SetLocalPosition(15.0f, 20.0f, -5.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Background
	transforms[2]->SetLocalPosition(10.0f, 20.0f, -0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Ball
	transforms[3]->SetLocalPosition(-11.0f, -9.0f, -1.0f)->SetLocalRotation(90.0f, 0.0f, 180.0f)->SetLocalScale(1.0f, 1.5f, 2.0f);//left wall hitbox
	transforms[4]->SetLocalPosition(-11.0f, 12.5f, -1.0f)->SetLocalRotation(90.0f, 0.0f, 180.0f)->SetLocalScale(1.8f, 1.5f, 2.0f);//top wall hitbox
	transforms[5]->SetLocalPosition(10.0f, -9.0f, -1.0f)->SetLocalRotation(90.0f, 0.0f, 180.0f)->SetLocalScale(1.0f, 1.5f, 2.0f);//right wall hitbox
	transforms[6]->SetLocalPosition(10.0f, 12.0f, -3.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//SandBox
	transforms[7]->SetLocalPosition(-25.0f, 5.0f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 90.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Slide
	transforms[8]->SetLocalPosition(-6.0f, 9.0f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Roundabout
	transforms[9]->SetLocalPosition(7.0f, 9.0f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//Swing
	transforms[10]->SetLocalPosition(0.0f,-6.0f, 1.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(2.5f, 2.5f, 2.5f);//player 2
	transforms[11]->SetLocalPosition(7.0f, 6.0f, 0.0f)->SetLocalRotation(00.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//Brick 6
	transforms[12]->SetLocalPosition(-4.7f, 5.0f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, .5f, .5f);//0 score
	transforms[13]->SetLocalPosition(-4.85f, 5.8f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//1 score
	transforms[14]->SetLocalPosition(-5.0f, 6.55f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//2 score
	transforms[15]->SetLocalPosition(-5.15f, 7.3f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//3 score
	transforms[16]->SetLocalPosition(-5.3f, 8.1f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//4 score
	transforms[17]->SetLocalPosition(-5.45f, 8.9f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//5 score
	transforms[18]->SetLocalPosition(-5.6f, 9.7f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//6 score
	transforms[19]->SetLocalPosition(-8.0f, 8.0f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//balls
	transforms[20]->SetLocalPosition(-8.0f, 8.0f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(.75f, .5f, .5f);//score
	transforms[21]->SetLocalPosition(-8.0f, -5.0f, 7.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(4.0f, 4.0f, 4.0f);//win
	transforms[22]->SetLocalPosition(-8.0f, -5.0f, 9.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(4.0f, 4.0f, 4.0f);//lose
	transforms[23]->SetLocalPosition(-5.28f, 6.4f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, 0.5f, 0.5f);//1 lives
	transforms[24]->SetLocalPosition(-5.4f, 7.2f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, 0.5f, 0.5f);//2 lives
	transforms[25]->SetLocalPosition(-5.5f, 8.0f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, 0.5f, 0.5f);//3 lives
	transforms[26]->SetLocalPosition(-3.9f, 5.0f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, .5f, .5f);//0 for hundred
	transforms[27]->SetLocalPosition(-4.3f, 5.0f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, .5f, .5f);//0 for hundred
	transforms[28]->SetLocalPosition(-11.0f, -11.0f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 180.0f)->SetLocalScale(1.8f, 1.5f, 2.0f);//bottom wall hitbox
	transforms[29]->SetLocalPosition(-5.1f, 5.6f, 5.0f)->SetLocalRotation(120.0f, 0.0f, 0.0f)->SetLocalScale(0.75f, .5f, .5f);//lives 0

	// We'll store all our VAOs into a nice array for easy access
	VertexArrayObject::sptr vaos[22];
	vaos[0] = vaoplayer;
	vaos[1] = vaobackground;
	vaos[2] = vaoball;
	vaos[3] = vaoleft;
	vaos[4] = vaoup;
	vaos[5] = vaoright;
	vaos[6] = vao0;
	vaos[7] = vao1;
	vaos[8] = vao2;
	vaos[9] = vao3;
	vaos[10] = vao4;
	vaos[11] = vao5;
	vaos[12] = vao6;
	vaos[13] = vaoballword;
	vaos[14] = vaoscore;
	vaos[15] = vaowin;
	vaos[16] = vaolose;
	vaos[17] = vaosandbox;
	vaos[18] = vaoslide;
	vaos[19] = vaoround;
	vaos[20] = vaoplayer2;
	vaos[21] = vaoswing;

	// TODO: load textures
	//need to somehow make this thing make multiple textures(hard for some reason)
	// Load our texture data from a file
	Texture2DData::sptr diffuseMap = Texture2DData::LoadFromFile("images/Green.jpg");
	Texture2DData::sptr diffuseMap2 = Texture2DData::LoadFromFile("images/box.bmp");
	Texture2DData::sptr diffuseMapbrick = Texture2DData::LoadFromFile("images/Sand.jpg");
	Texture2DData::sptr diffuseMapbrickstrong = Texture2DData::LoadFromFile("images/box.bmp");
	Texture2DData::sptr diffuseBall = Texture2DData::LoadFromFile("images/Green.jpg");
	Texture2DData::sptr specularMap = Texture2DData::LoadFromFile("images/Stone_001_Specular.png");

	// Create a texture from the data
	Texture2D::sptr diffuse = Texture2D::Create();
	diffuse->LoadData(diffuseMap);

	Texture2D::sptr specular = Texture2D::Create();
	specular->LoadData(specularMap);

	Texture2D::sptr diffuse2 = Texture2D::Create();
	diffuse2->LoadData(diffuseMap2);

	//brick texture
	Texture2D::sptr diffuseBrick = Texture2D::Create();
	diffuseBrick->LoadData(diffuseMapbrick);
	
	//strong brick texture
	Texture2D::sptr diffuseBrickstrong = Texture2D::Create();
	diffuseBrickstrong->LoadData(diffuseMapbrickstrong);

	//ball texture
	Texture2D::sptr diffusellab = Texture2D::Create();
	diffusellab->LoadData(diffuseBall);

	// Creating an empty texture
	Texture2DDescription desc = Texture2DDescription();
	desc.Width = 1;
	desc.Height = 1;
	desc.Format = InternalFormat::RGB8;
	Texture2D::sptr texture2 = Texture2D::Create(desc);
	texture2->Clear(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// TODO: store some info about our materials for each object
	Material materials[9];
	//player
	materials[0].Albedo = diffuse;
	materials[0].Albedo2 = diffuse2;
	materials[0].Specular = specular;
	materials[0].Shininess = 4.0f;
	materials[0].TextureMix = 0.0f;

	//Background
	materials[1].Albedo = diffuse;
	materials[1].Albedo2 = diffuse2;
	materials[1].Specular = specular;
	materials[1].Shininess = 16.0f;
	materials[1].TextureMix = 0.0f;

	//MonkeyBar
	materials[2].Albedo = diffuse;
	materials[2].Albedo2 = diffusellab;
	materials[2].Specular = specular;
	materials[2].Shininess = 32.0f;
	materials[2].TextureMix = 1.0f;
	
	//SandBox
	materials[3].Albedo = diffuseBrick;
	materials[3].Albedo2 = diffuseBrickstrong;
	materials[3].Specular = specular;
	materials[3].Shininess = 32.0f;
	materials[3].TextureMix = 0.5f;

	//Slide
	materials[4].Albedo = diffuse;
	materials[4].Albedo2 = diffuseBrickstrong;
	materials[4].Specular = specular;
	materials[4].Shininess = 32.0f;
	materials[4].TextureMix = 0.0f;
	
	materials[5].Albedo = diffuse;
	materials[5].Albedo2 = diffuse2;
	materials[5].Specular = specular;
	materials[5].Shininess = 32.0f;
	materials[5].TextureMix = 1.0f;

	//Camera
	camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, -3, 40)); // Set initial position
	camera->SetUp(glm::vec3(0, 0, 1)); // Use a z-up coordinate system
	camera->LookAt(glm::vec3(0.0f)); // Look at center of the screen
	camera->SetFovDegrees(90.0f); // Set an initial FOV
	camera->SetOrthoHeight(3.0f);
		
	// Our high-precision timer
	double lastFrame = glfwGetTime();
	//makes things not render multiple because we don't want to stop rendering all when the ball only touches one
	bool norender = false;
	bool norender2 = false;
	bool norender3 = false;
	bool norender4 = false;
	bool norender5 = false;
	bool norender6 = false;
	//itegers for score and lives
	int score = 0;
	int lives = 3;
	//resets the position of the ball
	glm::vec3 ballpos = transforms[2]->GetLocalPosition();
	
	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		ManipulateTransformWithInput(transforms[0], transforms[10], dt);

		//colour of the background
		glClearColor(0.08f, 0.17f, 0.31f, 1.0f);//rgb
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->Bind();
		// These are the uniforms that update only once per frame
		shader->SetUniformMatrix("u_View", camera->GetView());
		shader->SetUniform("u_CamPos", camera->GetPosition());

		// Tell OpenGL that slot 0 will hold the diffuse, and slot 1 will hold the specular
		shader->SetUniform("s_Diffuse", 0);
		shader->SetUniform("s_Diffuse2", 1);
		shader->SetUniform("s_Specular", 2);

		int r = (rand() % 2 + 1) - 1;

		// Render all VAOs in our scene
		for (int ix = 0; ix < 3; ix++) {
			// TODO: Apply materials
			materials[ix].Albedo->Bind(0);
			materials[ix].Albedo2->Bind(1);
			materials[ix].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[ix].Shininess);
			shader->SetUniform("u_TextureMix", materials[ix].TextureMix);
			RenderVAO(shader, vaos[ix], camera, transforms[ix]);
		}
		materials[3].Albedo->Bind(0);
		materials[3].Albedo2->Bind(1);
		materials[3].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[3].Shininess);
		shader->SetUniform("u_TextureMix", materials[3].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[28]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[28]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[28]->LocalTransform()));

		//ball movement
		//transforms[2]->MoveLocal(-8.0f * dt, -6.0f * dt, 0);
		//collision of kind of works(hopefully usable in our GDW)
		{
			//paddle and ball collison
			/*if (Collision(transforms[0], transforms[2]))
			{
				if (r) {
					transforms[2]->RotateLocal(0, 0, 90);
				}
				if (!r) {
					transforms[2]->RotateLocal(0, 0, -90);
				}
			}
			if (Collision_right_left(transforms[3], transforms[2]))
			{
				transforms[2]->RotateLocal(0, 0, 90);
			}
			if (Collision_top_wall(transforms[4], transforms[2]))
			{
				transforms[2]->RotateLocal(0, 0, 90);
			}	
			if (Collision_top_wall(transforms[28], transforms[2]))
			{
				lives = lives - 1;
				transforms[2]->SetLocalPosition(ballpos);
			}
			if (Collision_right_left(transforms[5], transforms[2]))
			{
				transforms[2]->RotateLocal(0, 0, 90);
			}*/
			//rendering like this to control when a brick gets deleted
				materials[3].Albedo->Bind(0);
				materials[3].Albedo2->Bind(1);
				materials[3].Specular->Bind(2);
				shader->SetUniform("u_Shininess", materials[3].Shininess);
				shader->SetUniform("u_TextureMix", materials[3].TextureMix);
				shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[6]->LocalTransform());
				shader->SetUniformMatrix("u_Model", transforms[6]->LocalTransform());
				shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[6]->LocalTransform()));
				vaosandbox->Render();
			

			//if (Collision(transforms[7], transforms[2]) == false) {
				//if (norender2 == false) {
					materials[4].Albedo->Bind(0);
					materials[4].Albedo2->Bind(1);
					materials[4].Specular->Bind(2);
					shader->SetUniform("u_Shininess", materials[4].Shininess);
					shader->SetUniform("u_TextureMix", materials[4].TextureMix);
					shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[7]->LocalTransform());
					shader->SetUniformMatrix("u_Model", transforms[7]->LocalTransform());
					shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[7]->LocalTransform()));
					vaoslide->Render();
				//}
			//}
			/*else {
				norender2 = true;
				score = score + 1;
				transforms[2]->RotateLocal(0, 0, -90);
				transforms[7]->MoveLocal(0, 100, 0);
			}*/

			//if (Collision(transforms[8], transforms[2]) == false) {
				//if (norender3 == false) {
					materials[4].Albedo->Bind(0);
					materials[4].Albedo2->Bind(1);
					materials[4].Specular->Bind(2);
					shader->SetUniform("u_Shininess", materials[4].Shininess);
					shader->SetUniform("u_TextureMix", materials[4].TextureMix);
					shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[8]->LocalTransform());
					shader->SetUniformMatrix("u_Model", transforms[8]->LocalTransform());
					shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[8]->LocalTransform()));
					vaoround->Render();
				//}
			//}
			/*else {
				norender3 = true;
				score = score + 1;
				transforms[2]->RotateLocal(0, 0, -90);
				transforms[8]->MoveLocal(0, 100, 0);
			}*/

			//if (Collision(transforms[9], transforms[2]) == false) {
				//if (norender4 == false) {
					materials[4].Albedo->Bind(0);
					materials[4].Albedo2->Bind(1);
					materials[4].Specular->Bind(2);
					shader->SetUniform("u_Shininess", materials[4].Shininess);
					shader->SetUniform("u_TextureMix", materials[4].TextureMix);
					shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[9]->LocalTransform());
					shader->SetUniformMatrix("u_Model", transforms[9]->LocalTransform());
					shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[9]->LocalTransform()));
					vaoswing->Render();
				//}
			//}
			/*else {
				norender4 = true;
				score = score + 1;
				transforms[2]->RotateLocal(0, 0, 90);
				transforms[9]->MoveLocal(0, 100, 0);
			}*/

			//if (Collision(transforms[10], transforms[2]) == false) {
				//if (norender5 == false) {
					materials[1].Albedo->Bind(0);
					materials[1].Albedo2->Bind(1);
					materials[1].Specular->Bind(2);
					shader->SetUniform("u_Shininess", materials[1].Shininess);
					shader->SetUniform("u_TextureMix", materials[1].TextureMix);
					shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[10]->LocalTransform());
					shader->SetUniformMatrix("u_Model", transforms[10]->LocalTransform());
					shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[10]->LocalTransform()));
					vaoplayer2->Render();
				//}
			//}
			/*
			else {
				norender5 = true;
				score = score + 1;
				transforms[2]->RotateLocal(0, 0, -90);
				transforms[10]->MoveLocal(0, 100, 0);
			}

			if (Collision(transforms[11], transforms[2]) == false) {
				if (norender6 == false) {
					materials[3].Albedo->Bind(0);
					materials[3].Albedo2->Bind(1);
					materials[3].Specular->Bind(2);
					shader->SetUniform("u_Shininess", materials[3].Shininess);
					shader->SetUniform("u_TextureMix", materials[3].TextureMix);
					shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[11]->LocalTransform());
					shader->SetUniformMatrix("u_Model", transforms[11]->LocalTransform());
					shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[11]->LocalTransform()));
					vaoplayer->Render();
				}
			}
			else {
				norender6 = true;
				score = score + 1;
				transforms[2]->RotateLocal(0, 0, -90);
				transforms[11]->MoveLocal(0, 100, 0);
			}*/
		}
		/*//score numbers
		//00
		materials[4].Albedo->Bind(0);
		materials[4].Albedo2->Bind(1);
		materials[4].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[4].Shininess);
		shader->SetUniform("u_TextureMix", materials[4].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[26]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[26]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[26]->LocalTransform()));
		vao0->Render();
		
		materials[4].Albedo->Bind(0);
		materials[4].Albedo2->Bind(1);
		materials[4].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[4].Shininess);
		shader->SetUniform("u_TextureMix", materials[4].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[27]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[27]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[27]->LocalTransform()));
		vao0->Render();
		//0 score
		if (score == 0) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[12]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[12]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[12]->LocalTransform()));
			vao0->Render();
		}

		//1 score
		if (score == 1) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[13]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[13]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[13]->LocalTransform()));
			vao1->Render();
		}

		//2 score
		if (score == 2) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[14]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[14]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[14]->LocalTransform()));
			vao2->Render();
		}

		//3 score
		if (score == 3) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[15]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[15]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[15]->LocalTransform()));
			vao3->Render();
		}

		//4 score
		if (score == 4) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[16]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[16]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[16]->LocalTransform()));
			vao4->Render();
		}

		//5 score
		if (score == 5) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[17]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[17]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[17]->LocalTransform()));
			vao5->Render();
		}

		//6 score
		if (score == 6) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[18]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[18]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[18]->LocalTransform()));
			vao6->Render();
		}
		
		//1 life
		if (lives == 1) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[23]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[23]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[23]->LocalTransform()));
			vao1->Render();
		}

		//2 lives
		if (lives == 2) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[24]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[24]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[24]->LocalTransform()));
			vao2->Render();
		}

		//3 lives
		if (lives == 3) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[25]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[25]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[25]->LocalTransform()));
			vao3->Render();
		}
		//0 lives
		if (lives == 0) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[29]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[29]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[29]->LocalTransform()));
			vao0->Render();
		}

		//balls
		materials[4].Albedo->Bind(0);
		materials[4].Albedo2->Bind(1);
		materials[4].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[4].Shininess);
		shader->SetUniform("u_TextureMix", materials[4].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[19]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[19]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[19]->LocalTransform()));
		vaoballword->Render();

		//score
		materials[4].Albedo->Bind(0);
		materials[4].Albedo2->Bind(1);
		materials[4].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[4].Shininess);
		shader->SetUniform("u_TextureMix", materials[4].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[20]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[20]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[20]->LocalTransform()));
		vaoscore->Render();

		//win screen
		if (norender == true && norender2 == true && norender3 == true && norender4 == true && norender5 == true && norender6 == true){
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[21]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[21]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[21]->LocalTransform()));
			vaowin->Render();
			transforms[2]->MoveLocal(8.0f * dt, 6.0f * dt, 0);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				transforms[0]->MoveLocal(9.0f * dt, 0.0f, 0.0f);
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				transforms[0]->MoveLocal(-9.0f * dt, 0.0f, 0.0f);
			}
		}
		//lose screen
		if (lives == 0) {
			materials[4].Albedo->Bind(0);
			materials[4].Albedo2->Bind(1);
			materials[4].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[4].Shininess);
			shader->SetUniform("u_TextureMix", materials[4].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[22]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[22]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[22]->LocalTransform()));
			vaolose->Render();
			transforms[2]->MoveLocal(8.0f * dt, 6.0f * dt, 0);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				transforms[0]->MoveLocal(9.0f * dt, 0.0f, 0.0f);
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				transforms[0]->MoveLocal(-9.0f * dt, 0.0f, 0.0f);
			}
		}*/
		glfwSwapBuffers(window);
		lastFrame = thisFrame;
	}
	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}