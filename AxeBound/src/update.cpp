// update.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//


// --- Includes
#include <SFML/Graphics.hpp>
#include "cmath"

#include "update.h"
#include "globals.h"
#include "inputManager.h"
#include "gameSystem.h"

// --- Private/Local Declerations

sf::Vector2f capMovementOffset(sf::Vector2f offset);


// CHANGED: squareSpeed / squareOffset (the temporary square) were removed. maxSpeed is the one speed value now
// (this answers the old "why is there 3 speed values" question) and the movement is handed to the game scene.

// --- Global/Declared Methods

void update(float dt)
{
	updateUserInputs();
	
	sf::Vector2f offset(0.0f, 0.0f);
	bool sprint = false;
	
	button currentInput;
	do
	{
		currentInput = getUserInput();
		if (currentInput.action == "Move Up"){
			offset.y -= maxSpeed * currentInput.strength;
		}
		else if (currentInput.action == "Move Down"){
			offset.y += maxSpeed * currentInput.strength;
		}
		else if (currentInput.action == "Move Left"){
			offset.x -= maxSpeed * currentInput.strength; // CHANGED: was += (left moved right)
		}
		else if (currentInput.action == "Move Right"){
			offset.x += maxSpeed * currentInput.strength; // CHANGED: was -= (right moved left)
		}
		else if (currentInput.action == "Sprint"){
			sprint = true;
		}
	} while (!(currentInput.action == "END"));
	frame.move = capMovementOffset(offset); // pixels per second, capped so diagonals are not faster
	frame.sprint = sprint;
	
	GameSystem::get().update(dt); // the active scene (menu or game) does the rest
}

// --- Private/Local Methods


// takes in the movement vector and caps it to max speed, needs to have a sprint modifier though?
sf::Vector2f capMovementOffset(sf::Vector2f offset)
{
	//sf::Vector2f cappedOffset = offset;
	float offsetSq = offset.x * offset.x + offset.y * offset.y;
	if (offsetSq > maxSpeedSq && offsetSq > 0.0f){
		float distance = std::sqrt(offsetSq);
		offset.x = (offset.x/distance) * maxSpeed;
		offset.y = (offset.y/distance) * maxSpeed;
	}
	return offset;
}
