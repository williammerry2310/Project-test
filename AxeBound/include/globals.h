// globals.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//

#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "logger.h"

// --- Fixed sizes (CHANGED/ADDED)
// The UI is designed on a 1280x720 canvas (ui.h scales it to the window).
// The world is drawn with a small 480x270 camera, so one 16px tile is large and "pixel art" on screen.
constexpr float UI_WIDTH = 1280.0f;
constexpr float UI_HEIGHT = 720.0f;
constexpr int WORLD_WIDTH = 480;
constexpr int WORLD_HEIGHT = 270;
constexpr int tileSize = 16; // moved here from render.cpp so every file shares it

// --- Colours taken from the design document mock-ups (ADDED)
namespace palette {
	const sf::Color uiGreen(0x92, 0xd0, 0x50);      // buttons / banners in the mock-ups
	const sf::Color uiGreenLight(0x9f, 0xce, 0x63); // panels in the mock-ups
	const sf::Color uiSelected(0x56, 0xbd, 0x3b);   // selected / hovered item in the mock-ups
	const sf::Color uiGrey(0xbf, 0xbf, 0xbf);       // text input box
	const sf::Color uiDisabled(0x7f, 0x7f, 0x7f);   // greyed out text ("Prev Page")
	const sf::Color grass(0x00, 0xc0, 0x13);
	const sf::Color water(0x4a, 0x86, 0xe8);
	const sf::Color sand(0xff, 0xd9, 0x66);
	const sf::Color trunk(0x78, 0x3f, 0x04);
	const sf::Color canopy(0x3d, 0x90, 0x72);
	const sf::Color bush(0x38, 0x76, 0x1d);
	const sf::Color bridge(0xb4, 0x5f, 0x06);
	const sf::Color playerGrey(0x99, 0x99, 0x99);
	const sf::Color snow(0xe8, 0xf1, 0xf5);
	const sf::Color hill(0x4b, 0x6b, 0x2f);
	const sf::Color brick(0xc0, 0x60, 0x3a);
}

// --- Where the 16:9 game area sits inside the (resizable) window (ADDED)
struct screenLayout {
	float offsetX = 0.0f;   // letterbox offset in window pixels
	float offsetY = 0.0f;
	unsigned width = 1280;  // size of the 16:9 area in window pixels
	unsigned height = 720;
	unsigned windowW = 1280;
	unsigned windowH = 720;
};
extern screenLayout layout;
void updateLayout(unsigned windowW, unsigned windowH);

// --- Everything main() collects from the window events for one frame (ADDED)
struct frameInput {
	sf::Vector2i mouse = {0, 0};  // mouse position in UI coordinates (window pixels minus the letterbox offset)
	bool mouseClicked = false;    // left click this frame
	bool rightClicked = false;
	bool keyPressedEvent = false; // used by the keybind screen
	sf::Keyboard::Key lastKey = sf::Keyboard::Unknown;
	sf::Vector2f move = {0.0f, 0.0f}; // movement request this frame (filled by update.cpp from the input manager)
	bool sprint = false;
	void clearEvents() { mouseClicked = false; rightClicked = false; keyPressedEvent = false; }
};
extern frameInput frame;

extern bool quitRequested;      // set true to close the game cleanly
extern const float maxSpeed; // max speed for player
extern float maxSpeedSq; // max speed squared stored globally to save simple multiplication 60 times per second
extern sf::Texture tileTexture; // tile atlas, built in code (one 16x16 tile per id)
extern sf::Font gameFont;       // dogica pixel font (or a system fallback)
extern logger gameLog;
