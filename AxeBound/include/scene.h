// scene.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// Scene (base), MenuScene (the "UI scene": main menu, saves, new game, settings, keybinds, credits)
// and GameScene (the "game scene": the actual logging game).

#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "ui.h"
#include "tileMap.h"
#include "objects.h"
#include "levels.h"
#include "saveManager.h"

class Scene {
	public:
		Scene() {}
		virtual ~Scene() {}
		virtual void update(float dt) = 0;
		virtual void render(sf::RenderWindow& window) = 0;
		virtual void load() = 0;      // build / reset the scene
		virtual void unload() {}
		virtual void onResize(sf::Vector2u uiSize) = 0;
		virtual void onText(sf::Uint32 unicode) {}
	};

// =====================================================================================================================
//  MenuScene
// =====================================================================================================================
class MenuScene : public Scene {
	public:
		enum Page { PAGE_MAIN = 0, PAGE_SAVES, PAGE_NEWGAME, PAGE_SETTINGS, PAGE_KEYBINDS, PAGE_CREDITS, PAGE_COUNT };
		MenuScene();
		void update(float dt) override;
		void render(sf::RenderWindow& window) override;
		void load() override;
		void onResize(sf::Vector2u uiSize) override;
		void onText(sf::Uint32 unicode) override;
		void showPage(Page p);
	private:
		uiGroup pages[PAGE_COUNT];
		Page current = PAGE_MAIN;
		sf::Vector2u uiSize = {1280, 720};
		bool built = false;
		// main page
		std::shared_ptr<uiButton> continueButton;
		// saves page
		std::shared_ptr<uiButton> slotButtons[SAVE_SLOTS];
		std::shared_ptr<uiButton> loadButton;
		std::shared_ptr<label> detailsLabel;
		int selectedSlot = 0;
		// new game page
		std::shared_ptr<uiButton> diffButtons[3];
		std::shared_ptr<inputTextBox> nameBox;
		int difficulty = 1;
		// settings page
		std::shared_ptr<uiButton> musicButton;
		std::shared_ptr<uiButton> soundButton;
		// keybinds page
		std::shared_ptr<label> keyNameLabels[4];
		std::shared_ptr<uiButton> keyValueButtons[4];
		std::shared_ptr<uiButton> keyNewButtons[4];
		std::shared_ptr<uiButton> prevPageButton;
		std::shared_ptr<uiButton> nextPageButton;
		std::shared_ptr<label> pageLabel;
		int keyPage = 0;
		int rebinding = -1; // key index being rebound, -1 = none

		void buildMain(); void buildSaves(); void buildNewGame(); void buildSettings(); void buildKeybinds(); void buildCredits();
		void addBanner(uiGroup& g, const std::string& text);
		std::shared_ptr<uiButton> addButton(uiGroup& g, float x, float y, float w, float h, const std::string& text, std::function<void()> cb);
		void refreshSaves();
		void selectSlot(int slot);
		void refreshKeybinds();
		void refreshSettings();
	};

// =====================================================================================================================
//  GameScene
// =====================================================================================================================
class GameScene : public Scene {
	public:
		enum Panel { PANEL_NONE = 0, PANEL_BUILD, PANEL_SHOP, PANEL_WORKERS, PANEL_PAUSE, PANEL_VICTORY };

		struct levelState {
			tileMap map;
			std::vector<std::shared_ptr<entity>> entities;
			std::string name;
			sf::Vector2f spawn;
			bool unlocked = false;
		};

		GameScene();
		// Scene
		void update(float dt) override;
		void render(sf::RenderWindow& window) override;   // HUD / UI layer
		void load() override;
		void onResize(sf::Vector2u uiSize) override;
		void drawWorld(sf::RenderWindow& window);          // the world layer (called from render.cpp)

		// starting / loading / saving
		void newGame(const std::string& name, int diff);
		bool loadGame(int slot);
		bool saveGame();

		// ----- game state (public so entities can use it, as in the design: money, entities, world) -----------
		std::vector<levelState> levels;
		int currentLevel = 0;
		std::shared_ptr<player> thePlayer;
		std::shared_ptr<vehicle> theVehicle;
		int vehicleLevel = -1;
		int money = 0;
		int difficulty = 1;           // 0 easy, 1 normal, 2 hard
		int treesChopped = 0;
		int saveSlot = 0;
		std::string playerName = "Lumberjack";
		bool airportComplete = false;
		float gateCooldown = 0.0f;
		bool truckOwned = false;
		std::vector<std::shared_ptr<Item>> truckCargo; // logs loaded on the truck (several at a time, design)

