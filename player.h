#pragma once
#include "camera.h"
#include "object3D.h"
#include "modelImporter.h"
#include "PhysXBody.h"
#include "primitveNodes.h"

class Player {
public:
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Node> playerModel;

	std::shared_ptr<PhysXBody> body;

	float health = 100.0f;
	float speed = 1.0f;

	Player() {
		//name camera
		camera = std::make_shared<Camera>("player camera");
	}


	Player(std::shared_ptr<Node> model) {
		//name camera
		camera = std::make_shared<Camera>("player camera");
		playerModel = model;


		//body = std::make_shared<PhysXBody>(playerModel, true, false);
		//body->createCapsuleGeometry(1.0f, 1.0f);
		//body->createActor();
	}	


	
	// maybe i want actually render the player model, or at least draw its shadows.
};