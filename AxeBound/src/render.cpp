// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//


// --- Includes
#include <SFML/Graphics.hpp>
#include "cmath"

#include "render.h"
#include "globals.h"
#include "gameSystem.h"

// --- Private/Local Declerations

void renderWorld(sf::RenderWindow &window);

// (tileSize moved to globals.h so every file shares it)
char renderMode = 'M'; // CHANGED: starts on the main menu
// is for renderering mods
// T = tilemap
// M = MainMenu
// etc


// --- Global/Declared Methods

void render(sf::RenderWindow &window)
{
	// main render sub
	if (renderMode == 'T'){ // CHANGED: was "renderMode = 'T'" (an assignment, so it was always true)
		renderWorld(window);              // 1. tile map + entities (world camera)
		GameSystem::get().render(window); // 2. HUD / UI on top
	}else{
		GameSystem::get().render(window); // main menu
	}
}

// --- Private/Local Methods

// the tile map render (renamed from tileMap(): that name is now the tileMap class from the design document)
void renderWorld(sf::RenderWindow &window){ // private function for tilemap renderng
	// To Do (done): tile offset - the camera follows the player, only visible tiles are drawn, the world is scaled to the window
	GameSystem::get().game().drawWorld(window);
}
