#pragma once
#include "Gameplay/Transform.h"

class PlayerMovement {

public:

	PlayerMovement();
	~PlayerMovement();

	static void player1and2move(Transform& Player1, Transform& Player2, float dt);
	static void Player1vswall(Transform& Player1, float dt);
	static void Player2vswall(Transform& Player2, float dt);
	static void Shoot(Transform& Bullet, float dt);
};