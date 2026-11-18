// gameSystem.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// SingletonGameSystem from the design document: owns the two scenes (MenuScene = "UI scene", GameScene = "game scene")
// and decides which one is active. update.cpp / render.cpp only talk to this class.

#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>

#include "scene.h"

enum sceneId { SCENE_MENU = 0, SCENE_GAME = 1 };

class GameSystem {
	public:
		static GameSystem& get();                    // singleton
		void start();                                // creates both scenes and shows the main menu
		void update(float dt);                       // updates the active scene, then applies any scene change that was asked for
		void render(sf::RenderWindow& window);       // draws the UI layer of the active scene
		void onResize(sf::Vector2u uiSize);
		void onText(sf::Uint32 unicode);
		void setActiveScene(int id);
		int activeSceneId() const { return activeId; }
		MenuScene& menu() { return *menuScene; }
		GameScene& game() { return *gameScene; }
		// scene changes are DEFERRED to the end of the frame so a button callback can never delete / rebuild the UI it is being called from
		void requestMenu();
		void requestNewGame(const std::string& name, int difficulty);
		void requestLoad(int slot);
		void shutdown();                             // saves the game if it is running (called when the window closes)
	private:
		GameSystem() = default;
		std::unique_ptr<MenuScene> menuScene;
		std::unique_ptr<GameScene> gameScene;
		int activeId = SCENE_MENU;
		std::function<void()> pending;
		sf::Vector2u uiSizeNow = {1280, 720};
	};
