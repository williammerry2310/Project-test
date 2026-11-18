// globals.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//


// --- Includes
#include <SFML/Graphics.hpp>
#include <algorithm>

#include "globals.h"
#include "logger.h"

// --- Global Declerations

const float maxSpeed = 70.0f; // CHANGED: world pixels per second (the world camera is only 480x270, 120 was too fast)
float maxSpeedSq;
sf::Texture tileTexture; // tile atlas
sf::Font gameFont;
screenLayout layout;
frameInput frame;
bool quitRequested = false;

//Log System
// CHANGED: the first argument is a mode STRING ("1" = enabled, see logger.h), passing the int 0 crashed at start up
logger gameLog(std::string("011111111"), std::string("logs/"));

// --- Methods
// works out the biggest 16:9 rectangle that fits in the window, the UI and the world are drawn inside it
void updateLayout(unsigned windowW, unsigned windowH)
{
	layout.windowW = windowW;
	layout.windowH = windowH;
	float scale = std::min(windowW / UI_WIDTH, windowH / UI_HEIGHT);
	if (scale <= 0.0f) scale = 1.0f;
	layout.width = static_cast<unsigned>(UI_WIDTH * scale);
	layout.height = static_cast<unsigned>(UI_HEIGHT * scale);
	layout.offsetX = (windowW - layout.width) / 2.0f;
	layout.offsetY = (windowH - layout.height) / 2.0f;
}
