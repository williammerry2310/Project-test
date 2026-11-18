// objects.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// All the things that exist in the game world (the entity class tree from the design document):
//
//   entity -+- tree, stump, bush, Item, sellPoint, objectiveBuilding, sceneChangeArea, storeBuilding
//           +- functionalBuilding -+- itemMovementBuilding (canal / conveyor)
//           |                      +- productionBuilding   (sawmill / chipper / kiln)
//           +- dynamicEntity -+- player
//                             +- vehicle
//                             +- workerAI  (State pattern: OffState / OnSearchingState / OnCollectionState)

#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <deque>
#include <cmath>

class GameScene; // the scene that owns every entity (scene.h)

// --- Shared enums / tables
enum treeType { TREE_SPRUCE = 0, TREE_PINE, TREE_OAK, TREE_FROST, TREE_TYPE_COUNT }; // oak + frost are the "rarer" trees
enum itemType { ITEM_LOG_SPRUCE = 0, ITEM_LOG_PINE, ITEM_LOG_OAK, ITEM_LOG_FROST, ITEM_PLANK, ITEM_CHIP, ITEM_TYPE_COUNT };
enum buildingType { B_CANAL = 0, B_CONVEYOR, B_SAWMILL, B_CHIPPER, B_KILN, B_HOUSING, B_CAMPFIRE, B_TYPE_COUNT };
enum entityClass { EC_GENERIC = 0, EC_TREE, EC_STUMP, EC_BUSH, EC_ITEM, EC_SELL, EC_OBJECTIVE, EC_GATE, EC_STORE, EC_BUILDING, EC_WORKER, EC_VEHICLE, EC_PLAYER };
enum storeType { STORE_GENERAL = 0, STORE_LABOUR };

struct buildingInfo { const char* name; int cost; int tilesW; int tilesH; };
const buildingInfo& getBuildingInfo(buildingType t);
const char* itemName(itemType t);
bool itemIsLog(itemType t);
float itemBasePrice(itemType t);
itemType logForTree(treeType t);
int treeMinAxeTier(treeType t);       // stronger trees require better axes (design rule)
float treeMaxHealth(treeType t);

// --- entity
class entity {
	protected:
		int id = 0; // used to reserve items for workers
		double posX = 0.0;   // world position of the BOTTOM CENTRE of the entity (so sorting by posY draws things in the right order)
		double posY = 0.0;
		sf::Vector2f hitSize = {0.0f, 0.0f}; // collision / pick box size
		bool solid = false;
		bool dead = false;
		int eclass = EC_GENERIC;
	public:
		entity();
		virtual ~entity() = default;
		void updatePos(double x, double y);
		sf::Vector2f getPosition() const;
		sf::FloatRect getHitbox() const;
		bool isSolid() const { return solid; }
		bool isDead() const { return dead; }
		void destroy() { dead = true; }
		int getClass() const { return eclass; }
		int getId() const { return id; }
		void setId(int newId) { id = newId; }
		virtual void update(float dt, GameScene& scene) {}
		virtual void draw(sf::RenderWindow& window) const {}
		virtual std::string label() const { return ""; }     // text drawn above the entity in the UI layer
		virtual std::string serialize() const { return ""; } // one save-file line ("" = not saved)
	};

// --- items (logs, planks, woodchips)
class Item : public entity {
	public:
		itemType type;
		bool damp = false;   // logs that travelled down a river/canal are damp and sell for less
		bool dried = false;  // kiln output sells for more
		bool pickedUp = false;
		int reservedBy = 0;  // id of the worker that is going to collect this (0 = free)
		float ignoreTimer = 0.0f;
		Item(itemType t, double x, double y);
		float value() const;
		void update(float dt, GameScene& scene) override; // floats along rivers / canals / conveyors
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	};

// --- trees / stumps / bushes
class tree : public entity {
	public:
		treeType type;
		float health;
		bool felled = false;
		tree(treeType t, double x, double y, float healthMult = 1.0f);
		bool damage(float amount, int axeTier); // returns true when this hit felled the tree
		void fell();
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	};

class stump : public entity {
	public:
		float rotTime; // seconds until the stump rots away
		stump(double x, double y, float rot = 70.0f);
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	};

class bush : public entity { // decoration from the mock-ups
	public:
		bush(double x, double y);
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	};

// --- towns / objectives
class sellPoint : public entity {
	public:
		std::string name;
		sf::FloatRect area;               // anything dropped inside this area is sold
		float basePrice[ITEM_TYPE_COUNT]; // 0 = this buyer does not buy it
		float priceMult[ITEM_TYPE_COUNT]; // drops every sale, recovers slowly (design: only THIS sell point is affected)
		sf::Color colour;
		sellPoint(const std::string& sellName, sf::FloatRect sellArea, sf::Color col);
		void setPrice(itemType t, float price) { basePrice[t] = price; }
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
		std::string label() const override { return name; }
	};

class objectiveBuilding : public entity {
	public:
		int objectiveId;                  // 0 = port, 1 = train station, 2 = airport
		std::string name;
		std::map<int, int> required;      // itemType -> quantity needed
		std::map<int, int> delivered;
		bool built = false;
		sf::FloatRect dropArea;           // items placed here are consumed
		sf::Vector2f size;
		objectiveBuilding(int objId, const std::string& objName, sf::Vector2f bottomCentre, sf::Vector2f buildingSize);
		bool isComplete() const;
		std::string progressText() const;
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
		std::string label() const override { return name + (built ? " (built)" : ""); }
		std::string serialize() const override;
	};

