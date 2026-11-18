// gameSystem.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// SingletonGameSystem from the design document. Owns the menu scene and the game scene.

#include "gameSystem.h"
#include "globals.h"
#include "render.h"
#include "audio.h"
#include "saveManager.h"

GameSystem& GameSystem::get()
{
	static GameSystem instance;
	return instance;
}

void GameSystem::start()
{
	menuScene = std::make_unique<MenuScene>();
	gameScene = std::make_unique<GameScene>();
	uiSizeNow = sf::Vector2u(layout.width, layout.height);
	menuScene->onResize(uiSizeNow);
	gameScene->onResize(uiSizeNow);
	setActiveScene(SCENE_MENU);
}

void GameSystem::update(float dt)
{
	if (activeId == SCENE_MENU) menuScene->update(dt);
	else gameScene->update(dt);
	if (pending){ std::function<void()> p = pending; pending = nullptr; p(); }   // scene changes happen here, never inside a button callback
}

void GameSystem::render(sf::RenderWindow& window)
{
	if (activeId == SCENE_MENU) menuScene->render(window);
	else gameScene->render(window);
}

void GameSystem::onResize(sf::Vector2u uiSize)
{
	uiSizeNow = uiSize;
	if (menuScene) menuScene->onResize(uiSize);
	if (gameScene) gameScene->onResize(uiSize);
}

void GameSystem::onText(sf::Uint32 unicode)
{
	if (activeId == SCENE_MENU && menuScene) menuScene->onText(unicode);
}

void GameSystem::setActiveScene(int id)
{
	activeId = id;
	if (id == SCENE_MENU){
		renderMode = 'M';
		menuScene->load();
		playMenuMusic();
	}else{
		renderMode = 'T';
		gameScene->load();
	}
}

void GameSystem::requestMenu()
{
	pending = [this]() {
		if (activeId == SCENE_GAME) gameScene->saveGame();
		setActiveScene(SCENE_MENU);
	};
}

void GameSystem::requestNewGame(const std::string& name, int difficulty)
{
	pending = [this, name, difficulty]() {
		gameScene->newGame(name, difficulty);
		setActiveScene(SCENE_GAME);
	};
}

void GameSystem::requestLoad(int slot)
{
	pending = [this, slot]() {
		if (gameScene->loadGame(slot)) setActiveScene(SCENE_GAME);
	};
}

void GameSystem::shutdown()
{
	if (activeId == SCENE_GAME && gameScene) gameScene->saveGame();
	saveSettings();
}
