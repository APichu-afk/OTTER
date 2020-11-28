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

void ManipulateTransformWithInput(const Transform::sptr& transformPlayer, const Transform::sptr& transformPlayer2, float dt) {
	//first player
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		transformPlayer->RotateLocal(0.0f, 0.25f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		transformPlayer->RotateLocal(0.0f, -0.25, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		transformPlayer->MoveLocal(0.0f, 0.0f, 9.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		transformPlayer->MoveLocal(0.0f, 0.0f, -9.0f * dt); 
	}

	//second player
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
		transformPlayer2->RotateLocal(0.0f, 0.25f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		transformPlayer2->RotateLocal(0.0f, -0.25f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		transformPlayer2->MoveLocal(0.0f, 0.0f, 9.0f * dt);
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		transformPlayer2->MoveLocal(0.0f, 0.0f, -9.0f * dt); 
	}
}

// Borrowed collision from https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection
bool Collision(const Transform::sptr& transform1, const Transform::sptr& transform2)
{
	bool colX = transform1->GetLocalPosition().x + transform1->GetLocalScale().x >= transform2->GetLocalPosition().x
		&& transform2->GetLocalPosition().x + transform2->GetLocalScale().x >= transform1->GetLocalPosition().x;

	bool colY = transform1->GetLocalPosition().y + transform1->GetLocalScale().y >= transform2->GetLocalPosition().y
		&& transform2->GetLocalPosition().y + transform2->GetLocalScale().y >= transform1->GetLocalPosition().y;
	return colX && colY;
}

//working LERP
glm::vec3 LERP(const Transform::sptr& start, const Transform::sptr& end, float t)
{
	return start->GetLocalPosition() * (1 - t) + end->GetLocalPosition() * t;
}

//hopefully this will work
glm::vec3 Catmull(const Transform::sptr& p0, const Transform::sptr& p1, const Transform::sptr& p2, const Transform::sptr& p3, float t)
{
	return 0.5f *(2.0f * p1->GetLocalPosition() + t * (-p0->GetLocalPosition() + p2->GetLocalPosition())
		+ t * t * (2.0f * p0->GetLocalPosition() - 5.0f * p1->GetLocalPosition() + 4.0f * p2->GetLocalPosition() - p3->GetLocalPosition())
		+ t * t * t * (-p0->GetLocalPosition() + 3.0f * p1->GetLocalPosition() - 3.0f * p2->GetLocalPosition() + p3->GetLocalPosition()));
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
	VertexArrayObject::sptr vaoplayer = ObjLoader::LoadFromFile("models/Dunce.obj");//player 1
	VertexArrayObject::sptr vaobackground = ObjLoader::LoadFromFile("models/Ground.obj");//Ground
	VertexArrayObject::sptr vaomonkeybar = ObjLoader::LoadFromFile("models/MonkeyBar.obj");//MonkeyBars
	VertexArrayObject::sptr vaosandbox = ObjLoader::LoadFromFile("models/SandBox.obj");//SandBox
	VertexArrayObject::sptr vaoslide = ObjLoader::LoadFromFile("models/Slide.obj");//Slide
	VertexArrayObject::sptr vaoround = ObjLoader::LoadFromFile("models/RA.obj");//roundabout
	VertexArrayObject::sptr vaobottle = ObjLoader::LoadFromFile("models/waterBottle.obj");//Waterbottle
	VertexArrayObject::sptr vaoswing = ObjLoader::LoadFromFile("models/Swing.obj");//swing
	VertexArrayObject::sptr vaoplayer2 = ObjLoader::LoadFromFile("models/Duncet.obj");//Player 2
	VertexArrayObject::sptr vaotable = ObjLoader::LoadFromFile("models/Table.obj");//table
	VertexArrayObject::sptr vaoballoon = ObjLoader::LoadFromFile("models/Balloon.obj");//balloon
	VertexArrayObject::sptr vaoballoonicon = ObjLoader::LoadFromFile("models/BalloonIcon.obj");//balloonicon
	VertexArrayObject::sptr vaobench = ObjLoader::LoadFromFile("models/Bench.obj");//bench
	VertexArrayObject::sptr vaoduncetwin = ObjLoader::LoadFromFile("models/DuncetWin.obj");//duncetwin
	VertexArrayObject::sptr vaoduncewin = ObjLoader::LoadFromFile("models/DunceWin.obj");//duncewin
	VertexArrayObject::sptr vaoflower = ObjLoader::LoadFromFile("models/Flower1.obj");//flower
	VertexArrayObject::sptr vaograss1 = ObjLoader::LoadFromFile("models/Grass1.obj");//grass1
	VertexArrayObject::sptr vaograss2 = ObjLoader::LoadFromFile("models/Grass2.obj");//grass2
	VertexArrayObject::sptr vaohedge = ObjLoader::LoadFromFile("models/Hedge.obj");//Hedge
	VertexArrayObject::sptr vaopinwheel = ObjLoader::LoadFromFile("models/Pinwheel.fbx");//pinwheel
	VertexArrayObject::sptr vaoscore = ObjLoader::LoadFromFile("models/Score.obj");//score
	VertexArrayObject::sptr vaosliceofcake = ObjLoader::LoadFromFile("models/SliceofCake.obj");//Sliceofcake
	VertexArrayObject::sptr vaobigtree = ObjLoader::LoadFromFile("models/TreeBig.obj");//bigtree
	VertexArrayObject::sptr vaosmalltree = ObjLoader::LoadFromFile("models/TreeSmall.obj");//smalltree
	VertexArrayObject::sptr vaoHitbox = ObjLoader::LoadFromFile("models/HitBox.obj");//Hitbox
		
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

	// Create some transforms and initialize them
	Transform::sptr transforms[38];
	for (int x = 0; x < 38; x++)
	{
		transforms[x] = Transform::Create();
	}

	// We can use operator chaining, since our Set* methods return a pointer to the instance, neat!
	transforms[0]->SetLocalPosition(-30.0f, -20.0f, 1.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(2.5f, 2.5f, 2.5f);//Player1
	transforms[1]->SetLocalPosition(30.0f, -20.0f, 1.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(2.5f, 2.5f, 2.5f);//Player2
	transforms[2]->SetLocalPosition(0.0f, 0.0f, -5.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Background
	transforms[3]->SetLocalPosition(0.0f, 0.0f, -3.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//Sandbox
	transforms[4]->SetLocalPosition(-8.0f, 8.0f, 5.0f)->SetLocalRotation(90.0f, 0.0f, 270.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//slide
	transforms[5]->SetLocalPosition(12.0f, -7.0f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 180.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//swing
	transforms[6]->SetLocalPosition(0.0f, 5.0f, -3.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//roundabout
	transforms[7]->SetLocalPosition(12.0f, 8.0f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 90.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//monkeybar
	transforms[8]->SetLocalPosition(1.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//balloon
	transforms[9]->SetLocalPosition(3.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//table
	transforms[10]->SetLocalPosition(5.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//bench
	transforms[11]->SetLocalPosition(7.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//flower
	transforms[12]->SetLocalPosition(9.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//grass1
	transforms[13]->SetLocalPosition(11.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//grass2
	transforms[14]->SetLocalPosition(13.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//hedge
	transforms[15]->SetLocalPosition(15.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//pinwheel
	transforms[16]->SetLocalPosition(17.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//score
	transforms[17]->SetLocalPosition(19.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//slice
	transforms[18]->SetLocalPosition(21.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//bigtree
	transforms[19]->SetLocalPosition(23.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//smalltree
	transforms[20]->SetLocalPosition(25.0f, 3.5f, 0.0f)->SetLocalRotation(90.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//balloonicon
	transforms[21]->SetLocalPosition(0.0f, -20.0f, 2.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(3.0f, 3.0f, 3.0f);//BOTTLE 1 Bot Mid Sandbox
	transforms[22]->SetLocalPosition(25.0f, 10.0f, 1.0f)->SetLocalRotation(0.0f, 90.0f, 90.0f)->SetLocalScale(3.0f, 3.0f, 3.0f);//BOTTLE 2 Top Right Field
	transforms[23]->SetLocalPosition(0.0f, 0.0f, 2.0f)->SetLocalRotation(0.0f, 0.0f, 125.0f)->SetLocalScale(3.0f, 3.0f, 3.0f);//BOTTLE 3 Mid Sandbox
	transforms[24]->SetLocalPosition(-25.0f, 15.0f, 1.0f)->SetLocalRotation(0.0f, 180.0f, 45.0f)->SetLocalScale(3.0f, 3.0f, 3.0f);//BOTTLE 4 Top Left Field
	transforms[25]->SetLocalPosition(-30.0f, -20.0f, 1.0f)->SetLocalRotation(90.0f, 0.0f, 135.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//bullethitboxplayer1
	transforms[26]->SetLocalPosition(30.0f, -20.0f, 1.0f)->SetLocalRotation(90.0f, 0.0f, 225.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//bullethitboxplayer2
	transforms[27]->SetLocalPosition(-35.0f, -30.0f, 1.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 70.0f, 1.0f);//hitbox left wall
	transforms[28]->SetLocalPosition(35.0f, -30.0f, 1.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 70.0f, 1.0f);//hitbox right wall
	transforms[29]->SetLocalPosition(-35.0f, -30.0f, 1.0f)->SetLocalRotation(1.0f, 1.0f, 0.0f)->SetLocalScale(70.0f, 1.0f, 1.0f);//hitbox bottom wall
	transforms[30]->SetLocalPosition(-35.0f, 30.0f, 1.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(70.0f, 1.0f, 1.0f);//hitbox top wall
	transforms[31]->SetLocalPosition(0.0f, 0.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test hitbox
	transforms[32]->SetLocalPosition(0.0f, 0.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test start
	transforms[33]->SetLocalPosition(10.0f, 0.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test end
	transforms[34]->SetLocalPosition(20.0f, 20.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test catmull start
	transforms[35]->SetLocalPosition(15.0f, 0.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test catmull mid1
	transforms[36]->SetLocalPosition(-5.0f, -10.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test catmull mid2
	transforms[37]->SetLocalPosition(-5.0f, 0.0f, 0.0f)->SetLocalRotation(0.0f, 0.0f, 0.0f)->SetLocalScale(1.0f, 1.0f, 1.0f);//test catmull end

	// We'll store all our VAOs into a nice array for easy access
	VertexArrayObject::sptr vaos[21];
	vaos[0] = vaoplayer;
	vaos[1] = vaoplayer2;
	vaos[2] = vaobackground;
	vaos[3] = vaosandbox;
	vaos[4] = vaoslide;
	vaos[5] = vaoswing;
	vaos[6] = vaoround;
	vaos[7] = vaomonkeybar;
	vaos[8] = vaoballoon;
	vaos[9] = vaotable;
	vaos[10] = vaobench;
	vaos[11] = vaoflower;
	vaos[12] = vaograss1;
	vaos[13] = vaograss2;
	vaos[14] = vaohedge;
	vaos[15] = vaopinwheel;
	vaos[16] = vaoscore;
	vaos[17] = vaosliceofcake;
	vaos[18] = vaobigtree;
	vaos[19] = vaosmalltree;
	vaos[20] = vaoballoonicon;

	//need to somehow make this thing make multiple textures(hard for some reason)
	// Load our texture data from a file
	Texture2DData::sptr diffuseMapGround = Texture2DData::LoadFromFile("images/Ground.png");
	Texture2DData::sptr diffuseMap2 = Texture2DData::LoadFromFile("images/box.bmp");//test map/default second texture on objects
	Texture2DData::sptr diffuseMapSandBox = Texture2DData::LoadFromFile("images/SandBox.png");
	Texture2DData::sptr diffuseMapSlide = Texture2DData::LoadFromFile("images/Slide.png");
	Texture2DData::sptr diffuseBottleMap = Texture2DData::LoadFromFile("images/BottleTex.png");
	Texture2DData::sptr diffuseMapMonkeyBar = Texture2DData::LoadFromFile("images/MonkeyBar.png");
	Texture2DData::sptr diffuseMapRA = Texture2DData::LoadFromFile("images/RoundAbout.png");
	Texture2DData::sptr diffuseMapSwing = Texture2DData::LoadFromFile("images/Swing.png");
	Texture2DData::sptr diffuseMapBlueBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonBlue.png");
	Texture2DData::sptr diffuseMapDarkBlueBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonDarkBlue.png");
	Texture2DData::sptr diffuseMapGreenBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonGreen.png");
	Texture2DData::sptr diffuseMapLightBlueBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonLightBlue.png");
	Texture2DData::sptr diffuseMapLimeBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonLime.png");
	Texture2DData::sptr diffuseMapOrangeBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonOrange.png");
	Texture2DData::sptr diffuseMapPinkBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonPink.png");
	Texture2DData::sptr diffuseMapPurpleBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonPurple.png");
	Texture2DData::sptr diffuseMapRedBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonRed.png");
	Texture2DData::sptr diffuseMapYellowBalloon = Texture2DData::LoadFromFile("images/Balloon/BalloonYellow.png");
	Texture2DData::sptr diffuseMapTable = Texture2DData::LoadFromFile("images/Table.png");
	Texture2DData::sptr diffuseMapBench = Texture2DData::LoadFromFile("images/BenchNew.png");
	Texture2DData::sptr diffuseMapFlower = Texture2DData::LoadFromFile("images/Flower1.png");
	Texture2DData::sptr diffuseMapGrass = Texture2DData::LoadFromFile("images/Grass.png");
	Texture2DData::sptr diffuseMapGrass2 = Texture2DData::LoadFromFile("images/Grass2.png");
	Texture2DData::sptr diffuseMapHedge = Texture2DData::LoadFromFile("images/Hedge.png");
	Texture2DData::sptr diffuseMapSliceofcake = Texture2DData::LoadFromFile("images/Slice of Cake.png");
	Texture2DData::sptr diffuseMapTreeBig = Texture2DData::LoadFromFile("images/TreeBig.png");
	Texture2DData::sptr diffuseMapTreeSmall = Texture2DData::LoadFromFile("images/TreeSmall.png");
	Texture2DData::sptr diffuseMapDunce = Texture2DData::LoadFromFile("images/Dunce.png");
	Texture2DData::sptr diffuseMapDuncet = Texture2DData::LoadFromFile("images/Duncet.png");
	Texture2DData::sptr diffuseMapYellowBalloonicon = Texture2DData::LoadFromFile("images/Balloon Icon/BalloonIconDunce.png");
	Texture2DData::sptr diffuseMapPinkBalloonicon = Texture2DData::LoadFromFile("images/Balloon Icon/BalloonIconDuncet.png");
	Texture2DData::sptr diffuseMapPinwheel = Texture2DData::LoadFromFile("images/PinWheel/PinWheel.png");
	Texture2DData::sptr diffuseMapPinwheelblackandwhite = Texture2DData::LoadFromFile("images/PinWheel/PinWheelBlackandWhite.png");
	Texture2DData::sptr diffuseMapPinwheelblue = Texture2DData::LoadFromFile("images/PinWheel/PinWheelBlue.png");
	Texture2DData::sptr diffuseMapPinwheellightblue = Texture2DData::LoadFromFile("images/PinWheel/PinWheelLightBlue.png");
	Texture2DData::sptr diffuseMapPinwheelpink = Texture2DData::LoadFromFile("images/PinWheel/PinWheelPink.png");
	Texture2DData::sptr diffuseMapPinwheelred = Texture2DData::LoadFromFile("images/PinWheel/PinWheelRed.png");
	Texture2DData::sptr diffuseMapPinwheelyellow = Texture2DData::LoadFromFile("images/PinWheel/PinWheelYellow.png");
	Texture2DData::sptr specularMap = Texture2DData::LoadFromFile("images/Stone_001_Specular.png");

	// Create a texture from the data
	//Ground texture
	Texture2D::sptr diffuseGround = Texture2D::Create();
	diffuseGround->LoadData(diffuseMapGround);
	
	//Player1 texture
	Texture2D::sptr diffuseDunce = Texture2D::Create();
	diffuseDunce->LoadData(diffuseMapDunce);
	
	//Player2 texture
	Texture2D::sptr diffuseDuncet = Texture2D::Create();
	diffuseDuncet->LoadData(diffuseMapDuncet);

	Texture2D::sptr specular = Texture2D::Create();
	specular->LoadData(specularMap);

	//testing texture
	Texture2D::sptr diffuse2 = Texture2D::Create();
	diffuse2->LoadData(diffuseMap2);

	//Slide texture
	Texture2D::sptr diffuseSlide = Texture2D::Create();
	diffuseSlide->LoadData(diffuseMapSlide);
	
	//SandBox texture
	Texture2D::sptr diffuseSandBox = Texture2D::Create();
	diffuseSandBox->LoadData(diffuseMapSandBox);

	//bottle texture
	Texture2D::sptr diffuseBottle = Texture2D::Create();
	diffuseBottle->LoadData(diffuseBottleMap);

	//MonkeyBar texture
	Texture2D::sptr diffuseMonkeyBar = Texture2D::Create();
	diffuseMonkeyBar->LoadData(diffuseMapMonkeyBar);
	
	//RoundAbout texture
	Texture2D::sptr diffuseRA = Texture2D::Create();
	diffuseRA->LoadData(diffuseMapRA);
	
	//Swing texture
	Texture2D::sptr diffuseSwing = Texture2D::Create();
	diffuseSwing->LoadData(diffuseMapSwing);

	//bench texture
	Texture2D::sptr diffuseBench = Texture2D::Create();
	diffuseBench->LoadData(diffuseMapBench);
	
	//Table texture
	Texture2D::sptr diffuseTable = Texture2D::Create();
	diffuseTable->LoadData(diffuseMapTable);
	
	//Flower texture
	Texture2D::sptr diffuseFlower = Texture2D::Create();
	diffuseFlower->LoadData(diffuseMapFlower);
	
	//grass1 texture
	Texture2D::sptr diffusegrass = Texture2D::Create();
	diffusegrass->LoadData(diffuseMapGrass);
	
	//grass2 texture
	Texture2D::sptr diffusegrass2 = Texture2D::Create();
	diffusegrass2->LoadData(diffuseMapGrass2);
	
	//hedge texture
	Texture2D::sptr diffusehedge = Texture2D::Create();
	diffusehedge->LoadData(diffuseMapHedge);
	
	//sliceofcake texture
	Texture2D::sptr diffuseslice = Texture2D::Create();
	diffuseslice->LoadData(diffuseMapSliceofcake);
	
	//treebig texture
	Texture2D::sptr diffusebigtree = Texture2D::Create();
	diffusebigtree->LoadData(diffuseMapTreeBig);
	
	//treesmall texture
	Texture2D::sptr diffusesmalltree = Texture2D::Create();
	diffusesmalltree->LoadData(diffuseMapTreeSmall);
	
	//Blueballoon texture
	Texture2D::sptr diffuseBlueBalloon = Texture2D::Create();
	diffuseBlueBalloon->LoadData(diffuseMapBlueBalloon);
	
	//DarkBlueballoon texture
	Texture2D::sptr diffuseDarkBlueBalloon = Texture2D::Create();
	diffuseDarkBlueBalloon->LoadData(diffuseMapDarkBlueBalloon);
	
	//LightBlueballoon texture
	Texture2D::sptr diffuseLightBlueBalloon = Texture2D::Create();
	diffuseLightBlueBalloon->LoadData(diffuseMapLightBlueBalloon);
	
	//Greenballoon texture
	Texture2D::sptr diffuseGreenBalloon = Texture2D::Create();
	diffuseGreenBalloon->LoadData(diffuseMapGreenBalloon);
	
	//Limeballoon texture
	Texture2D::sptr diffuseLimeBalloon = Texture2D::Create();
	diffuseLimeBalloon->LoadData(diffuseMapLimeBalloon);
	
	//Orangeballoon texture
	Texture2D::sptr diffuseOrangeBalloon = Texture2D::Create();
	diffuseOrangeBalloon->LoadData(diffuseMapOrangeBalloon);
	
	//Pinkballoon texture
	Texture2D::sptr diffusePinkBalloon = Texture2D::Create();
	diffusePinkBalloon->LoadData(diffuseMapPinkBalloon);
	
	//Purpleballoon texture
	Texture2D::sptr diffusePurpleBalloon = Texture2D::Create();
	diffusePurpleBalloon->LoadData(diffuseMapPurpleBalloon);
	
	//Redballoon texture
	Texture2D::sptr diffuseRedBalloon = Texture2D::Create();
	diffuseRedBalloon->LoadData(diffuseMapRedBalloon);
	
	//Yellowballoon texture
	Texture2D::sptr diffuseYellowBalloon = Texture2D::Create();
	diffuseYellowBalloon->LoadData(diffuseMapYellowBalloon);
	
	//Ballooniconbunce texture
	Texture2D::sptr diffuseDunceballoon = Texture2D::Create();
	diffuseDunceballoon->LoadData(diffuseMapYellowBalloonicon);
	
	//Ballooniconbuncet texture
	Texture2D::sptr diffuseDuncetballoon = Texture2D::Create();
	diffuseDuncetballoon->LoadData(diffuseMapPinkBalloonicon);
	
	//PinWheelrainbow texture
	Texture2D::sptr diffusePinwheel = Texture2D::Create();
	diffusePinwheel->LoadData(diffuseMapPinwheel);
	
	//Pinwheelblackandwhite texture
	Texture2D::sptr diffusePinwheelblackandwhite = Texture2D::Create();
	diffusePinwheelblackandwhite->LoadData(diffuseMapPinwheelblackandwhite);
	
	//PinwheelBlue texture
	Texture2D::sptr diffusePinwheelBlue = Texture2D::Create();
	diffusePinwheelBlue->LoadData(diffuseMapPinwheelblue);
	
	//PinwheelLightblue texture
	Texture2D::sptr diffusePinwheellightblue = Texture2D::Create();
	diffusePinwheellightblue->LoadData(diffuseMapPinwheellightblue);
	
	//PinwheelPink texture
	Texture2D::sptr diffusePinwheelpink = Texture2D::Create();
	diffusePinwheelpink->LoadData(diffuseMapPinwheelpink);
	
	//Pinwheelred texture
	Texture2D::sptr diffusePinwheelred = Texture2D::Create();
	diffusePinwheelred->LoadData(diffuseMapPinwheelred);
	
	//Pinwheelyellow texture
	Texture2D::sptr diffusePinwheelyellow = Texture2D::Create();
	diffusePinwheelyellow->LoadData(diffuseMapPinwheelyellow);

	// Creating an empty texture
	Texture2DDescription desc = Texture2DDescription();
	desc.Width = 1;
	desc.Height = 1;
	desc.Format = InternalFormat::RGB8;
	Texture2D::sptr texture2 = Texture2D::Create(desc);
	texture2->Clear(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// TODO: store some info about our materials for each object
	Material materials[22];
	//player1
	materials[0].Albedo = diffuseDunce;
	materials[0].Albedo2 = diffuse2;
	materials[0].Specular = specular;
	materials[0].Shininess = 4.0f;
	materials[0].TextureMix = 0.0f;
	
	//player2
	materials[1].Albedo = diffuseDuncet;
	materials[1].Albedo2 = diffuse2;
	materials[1].Specular = specular;
	materials[1].Shininess = 4.0f;
	materials[1].TextureMix = 0.0f;

	//Background
	materials[2].Albedo = diffuseGround;
	materials[2].Albedo2 = diffuse2;
	materials[2].Specular = specular;
	materials[2].Shininess = 16.0f;
	materials[2].TextureMix = 0.0f;

	//Sandbox
	materials[3].Albedo = diffuseSandBox;
	materials[3].Albedo2 = diffuse2;
	materials[3].Specular = specular;
	materials[3].Shininess = 32.0f;
	materials[3].TextureMix = 0.0f;
	
	//slide
	materials[4].Albedo = diffuseSlide;
	materials[4].Albedo2 = diffuse2;
	materials[4].Specular = specular;
	materials[4].Shininess = 32.0f;
	materials[4].TextureMix = 0.0f;

	//Swing
	materials[5].Albedo = diffuseSwing;
	materials[5].Albedo2 = diffuse2;
	materials[5].Specular = specular;
	materials[5].Shininess = 32.0f;
	materials[5].TextureMix = 0.0f;
	
	//RoundAbout
	materials[6].Albedo = diffuseRA;
	materials[6].Albedo2 = diffuse2;
	materials[6].Specular = specular;
	materials[6].Shininess = 32.0f;
	materials[6].TextureMix = 0.0f;

	//Monkeybar
	materials[7].Albedo = diffuseMonkeyBar;
	materials[7].Albedo2 = diffuse2;
	materials[7].Specular = specular;
	materials[7].Shininess = 32.0f;
	materials[7].TextureMix = 0.0f;
	
	//blueballoon
	materials[8].Albedo = diffuseBlueBalloon;
	materials[8].Albedo2 = diffuse2;
	materials[8].Specular = specular;
	materials[8].Shininess = 32.0f;
	materials[8].TextureMix = 0.0f;
	
	//table
	materials[9].Albedo = diffuseTable;
	materials[9].Albedo2 = diffuse2;
	materials[9].Specular = specular;
	materials[9].Shininess = 32.0f;
	materials[9].TextureMix = 0.0f;

	//bench
	materials[10].Albedo = diffuseBench;
	materials[10].Albedo2 = diffuse2;
	materials[10].Specular = specular;
	materials[10].Shininess = 32.0f;
	materials[10].TextureMix = 0.0f;

	//flower
	materials[11].Albedo = diffuseFlower;
	materials[11].Albedo2 = diffuse2;
	materials[11].Specular = specular;
	materials[11].Shininess = 32.0f;
	materials[11].TextureMix = 0.0f;

	//grass1
	materials[12].Albedo = diffusegrass;
	materials[12].Albedo2 = diffuse2;
	materials[12].Specular = specular;
	materials[12].Shininess = 32.0f;
	materials[12].TextureMix = 0.0f;

	//grass2
	materials[13].Albedo = diffusegrass2;
	materials[13].Albedo2 = diffuse2;
	materials[13].Specular = specular;
	materials[13].Shininess = 32.0f;
	materials[13].TextureMix = 0.0f;

	//hedge
	materials[14].Albedo = diffusehedge;
	materials[14].Albedo2 = diffuse2;
	materials[14].Specular = specular;
	materials[14].Shininess = 32.0f;
	materials[14].TextureMix = 0.0f;

	//pinwheel
	materials[15].Albedo = diffusePinwheel;
	materials[15].Albedo2 = diffuse2;
	materials[15].Specular = specular;
	materials[15].Shininess = 32.0f;
	materials[15].TextureMix = 0.0f;

	//Score
	materials[16].Albedo = diffuseGround;
	materials[16].Albedo2 = diffuse2;
	materials[16].Specular = specular;
	materials[16].Shininess = 32.0f;
	materials[16].TextureMix = 0.0f;

	//sliceofcake
	materials[17].Albedo = diffuseslice;
	materials[17].Albedo2 = diffuse2;
	materials[17].Specular = specular;
	materials[17].Shininess = 32.0f;
	materials[17].TextureMix = 0.0f;

	//bigtree
	materials[18].Albedo = diffusebigtree;
	materials[18].Albedo2 = diffuse2;
	materials[18].Specular = specular;
	materials[18].Shininess = 32.0f;
	materials[18].TextureMix = 0.0f;

	//smalltree
	materials[19].Albedo = diffusesmalltree;
	materials[19].Albedo2 = diffuse2;
	materials[19].Specular = specular;
	materials[19].Shininess = 32.0f;
	materials[19].TextureMix = 0.0f;

	//balloonicon
	materials[20].Albedo = diffuseGround;
	materials[20].Albedo2 = diffuse2;
	materials[20].Specular = specular;
	materials[20].Shininess = 32.0f;
	materials[20].TextureMix = 0.0f;
	
	//Bottle
	materials[21].Albedo = diffuseBottle;
	materials[21].Albedo2 = diffuse2;
	materials[21].Specular = specular;
	materials[21].Shininess = 32.0f;
	materials[21].TextureMix = 0.0f;

	//Camera
	camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, -3, 40)); // Set initial position
	camera->SetUp(glm::vec3(0, 0, 1)); // Use a z-up coordinate system
	camera->LookAt(glm::vec3(0.0f)); // Look at center of the screen
	camera->SetFovDegrees(90.0f); // Set an initial FOV
	camera->SetOrthoHeight(3.0f);
		
	// Our high-precision timer
	double lastFrame = glfwGetTime();

	//checks if the player shot the bullet or not
	bool shoot = false;
	bool shoot2 = false;
	//checks if the player has ammo or not
	bool ammo = true;
	bool ammo2 = true;

	//LERP stuff
	float tLERP = 0.0f;
	float tlimitLERP = 1.0f;
	bool forward = true;

	//Catmull stuff
	float tCatmull = 0.0f;
	float segmenttime = 1.0f;
	int segmentindex = 0.0f;
	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);
		//keeps track of time in our game
		tLERP += dt;

		//LERP
		//makes LERP switch
		if (tLERP >= tlimitLERP)
		{
			tLERP = 0.0f;
			forward = !forward;
		}

		//checks if the lerp should go backwards or forwards
		if (forward) {
			transforms[8]->SetLocalPosition(LERP(transforms[32], transforms[33], tLERP));
		}
		else
		{
			transforms[8]->SetLocalPosition(LERP(transforms[33], transforms[32], tLERP));
		}
		//////////////////////////////

		tCatmull += dt;

		while (tCatmull > segmenttime)
		{
			tCatmull -= segmenttime;
		}

		float t = tCatmull / segmenttime;

		transforms[31]->SetLocalPosition(Catmull(transforms[34], transforms[35], transforms[36], transforms[37], t));

		ManipulateTransformWithInput(transforms[0], transforms[1], dt);

		//colour of the background
		glClearColor(0.08f, 0.17f, 0.31f, 1.0f);//rgba
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->Bind();
		// These are the uniforms that update only once per frame
		shader->SetUniformMatrix("u_View", camera->GetView());
		shader->SetUniform("u_CamPos", camera->GetPosition());

		// Tell OpenGL that slot 0 will hold the diffuse, and slot 1 will hold the specular
		shader->SetUniform("s_Diffuse", 0);
		shader->SetUniform("s_Diffuse2", 1);
		shader->SetUniform("s_Specular", 2);

		// Render all VAOs in our scene
		for (int ix = 0; ix < 20; ix++) {
			// TODO: Apply materials
			materials[ix].Albedo->Bind(0);
			materials[ix].Albedo2->Bind(1);
			materials[ix].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[ix].Shininess);
			shader->SetUniform("u_TextureMix", materials[ix].TextureMix);
			RenderVAO(shader, vaos[ix], camera, transforms[ix]);
		}
		//bottle 1
		materials[21].Albedo->Bind(0);
		materials[21].Albedo2->Bind(1);
		materials[21].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[21].Shininess);
		shader->SetUniform("u_TextureMix", materials[21].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[21]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[21]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[21]->LocalTransform()));
		vaobottle->Render();

		//bottle 2
		materials[21].Albedo->Bind(0);
		materials[21].Albedo2->Bind(1);
		materials[21].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[21].Shininess);
		shader->SetUniform("u_TextureMix", materials[21].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[22]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[22]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[22]->LocalTransform()));
		vaobottle->Render();

		//bottle 3
		materials[21].Albedo->Bind(0);
		materials[21].Albedo2->Bind(1);
		materials[21].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[21].Shininess);
		shader->SetUniform("u_TextureMix", materials[21].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[23]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[23]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[23]->LocalTransform()));
		vaobottle->Render();

		//bottle 4
		materials[21].Albedo->Bind(0);
		materials[21].Albedo2->Bind(1);
		materials[21].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[21].Shininess);
		shader->SetUniform("u_TextureMix", materials[21].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[24]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[24]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[24]->LocalTransform()));
		vaobottle->Render();

		//Hitboxleft wall
		materials[1].Albedo->Bind(0);
		materials[1].Albedo2->Bind(1);
		materials[1].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[1].Shininess);
		shader->SetUniform("u_TextureMix", materials[1].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[27]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[27]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[27]->LocalTransform()));
		//vaoHitbox->Render();
		
		//Hitboxright wall
		materials[1].Albedo->Bind(0);
		materials[1].Albedo2->Bind(1);
		materials[1].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[1].Shininess);
		shader->SetUniform("u_TextureMix", materials[1].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[28]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[28]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[28]->LocalTransform()));
		//vaoHitbox->Render();
		
		//Hitboxbottom wall
		materials[1].Albedo->Bind(0);
		materials[1].Albedo2->Bind(1);
		materials[1].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[1].Shininess);
		shader->SetUniform("u_TextureMix", materials[1].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[29]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[29]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[29]->LocalTransform()));
		//vaoHitbox->Render();
		
		//Hitboxtop wall
		materials[1].Albedo->Bind(0);
		materials[1].Albedo2->Bind(1);
		materials[1].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[1].Shininess);
		shader->SetUniform("u_TextureMix", materials[1].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[30]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[30]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[30]->LocalTransform()));
		//vaoHitbox->Render();	
		
		//catmull tester
		materials[1].Albedo->Bind(0);
		materials[1].Albedo2->Bind(1);
		materials[1].Specular->Bind(2);
		shader->SetUniform("u_Shininess", materials[1].Shininess);
		shader->SetUniform("u_TextureMix", materials[1].TextureMix);
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[31]->LocalTransform());
		shader->SetUniformMatrix("u_Model", transforms[31]->LocalTransform());
		shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[31]->LocalTransform()));
		vaoHitbox->Render();	
		
		// working getto shooting
		//player1 shooting
		if (ammo) {
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || shoot) {
				transforms[25]->MoveLocal(0.0f, 0.0f, -18.0f * dt);
				shoot = true;
			}
			else
			{
				transforms[25]->SetLocalPosition(transforms[0]->GetLocalPosition());
				transforms[25]->SetLocalRotation(transforms[0]->GetLocalRotation());
				shoot = false;
			}
		}
		
		if (Collision(transforms[27], transforms[25]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			transforms[25]->SetLocalPosition(transforms[0]->GetLocalPosition());
			transforms[25]->SetLocalRotation(transforms[0]->GetLocalRotation());
			shoot = false;
			ammo = false;
		}
		
		if (Collision(transforms[28], transforms[25]))
		{
			//std::cout << "yes right" << std::endl;//checking if hit boxes are working
			transforms[25]->SetLocalPosition(transforms[0]->GetLocalPosition());
			transforms[25]->SetLocalRotation(transforms[0]->GetLocalRotation());
			shoot = false;
			ammo = false;
		}
		
		if (Collision(transforms[29], transforms[25]))
		{
			//std::cout << "yes bottom" << std::endl;//checking if hit boxes are working
			transforms[25]->SetLocalPosition(transforms[0]->GetLocalPosition());
			transforms[25]->SetLocalRotation(transforms[0]->GetLocalRotation());
			shoot = false;
			ammo = false;
		}
		
		if (Collision(transforms[30], transforms[25]))
		{
			//std::cout << "yes top" << std::endl;//checking if hit boxes are working
			transforms[25]->SetLocalPosition(transforms[0]->GetLocalPosition());
			transforms[25]->SetLocalRotation(transforms[0]->GetLocalRotation());
			shoot = false;
			ammo = false;
		}

		if (Collision(transforms[0], transforms[31]))
		{
			ammo = true;
		}
		if (shoot) {
			//Hitboxplayer 1 shoot
			materials[1].Albedo->Bind(0);
			materials[1].Albedo2->Bind(1);
			materials[1].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[1].Shininess);
			shader->SetUniform("u_TextureMix", materials[1].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transforms[25]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[25]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[25]->LocalTransform()));
			vaoHitbox->Render();
		}

		//player 1 hitboxes
		if (Collision(transforms[0], transforms[27]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}	
		
		if (Collision(transforms[0], transforms[26]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}	
		
		if (Collision(transforms[0], transforms[27]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}	
		
		if (Collision(transforms[0], transforms[28]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}

		//player 2 shooting
		if (ammo2) {
			if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS || shoot2) {
				transforms[26]->MoveLocal(0.0f, 0.0f, -18.0f * dt);
				shoot2 = true;
			}
			else
			{
				transforms[26]->SetLocalPosition(transforms[1]->GetLocalPosition());
				transforms[26]->SetLocalRotation(transforms[1]->GetLocalRotation());
				shoot2 = false;
			}
		}

		//player left collison
		if (Collision(transforms[27], transforms[26]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			transforms[26]->SetLocalPosition(transforms[1]->GetLocalPosition());
			transforms[26]->SetLocalRotation(transforms[1]->GetLocalRotation());
			shoot2 = false;
			ammo2 = false;
		}

		//player right collison
		if (Collision(transforms[28], transforms[26]))
		{
			//std::cout << "yes right" << std::endl;//checking if hit boxes are working
			transforms[26]->SetLocalPosition(transforms[1]->GetLocalPosition());
			transforms[26]->SetLocalRotation(transforms[1]->GetLocalRotation());
			shoot2 = false;
			ammo2 = false;
		}

		//player bottom collison
		if (Collision(transforms[29], transforms[26]))
		{
			//std::cout << "yes bottom" << std::endl;//checking if hit boxes are working
			transforms[26]->SetLocalPosition(transforms[1]->GetLocalPosition());
			transforms[26]->SetLocalRotation(transforms[1]->GetLocalRotation());
			shoot2 = false;
			ammo2 = false;
		}

		//player top collison
		if (Collision(transforms[30], transforms[26]))
		{
			//std::cout << "yes top" << std::endl;//checking if hit boxes are working
			transforms[26]->SetLocalPosition(transforms[1]->GetLocalPosition());
			transforms[26]->SetLocalRotation(transforms[1]->GetLocalRotation());
			shoot2 = false;
			ammo2 = false;
		}

		if (Collision(transforms[1], transforms[31]))
		{
			ammo2 = true;
		}

		if (shoot2) {
			//Hitboxplayer 2 shoot
			materials[1].Albedo->Bind(0);
			materials[1].Albedo2->Bind(1);
			materials[1].Specular->Bind(2);
			shader->SetUniform("u_Shininess", materials[1].Shininess);
			shader->SetUniform("u_TextureMix", materials[1].TextureMix);
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* transforms[26]->LocalTransform());
			shader->SetUniformMatrix("u_Model", transforms[26]->LocalTransform());
			shader->SetUniformMatrix("u_ModelRotation", glm::mat3(transforms[26]->LocalTransform()));
			vaoHitbox->Render();
		}

		//player2 left collison
		if (Collision(transforms[1], transforms[27]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}

		//player2 right collison
		if (Collision(transforms[1], transforms[28]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}

		//player2 bottom collison
		if (Collision(transforms[1], transforms[29]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}

		//player2 top collison
		if (Collision(transforms[1], transforms[30]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}
		
		//player2 player1 collison
		if (Collision(transforms[1], transforms[0]))
		{
			//std::cout << "yes" << std::endl;//checking if hit boxes are working
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
				transforms[1]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, -9.0f * dt);
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				transforms[0]->MoveLocal(0.0f, 0.0f, 9.0f * dt);
			}
		}
		glfwSwapBuffers(window);
		lastFrame = thisFrame;
	}
	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}