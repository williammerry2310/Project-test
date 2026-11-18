// init.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//


// --- Includes
#include <SFML/Graphics.hpp>
#include "cmath"
#include <filesystem>
#include <string>
#include <vector>

#include "init.h"
#include "globals.h"
#include "inputManager.h"
#include "logger.h"
#include "tileMap.h"
#include "saveManager.h"
#include "audio.h"
#include "gameSystem.h"

// --- Private/Local Declerations
static bool loadGameFont();

// --- Global/Declared Methods

// ADDED: assets/ and saves/ are found relative to the working directory, so look for them in the current folder and its parents
void initWorkingDirectory(const char* argv0)
{
	namespace fs = std::filesystem;
	std::error_code ec;
	std::vector<fs::path> starts;
	starts.push_back(fs::current_path(ec));
	if (argv0 != nullptr) starts.push_back(fs::absolute(fs::path(argv0), ec).parent_path());
	for (const fs::path& start : starts){
		fs::path p = start;
		for (int up = 0; up < 4 && !p.empty(); up++){
			if (fs::exists(p / "assets", ec)){
				fs::current_path(p, ec);
				return;
			}
			p = p.parent_path();
		}
	}
}

void init()
{
	// math optimisations
	maxSpeedSq = maxSpeed * maxSpeed; // prevents doing this every frame as its stored, not that multiplication is that expensive.
	
	// inits
	initInputManager();
	loadSettings();
	initAudio();
	
	// tile atlas - CHANGED: built in code from the design colours instead of loading assets/textures/ph.png (that file did not exist)
	buildTileTexture(tileTexture);
	
	if (!loadGameFont()) gameLog.fatal("No font could be loaded! Put dogicapixelbold.ttf in assets/fonts/dogica/TTF/");
	
	GameSystem::get().start(); // builds the menu scene + game scene
	
	gameLog.info("Initilisation Complete!");
}

// --- Private/Local Methods
static bool loadGameFont()
{
	const char* paths[] = {
		"assets/fonts/dogica/TTF/dogicapixelbold.ttf", // the font from the design document (dafont.com)
		"assets/fonts/dogicapixelbold.ttf",
		"C:/Windows/Fonts/arial.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/System/Library/Fonts/Supplemental/Arial.ttf",
		"/Library/Fonts/Arial.ttf"
	};
	for (const char* p : paths){
		if (gameFont.loadFromFile(p)){
			gameLog.info(std::string("Font loaded: ") + p);
			return true;
		}
	}
	return false;
}