		std::vector<std::shared_ptr<entity>>& get_entities() { return levels[currentLevel].entities; }
		tileMap& world() { return levels[currentLevel].map; }
		void addEntity(std::shared_ptr<entity> e) { levels[currentLevel].entities.push_back(e); }
		void addEntityToLevel(int level, std::shared_ptr<entity> e) { levels[level].entities.push_back(e); }
		void incrementMoney(int amount) { money += amount; }
		bool decrementMoney(int amount) { if (money < amount) return false; money -= amount; return true; }
		void toast(const std::string& text, float seconds = 3.0f);

		// ----- helpers used by the entities ----------------------------------------------------------------
		bool isBlocked(const sf::FloatRect& box, const entity* ignore);
		bool tileBlocked(int tx, int ty);                          // tile is solid or covered by a solid entity
		bool driftAt(const sf::Vector2f& pos, sf::Vector2f& dir, float& speed, bool& damp); // does a river / canal / conveyor carry an item at pos?
		std::shared_ptr<Item> spawnItem(itemType t, double x, double y, bool damp = false, bool dried = false);
		void spawnLogsFromTree(const tree& t);
		void onObjectiveBuilt(objectiveBuilding& o, bool silent);
		void changeLevel(int level, sf::Vector2f spawn);
		bool isNearHeat(sf::Vector2f pos);
		std::shared_ptr<Item> findFreeLog(const workerAI& w);
		sf::Vector2f depositPosition(int index);
		int depositCount();
		std::string depositName(int index);
		float priceFactor() const;     // difficulty scales selling prices
		float frostFactor() const;     // difficulty scales how fast you freeze
		float treeHealthFactor() const;
		int currentObjective();        // 1..3 for the pause screen
		int treeCount(int level);
		void spawnTreeAt(int level, int tx, int ty);

	private:
		// ui
		uiGroup hud, buildPanel, shopPanel, workerPanel, pausePanel, victoryPanel;
		std::shared_ptr<uiButton> menuBtn, buildBtn, shopBtn, workersBtn;
		std::shared_ptr<label> coinsLabel, healthLabel, hotbarLabel, toolLabel[3], toastLabel, hintLabel, objectiveLabel, pauseInfo, shopInfo, workerInfo, victoryLabel;
		std::shared_ptr<uiButton> buildItemBtn[B_TYPE_COUNT];
		std::shared_ptr<uiButton> shopIronBtn, shopSteelBtn, shopTruckBtn;
		std::shared_ptr<uiButton> hireBtn, workerToggle[6], workerDeposit[6];
		std::shared_ptr<label> workerRowLabel[6];
		sf::Vector2u uiSize = {1280, 720};
		bool built = false;
		Panel panel = PANEL_NONE;
		std::function<void()> deferred;      // a button click is applied AFTER the UI loop (so it can never change the UI it is running in)
		std::string toastText;
		float toastTimer = 0.0f;
		float autosaveTimer = 0.0f;
		float treeTimer = 0.0f;
		sf::View worldView;
		sf::Vector2f cameraCentre = {0.0f, 0.0f};
		// building placement
		int placing = -1;     // buildingType being placed, -1 = not placing
		int placeDir = 0;
		sf::Vector2f cursorUi = {640.0f, 360.0f}; // controller cursor
		bool usingCursor = false;
		float footstepTimer = 0.0f;
		float campTimer = 0.0f;
		int pendingLevel = -1;               // level changes are applied at the end of the frame
		sf::Vector2f pendingSpawn = {0.0f, 0.0f};
		std::vector<itemMovementBuilding*> moverCache; // canals + conveyors, rebuilt every frame for driftAt()
		float warnTimer = 0.0f;
		float chopHintTimer = 0.0f;

		void buildUi();
		void openPanel(Panel p);
		void closePanels();
		void refreshHud();
		void refreshPanels();
		void updatePlayer(float dt);
		void updateVehicle(float dt);
		void updateCold(float dt);
		void updateTrees(float dt);
		void handleChop();
		void handlePickup();
		void handleDrop();
		void handleInteract();
		void tryPlace();
		bool canPlace(buildingType t, int tx, int ty);
		sf::Vector2i placementTile(buildingType t);
		void hireWorker();
		void buyTool(int tier);
		void buyTruck();
		void cleanDead();
		void drawPlacementGhost(sf::RenderWindow& window);
		void drawOverheadLabels(sf::RenderWindow& window);
		std::shared_ptr<Item> nearestItem(sf::Vector2f pos, float radius);
		std::shared_ptr<tree> nearestTree(sf::Vector2f pos, float radius);
		storeBuilding* nearestStore(sf::Vector2f pos, float radius, int type);
		sf::Vector2f pointerUi();
	};
