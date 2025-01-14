#pragma once
#include "camera.h"
#include "object3D.h"
#include "modelImporter.h"

class Player {
public:
	std::shared_ptr<Camera> camera;
	Node handModel;

	float health;
	
	// maybe i want actually render the player model, or at least draw its shadows.
};