class sceneChangeArea : public entity { // invisible entity next to a finished objective building that moves the player to another level
	public:
		sf::FloatRect area;
		int targetLevel;
		sf::Vector2f targetSpawn;
		std::string name;
		bool active = true;
		sceneChangeArea(sf::FloatRect a, int level, sf::Vector2f spawn, const std::string& gateName);
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
		std::string label() const override { return active ? name : ""; }
	};

class storeBuilding : public entity { // General Store (tools + truck) and Labour Office (hire workers)
	public:
		storeType type;
		std::string name;
		sf::Vector2f size;
		storeBuilding(storeType t, const std::string& storeName, sf::Vector2f bottomCentre, sf::Vector2f buildingSize);
		void draw(sf::RenderWindow& window) const override;
		std::string label() const override { return name; }
	};

// --- buildings the player builds
class functionalBuilding : public entity {
	public:
		buildingType type;
		int dir = 0;                  // 0 = east, 1 = south, 2 = west, 3 = north
		sf::Vector2i tileOrigin;      // top left tile
		sf::Vector2i tilesSize;
		bool occupied = false;        // worker housing: has a worker been assigned to it?
		functionalBuilding(buildingType t, int tx, int ty, int direction);
		sf::FloatRect footprint() const;
		sf::Vector2f dirVector() const;
		bool isHeatSource() const { return type == B_CAMPFIRE; }
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	};

class itemMovementBuilding : public functionalBuilding { // canal (cheap + fast + makes logs damp) or conveyor (expensive + slow)
	public:
		float itemMoveSpeed;
		itemMovementBuilding(buildingType t, int tx, int ty, int direction);
	};

class productionBuilding : public functionalBuilding {   // sawmill -> planks, chipper -> woodchips, kiln -> dries things
	public:
		float productionSpeed;        // seconds per item
		float timer = 0.0f;
		struct queued { itemType type; bool damp; bool dried; };
		std::deque<queued> input;
		productionBuilding(buildingType t, int tx, int ty, int direction);
		void update(float dt, GameScene& scene) override;
	private:
		void produce(const queued& q, GameScene& scene);
	};

// --- dynamic entities
class dynamicEntity : public entity {
	protected:
		sf::Vector2f velocity = {0.0f, 0.0f};
		sf::Vector2f direction = {0.0f, 1.0f}; // facing
	public:
		void updateDirection(sf::Vector2f newDirection);
		void updateVelocity(sf::Vector2f newVelocity);
		sf::Vector2f getDirection() const { return direction; }
		// moves by "delta" and stops at solid tiles / solid entities (x and y separately so you slide along walls)
		void moveWithCollision(sf::Vector2f delta, GameScene& scene);
	};

class player : public dynamicEntity {
	public:
		std::string name;
		float health = 100.0f;
		float frost = 0.0f;                 // cold build up (design: frost build up -> lose health)
		std::shared_ptr<Item> heldItem;     // at most one log in the arms
		bool tools[3] = {true, false, false}; // basic / iron / steel axe
		int currentHotbarSlot = 1;          // 1..3 = axes
		float chopCooldown = 0.0f;
		float swingTimer = 0.0f;
		float walkTimer = 0.0f;
		bool inVehicle = false;
		player(std::string playerName, double startX, double startY);
		void setVelocityPublic(sf::Vector2f v) { velocity = v; }
		void pickupItem(std::shared_ptr<Item> item);
		std::shared_ptr<Item> dropItem();
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
	};

class vehicle : public dynamicEntity { // the truck, 16 directional sprites (here: the rotation snaps to 22.5 degrees)
	public:
		bool occupied = false;
		float heading = 0.0f;               // degrees, 0 = east
		float speedNow = 0.0f;
		vehicle(double x, double y);
		void drive(float throttle, float steer, float dt, GameScene& scene);
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	};

// --- worker AI (State design pattern from the design document)
class workerAI;
class State {
	public:
		virtual ~State() {}
		virtual void handle(workerAI& worker, float dt, GameScene& scene) = 0;
		virtual const char* stateName() const = 0;
	};
class OffState : public State {
	public:
		void handle(workerAI& worker, float dt, GameScene& scene) override;
		const char* stateName() const override { return "Off"; }
	};
class OnSearchingState : public State {
	public:
		float searchTime = 0.0f;
		float pollTimer = 0.0f;
		void handle(workerAI& worker, float dt, GameScene& scene) override;
		const char* stateName() const override { return "Searching"; }
	};
class OnCollectionState : public State {
	public:
		void handle(workerAI& worker, float dt, GameScene& scene) override;
		const char* stateName() const override { return "Collecting"; }
	};

class workerAI : public dynamicEntity {
	public:
		bool isActive = false;
		bool holdingItem = false;               // only one log (design): a bool is enough
		itemType heldType = ITEM_LOG_SPRUCE;
		bool heldDamp = false;
		bool heldDried = false;
		int depositIndex = 0;                   // which deposit location the player picked
		std::shared_ptr<Item> target;
		std::vector<sf::Vector2i> path;         // tile path from A*
		size_t pathIndex = 0;
		float walkTimer = 0.0f;
		workerAI(double x, double y);
		void setState(std::unique_ptr<State> newState); // takes effect after the current handle() returns
		void setActive(bool active);
		std::string stateName() const { return _state ? _state->stateName() : "?"; }
		bool followPath(float dt, GameScene& scene); // returns true when the end of the path was reached
		void update(float dt, GameScene& scene) override;
		void draw(sf::RenderWindow& window) const override;
		std::string serialize() const override;
	private:
		std::unique_ptr<State> _state;
		std::unique_ptr<State> _pending;
	};

// --- helpers shared between files
std::vector<sf::Vector2i> findPathAStar(GameScene& scene, sf::Vector2i from, sf::Vector2i to); // empty = no path
