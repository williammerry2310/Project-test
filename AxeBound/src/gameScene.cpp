// gameScene.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// GameScene = the "game scene" in the design flowchart ("Gameplay"): new game / load, player movement, chopping,
// carrying logs, selling, building, workers, cold areas, objectives, level changes, saving.
// (The HUD / panels / drawing are in gameUi.cpp.)

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <sstream>
#include <random>
#include <filesystem>

#include "scene.h"
#include "gameSystem.h"
#include "globals.h"
#include "inputManager.h"
#include "saveManager.h"
#include "audio.h"
#include "render.h"

static std::mt19937 gameRng(static_cast<unsigned>(std::time(nullptr)));
static const float AXE_DAMAGE[3] = {12.0f, 24.0f, 45.0f};
static const int IRON_AXE_COST = 300, STEEL_AXE_COST = 900, TRUCK_COST = 1200, WORKER_COST = 100;

GameScene::GameScene() {}

// =====================================================================================================================
//  small helpers
// =====================================================================================================================
float GameScene::priceFactor() const { return difficulty == 0 ? 1.3f : (difficulty == 2 ? 0.8f : 1.0f); }
float GameScene::frostFactor() const { return difficulty == 0 ? 0.6f : (difficulty == 2 ? 1.5f : 1.0f); }
float GameScene::treeHealthFactor() const { return difficulty == 0 ? 0.7f : (difficulty == 2 ? 1.4f : 1.0f); }

void GameScene::toast(const std::string& text, float seconds)
{
	toastText = text;
	toastTimer = seconds;
}

void GameScene::load()
{
	if (!built) { buildUi(); built = true; }
	closePanels();
	placing = -1;
	playAmbientMusic();
}

void GameScene::onResize(sf::Vector2u newSize)
{
	uiSize = newSize;
	hud.resize(newSize); buildPanel.resize(newSize); shopPanel.resize(newSize);
	workerPanel.resize(newSize); pausePanel.resize(newSize); victoryPanel.resize(newSize);
}

// =====================================================================================================================
//  new game / load / save
// =====================================================================================================================
void GameScene::newGame(const std::string& name, int diff)
{
	playerName = name;
	difficulty = std::max(0, std::min(2, diff));
	money = (difficulty == 0) ? 300 : (difficulty == 1 ? 100 : 0);
	treesChopped = 0;
	airportComplete = false;
	truckOwned = false;
	truckCargo.clear();
	theVehicle.reset();
	vehicleLevel = -1;
	currentLevel = 0;
	gateCooldown = 1.0f;
	levels.clear();
	levels.resize(LEVEL_COUNT);
	for (int i = 0; i < LEVEL_COUNT; i++) buildLevel(*this, i, true);
	thePlayer = std::make_shared<player>(name, levels[0].spawn.x, levels[0].spawn.y);
	saveSlot = firstFreeOrOldestSlot();
	cameraCentre = levels[0].spawn;
	autosaveTimer = 0.0f;
	pendingLevel = -1;
	closePanels();
	placing = -1;
	saveGame();                                       // flowchart: "Initialise new game save"
	toast("Welcome " + name + "! Chop trees (F) and sell the logs in town.", 6.0f);
	gameLog.info("New game started: " + name);
}

int GameScene::currentObjective()
{
	bool port = false, station = false;
	if (levels.size() > 0){
		for (auto& e : levels[0].entities){
			if (e->getClass() != EC_OBJECTIVE) continue;
			objectiveBuilding* o = static_cast<objectiveBuilding*>(e.get());
			if (o->objectiveId == 0) port = o->built;
			if (o->objectiveId == 1) station = o->built;
		}
	}
	if (!port) return 1;
	if (!station) return 2;
	return 3;
}

bool GameScene::saveGame()
{
	if (!thePlayer || levels.empty()) return false;
	std::error_code ec;
	std::filesystem::create_directories("saves", ec);
	if (saveSlot < 1) saveSlot = firstFreeOrOldestSlot();
	std::ofstream f(slotPath(saveSlot), std::ios::trunc);
	if (!f.is_open()) { gameLog.error("GameScene::saveGame: can not write " + slotPath(saveSlot)); return false; }
	std::time_t now = std::time(nullptr);
	char buf[16]; std::strftime(buf, sizeof(buf), "%d/%m/%y", std::localtime(&now));
	f << "name " << playerName << "\n";
	f << "date " << buf << "\n";
	f << "epoch " << static_cast<long long>(now) << "\n";
	f << "difficulty " << difficulty << "\n";
	f << "level " << currentLevel << "\n";
	f << "money " << money << "\n";
	f << "trees " << treesChopped << "\n";
	f << "objective " << currentObjective() << "\n";
	f << "player " << thePlayer->getPosition().x << " " << thePlayer->getPosition().y << " " << thePlayer->health << " " << thePlayer->frost << " "
	  << (thePlayer->tools[0] ? 1 : 0) << " " << (thePlayer->tools[1] ? 1 : 0) << " " << (thePlayer->tools[2] ? 1 : 0) << " " << thePlayer->currentHotbarSlot << "\n";
	f << "truck " << (truckOwned ? 1 : 0) << " " << vehicleLevel;
	if (theVehicle) f << " " << theVehicle->getPosition().x << " " << theVehicle->getPosition().y << " " << theVehicle->heading;
	else f << " 0 0 0";
	f << "\n";
	f << "airport " << (airportComplete ? 1 : 0) << "\n";
	f << "ENTITIES\n";
	for (int l = 0; l < static_cast<int>(levels.size()); l++){
		f << "LEVEL " << l << "\n";
		for (auto& e : levels[l].entities){
			if (e->isDead()) continue;
			if (e->getClass() == EC_ITEM && static_cast<Item*>(e.get())->pickedUp) continue; // held / loaded items are written below
			std::string line = e->serialize();
			if (!line.empty()) f << line << "\n";
		}
		if (l == currentLevel && thePlayer->heldItem){      // the log in the player's arms is written at the player's feet
			Item* it = thePlayer->heldItem.get();
			f << "I " << static_cast<int>(it->type) << " " << thePlayer->getPosition().x << " " << thePlayer->getPosition().y << " " << (it->damp ? 1 : 0) << " " << (it->dried ? 1 : 0) << "\n";
		}
		if (l == vehicleLevel && theVehicle){               // logs on the truck are put on the ground next to it
			for (auto& c : truckCargo) f << "I " << static_cast<int>(c->type) << " " << theVehicle->getPosition().x << " " << theVehicle->getPosition().y + 4.0f << " " << (c->damp ? 1 : 0) << " " << (c->dried ? 1 : 0) << "\n";
		}
	}
	return true;
}

bool GameScene::loadGame(int slot)
{
	std::ifstream f(slotPath(slot));
	if (!f.is_open()) { gameLog.error("GameScene::loadGame: no save in slot " + std::to_string(slot)); return false; }
	levels.clear();
	levels.resize(LEVEL_COUNT);
	for (int i = 0; i < LEVEL_COUNT; i++) buildLevel(*this, i, false); // premade map + static entities, the trees come from the file
	truckCargo.clear();
	theVehicle.reset();
	vehicleLevel = -1; truckOwned = false; airportComplete = false;
	pendingLevel = -1;
	saveSlot = slot;
	currentLevel = 0;
	treesChopped = 0; money = 0; difficulty = 1; playerName = "Lumberjack";
	float px = levels[0].spawn.x, py = levels[0].spawn.y, hp = 100.0f, fr = 0.0f; int t0 = 1, t1 = 0, t2 = 0, hot = 1;
	float vx = 0, vy = 0, vh = 0;
	std::string line;
	int curLevel = -1;
	bool entitiesPart = false;
	while (std::getline(f, line)){
		if (line.empty()) continue;
		std::istringstream in(line);
		std::string key; in >> key;
		if (!entitiesPart){
			if (key == "ENTITIES") { entitiesPart = true; continue; }
			if (key == "name") std::getline(in >> std::ws, playerName);
			else if (key == "difficulty") in >> difficulty;
			else if (key == "level") in >> currentLevel;
			else if (key == "money") in >> money;
			else if (key == "trees") in >> treesChopped;
			else if (key == "player") in >> px >> py >> hp >> fr >> t0 >> t1 >> t2 >> hot;
			else if (key == "truck") { int owned; in >> owned >> vehicleLevel >> vx >> vy >> vh; truckOwned = owned != 0; }
			else if (key == "airport") { int a; in >> a; airportComplete = a != 0; }
			continue;
		}
		if (key == "LEVEL") { in >> curLevel; continue; }
		if (curLevel < 0 || curLevel >= LEVEL_COUNT) continue;
		if (key == "T"){ int ty; double x, y; float h; in >> ty >> x >> y >> h; auto t = std::make_shared<tree>(static_cast<treeType>(std::max(0, std::min(static_cast<int>(TREE_TYPE_COUNT) - 1, ty))), x, y, 1.0f); t->health = h; levels[curLevel].entities.push_back(t); }
		else if (key == "S"){ double x, y; float r; in >> x >> y >> r; levels[curLevel].entities.push_back(std::make_shared<stump>(x, y, r)); }
		else if (key == "I"){ int ty, d1, d2; double x, y; in >> ty >> x >> y >> d1 >> d2; auto it = std::make_shared<Item>(static_cast<itemType>(std::max(0, std::min(static_cast<int>(ITEM_TYPE_COUNT) - 1, ty))), x, y); it->damp = d1 != 0; it->dried = d2 != 0; levels[curLevel].entities.push_back(it); }
		else if (key == "B"){
			int ty, tx, tyy, d, occ; in >> ty >> tx >> tyy >> d >> occ;
			buildingType bt = static_cast<buildingType>(std::max(0, std::min(static_cast<int>(B_TYPE_COUNT) - 1, ty)));
			std::shared_ptr<functionalBuilding> b;
			if (bt == B_CANAL || bt == B_CONVEYOR) b = std::make_shared<itemMovementBuilding>(bt, tx, tyy, d);
			else if (bt == B_SAWMILL || bt == B_CHIPPER || bt == B_KILN) b = std::make_shared<productionBuilding>(bt, tx, tyy, d);
			else b = std::make_shared<functionalBuilding>(bt, tx, tyy, d);
			b->occupied = occ != 0;
			levels[curLevel].entities.push_back(b);
		}
		else if (key == "W"){ double x, y; int dep, act; in >> x >> y >> dep >> act; auto w = std::make_shared<workerAI>(x, y); w->depositIndex = dep; w->setActive(act != 0); levels[curLevel].entities.push_back(w); }
		else if (key == "O"){
			int id, built; in >> id >> built;
			for (auto& e : levels[curLevel].entities){
				if (e->getClass() != EC_OBJECTIVE) continue;
				objectiveBuilding* o = static_cast<objectiveBuilding*>(e.get());
				if (o->objectiveId != id) continue;
				for (int i = 0; i < ITEM_TYPE_COUNT; i++){ int d = 0; in >> d; if (d > 0) o->delivered[i] = d; }
				o->built = built != 0;
				if (o->built) onObjectiveBuilt(*o, true);
			}
		}
	}
	currentLevel = std::max(0, std::min(LEVEL_COUNT - 1, currentLevel));
	thePlayer = std::make_shared<player>(playerName, px, py);
	thePlayer->health = hp; thePlayer->frost = fr;
	thePlayer->tools[0] = t0 != 0; thePlayer->tools[1] = t1 != 0; thePlayer->tools[2] = t2 != 0;
	thePlayer->currentHotbarSlot = std::max(1, std::min(3, hot));
	if (truckOwned && vehicleLevel >= 0){
		theVehicle = std::make_shared<vehicle>(vx, vy);
		theVehicle->heading = vh;
	}
	cameraCentre = thePlayer->getPosition();
	gateCooldown = 1.0f;
	autosaveTimer = 0.0f;
	closePanels();
	placing = -1;
	toast("Game loaded. Welcome back " + playerName + "!", 4.0f);
	gameLog.info("Loaded save slot " + std::to_string(slot));
	return true;
}

// =====================================================================================================================
//  world helpers used by the entities
// =====================================================================================================================
bool GameScene::isBlocked(const sf::FloatRect& box, const entity* ignore)
{
	tileMap& m = world();
	int x0 = static_cast<int>(std::floor(box.left / tileSize)), x1 = static_cast<int>(std::floor((box.left + box.width - 0.001f) / tileSize));
	int y0 = static_cast<int>(std::floor(box.top / tileSize)), y1 = static_cast<int>(std::floor((box.top + box.height - 0.001f) / tileSize));
	for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (m.isSolid(x, y)) return true;
	for (auto& e : get_entities()){
		if (!e->isSolid() || e->isDead() || e.get() == ignore) continue;
		if (e->getHitbox().intersects(box)) return true;
	}
	return false;
}

bool GameScene::tileBlocked(int tx, int ty)
{
	if (world().isSolid(tx, ty)) return true;
	sf::FloatRect r(static_cast<float>(tx * tileSize), static_cast<float>(ty * tileSize), static_cast<float>(tileSize), static_cast<float>(tileSize));
	for (auto& e : get_entities()){
		if (!e->isSolid() || e->isDead()) continue;
		if (e->getHitbox().intersects(r)) return true;
	}
	return false;
}

// does a river / canal / conveyor carry an item that sits at pos?
bool GameScene::driftAt(const sf::Vector2f& pos, sf::Vector2f& dir, float& speed, bool& damp)
{
	sf::Vector2f probe(pos.x, pos.y - 2.0f);
	for (itemMovementBuilding* b : moverCache){
		if (b->footprint().contains(probe)){
			dir = b->dirVector(); speed = b->itemMoveSpeed; damp = (b->type == B_CANAL);
			return true;
		}
	}
	sf::Vector2i t = world().getTileOnScreen(probe);
	sf::Vector2f flow = world().getFlow(t.x, t.y);
	if (flow.x != 0.0f || flow.y != 0.0f){
		dir = flow; speed = 22.0f; damp = true;
		return true;
	}
	return false;
}

std::shared_ptr<Item> GameScene::spawnItem(itemType t, double x, double y, bool damp, bool dried)
{
	auto it = std::make_shared<Item>(t, x, y);
	it->damp = damp; it->dried = dried;
	addEntity(it);
	return it;
}

void GameScene::spawnLogsFromTree(const tree& t)
{
	static const int counts[TREE_TYPE_COUNT] = {2, 2, 3, 4};
	sf::Vector2f p = t.getPosition();
	std::uniform_real_distribution<float> jx(-12.0f, 12.0f), jy(2.0f, 12.0f);
	for (int i = 0; i < counts[t.type]; i++){
		float x = p.x + jx(gameRng), y = p.y + jy(gameRng);
		if (world().isSolid(world().getTileOnScreen(sf::Vector2f(x, y - 2.0f)).x, world().getTileOnScreen(sf::Vector2f(x, y - 2.0f)).y)) { x = p.x; y = p.y + 4.0f; } // never drop a log into a wall
		spawnItem(logForTree(t.type), x, y);
	}
	addEntity(std::make_shared<stump>(p.x, p.y));
}

int GameScene::treeCount(int level)
{
	int n = 0;
	for (auto& e : levels[level].entities) if (e->getClass() == EC_TREE && !e->isDead()) n++;
	return n;
}

void GameScene::spawnTreeAt(int level, int tx, int ty)
{
	unsigned char tile = levels[level].map.getTileAt(tx, ty);
	std::uniform_int_distribution<int> roll(0, 99);
	int r = roll(gameRng);
	treeType type = TREE_SPRUCE;
	if (tile == T_SNOW) type = (r < 55) ? TREE_FROST : (r < 85 ? TREE_SPRUCE : TREE_OAK);
	else if (level == 0) type = (r < 55) ? TREE_SPRUCE : (r < 90 ? TREE_PINE : TREE_OAK);
	else type = (r < 40) ? TREE_SPRUCE : (r < 70 ? TREE_PINE : TREE_OAK);
	std::uniform_real_distribution<float> jx(-3.0f, 3.0f);
	levels[level].entities.push_back(std::make_shared<tree>(type, tx * tileSize + 8.0f + jx(gameRng), ty * tileSize + 14.0f, treeHealthFactor()));
}

bool GameScene::isNearHeat(sf::Vector2f pos)
{
	for (auto& e : get_entities()){
		if (e->getClass() != EC_BUILDING || e->isDead()) continue;
		functionalBuilding* b = static_cast<functionalBuilding*>(e.get());
		if (!b->isHeatSource()) continue;
		sf::Vector2f d = b->getPosition() - pos;
		if (d.x * d.x + (d.y - 8.0f) * (d.y - 8.0f) < 56.0f * 56.0f) return true;
	}
	return false;
}

std::shared_ptr<Item> GameScene::findFreeLog(const workerAI& w)
{
	std::shared_ptr<Item> best;
	float bestD = 1e9f;
	sf::Vector2f wp = w.getPosition();
	for (auto& e : get_entities()){
		if (e->getClass() != EC_ITEM || e->isDead()) continue;
		Item* it = static_cast<Item*>(e.get());
		if (!itemIsLog(it->type) || it->pickedUp || it->reservedBy != 0 || it->ignoreTimer > 0.0f) continue;
		sf::Vector2f dir; float sp; bool dm;
		if (driftAt(it->getPosition(), dir, sp, dm)) continue; // floating on a river / belt: leave it
		bool inArea = false;
		for (auto& o : get_entities()){                         // already in a sell point / objective area: it will be handled there
			if (o->getClass() == EC_SELL && static_cast<sellPoint*>(o.get())->area.contains(it->getPosition())) { inArea = true; break; }
			if (o->getClass() == EC_OBJECTIVE && static_cast<objectiveBuilding*>(o.get())->dropArea.contains(it->getPosition())) { inArea = true; break; }
		}
		if (inArea) continue;
		sf::Vector2f d = it->getPosition() - wp;
		float dist = d.x * d.x + d.y * d.y;
		if (dist < bestD) { bestD = dist; best = std::static_pointer_cast<Item>(e); }
	}
	return best;
}

// deposit locations for the workers = every sell point + every objective building of the current level
int GameScene::depositCount()
{
	int n = 0;
	for (auto& e : get_entities()) if (e->getClass() == EC_SELL || (e->getClass() == EC_OBJECTIVE && !static_cast<objectiveBuilding*>(e.get())->built)) n++;
	return n;
}
static entity* depositEntity(GameScene& s, int index)
{
	int n = 0;
	for (auto& e : s.get_entities()){
		if (e->getClass() == EC_SELL || (e->getClass() == EC_OBJECTIVE && !static_cast<objectiveBuilding*>(e.get())->built)){
			if (n == index) return e.get();
			n++;
		}
	}
	return nullptr;
}
std::string GameScene::depositName(int index)
{
	entity* e = depositEntity(*this, index);
	return e ? e->label() : "none";
}
sf::Vector2f GameScene::depositPosition(int index)
{
	entity* e = depositEntity(*this, index);
	if (!e) return thePlayer ? thePlayer->getPosition() : sf::Vector2f(0, 0);
	if (e->getClass() == EC_SELL){ sf::FloatRect a = static_cast<sellPoint*>(e)->area; return sf::Vector2f(a.left + a.width / 2.0f, a.top + a.height / 2.0f + 4.0f); }
	sf::FloatRect a = static_cast<objectiveBuilding*>(e)->dropArea;
	return sf::Vector2f(a.left + a.width / 2.0f, a.top + a.height / 2.0f + 4.0f);
}

void GameScene::onObjectiveBuilt(objectiveBuilding& o, bool silent)
{
	if (o.objectiveId == 0 || o.objectiveId == 1){
		int target = (o.objectiveId == 0) ? 1 : 2;
		for (auto& e : levels[0].entities){
			if (e->getClass() != EC_GATE) continue;
			sceneChangeArea* g = static_cast<sceneChangeArea*>(e.get());
			if (g->targetLevel == target) g->active = true;      // the invisible sceneChangeArea appears next to the finished building
		}
		levels[target].unlocked = true;
		if (!silent) toast(o.name + " built! " + levels[target].name + " is now open - walk into the white square.", 6.0f);
	}else{
		airportComplete = true;
		if (!silent) { toast("The airport is finished!", 5.0f); openPanel(PANEL_VICTORY); }
	}
}

// the gate only records the request, the level really changes at the end of update() (so no entity loop is disturbed)
void GameScene::changeLevel(int level, sf::Vector2f spawn)
{
	if (level < 0 || level >= static_cast<int>(levels.size()) || pendingLevel >= 0) return;
	pendingLevel = level;
	pendingSpawn = spawn;
}

// =====================================================================================================================
//  finding things near the player
// =====================================================================================================================
std::shared_ptr<Item> GameScene::nearestItem(sf::Vector2f pos, float radius)
{
	std::shared_ptr<Item> best; float bestD = radius * radius;
	for (auto& e : get_entities()){
		if (e->getClass() != EC_ITEM || e->isDead()) continue;
		Item* it = static_cast<Item*>(e.get());
		if (it->pickedUp) continue;
		sf::Vector2f d = it->getPosition() - pos;
		float dist = d.x * d.x + d.y * d.y;
		if (dist < bestD) { bestD = dist; best = std::static_pointer_cast<Item>(e); }
	}
	return best;
}

std::shared_ptr<tree> GameScene::nearestTree(sf::Vector2f pos, float radius)
{
	std::shared_ptr<tree> best; float bestD = radius * radius;
	for (auto& e : get_entities()){
		if (e->getClass() != EC_TREE || e->isDead()) continue;
		sf::Vector2f d = e->getPosition() - pos;
		float dist = d.x * d.x + d.y * d.y;
		if (dist < bestD) { bestD = dist; best = std::static_pointer_cast<tree>(e); }
	}
	return best;
}

storeBuilding* GameScene::nearestStore(sf::Vector2f pos, float radius, int type)
{
	for (auto& e : get_entities()){
		if (e->getClass() != EC_STORE) continue;
		storeBuilding* s = static_cast<storeBuilding*>(e.get());
		if (type >= 0 && static_cast<int>(s->type) != type) continue;
		sf::Vector2f d = s->getPosition() - pos;
		if (d.x * d.x + d.y * d.y < radius * radius) return s;
	}
	return nullptr;
}

sf::Vector2f GameScene::pointerUi()
{
	if (inputControllerConnected()) return cursorUi;
	return sf::Vector2f(static_cast<float>(frame.mouse.x), static_cast<float>(frame.mouse.y));
}

// =====================================================================================================================
//  player actions (flowchart "Gameplay": chop, pick up, carry, drop into river / canal / sell point)
// =====================================================================================================================
void GameScene::handleChop()
{
	if (thePlayer->chopCooldown > 0.0f) return;
	sf::Vector2f pp = thePlayer->getPosition();
	std::shared_ptr<tree> t = nearestTree(pp, 22.0f);       // "user chop input and tree within range?"
	if (!t) return;
	int tier = thePlayer->currentHotbarSlot - 1;
	thePlayer->chopCooldown = 0.5f;
	thePlayer->swingTimer = 0.3f;
	thePlayer->updateDirection(t->getPosition() - pp);
	playSound(SND_AXE);
	if (tier < treeMinAxeTier(t->type)){
		toast("This tree needs a stronger axe! (Tool " + std::to_string(treeMinAxeTier(t->type) + 1) + " or better - buy one at the General Store)", 2.5f);
		return;
	}
	if (t->damage(AXE_DAMAGE[tier], tier)){                 // "Deal damage to the tree" -> "tree health <= 0" -> remove tree, add log items
		treesChopped++;
		spawnLogsFromTree(*t);
	}
}

void GameScene::handlePickup()
{
	if (thePlayer->inVehicle){                              // the truck carries several logs at a time
		if (truckCargo.size() >= 8) { toast("The truck is full (8 items).", 2.0f); return; }
		std::shared_ptr<Item> it = nearestItem(theVehicle->getPosition(), 26.0f);
		if (!it) return;
		it->pickedUp = true;
		truckCargo.push_back(it);
		return;
	}
	if (thePlayer->heldItem) { toast("Your arms are full. Drop it (Q) first - you can only carry one item.", 2.0f); return; }
	std::shared_ptr<Item> it = nearestItem(thePlayer->getPosition(), 16.0f);   // "player interacts with item with empty main slot"
	if (it) thePlayer->pickupItem(it);                                          // "Carry log in main slot"
}

void GameScene::handleDrop()
{
	if (thePlayer->inVehicle){
		sf::Vector2f vp = theVehicle->getPosition();
		sf::Vector2f back = -theVehicle->getDirection();
		int n = 0;
		for (auto& c : truckCargo){
			c->updatePos(vp.x + back.x * 14.0f + (n % 3) * 4.0f - 4.0f, vp.y + back.y * 14.0f + (n / 3) * 3.0f);
			c->pickedUp = false;
			n++;
		}
		truckCargo.clear();
		return;
	}
	if (!thePlayer->heldItem) return;
	sf::Vector2f p = thePlayer->getPosition() + thePlayer->getDirection() * 12.0f;     // dropped in front: into a river / canal / sell point
	std::shared_ptr<Item> it = thePlayer->dropItem();
	it->updatePos(p.x, p.y);
	it->ignoreTimer = 0.0f;
}

void GameScene::handleInteract()
{
	bool pressed = inputJustPressed("Interact");
	if (thePlayer->inVehicle){
		if (!pressed) return;
		thePlayer->inVehicle = false;
		theVehicle->occupied = false;
		sf::Vector2f vp = theVehicle->getPosition();
		sf::Vector2f side(-theVehicle->getDirection().y, theVehicle->getDirection().x);
		sf::Vector2f spot = vp + side * 16.0f;
		if (isBlocked(sf::FloatRect(spot.x - 4.0f, spot.y - 6.0f, 8.0f, 6.0f), thePlayer.get())) spot = vp - side * 16.0f;
		thePlayer->updatePos(spot.x, spot.y);
		return;
	}
	if (pressed && theVehicle && vehicleLevel == currentLevel){
		sf::Vector2f d = theVehicle->getPosition() - thePlayer->getPosition();
		if (d.x * d.x + d.y * d.y < 30.0f * 30.0f){
			if (thePlayer->heldItem) { toast("Drop the log you are carrying before driving.", 2.0f); return; }
			thePlayer->inVehicle = true;
			theVehicle->occupied = true;
			return;
		}
	}
	if (nearestTree(thePlayer->getPosition(), 22.0f)) { handleChop(); return; }       // hold F to keep chopping
	if (pressed){
		if (nearestStore(thePlayer->getPosition(), 70.0f, STORE_GENERAL)) openPanel(PANEL_SHOP);
		else if (nearestStore(thePlayer->getPosition(), 70.0f, STORE_LABOUR)) openPanel(PANEL_WORKERS);
	}
}

void GameScene::buyTool(int tier)
{
	if (!nearestStore(thePlayer->getPosition(), 90.0f, STORE_GENERAL)) { toast("Walk to the General Store to buy tools.", 2.5f); return; }
	int cost = (tier == 1) ? IRON_AXE_COST : STEEL_AXE_COST;
	if (thePlayer->tools[tier]) return;
	if (!decrementMoney(cost)) { toast("Not enough money (" + std::to_string(cost) + " needed).", 2.5f); return; }   // "Check if the player has enough money"
	thePlayer->tools[tier] = true;
	thePlayer->currentHotbarSlot = tier + 1;
	toast(tier == 1 ? "Iron axe bought! (Tool 2)" : "Steel axe bought! (Tool 3)", 3.0f);
}

void GameScene::buyTruck()
{
	storeBuilding* st = nearestStore(thePlayer->getPosition(), 90.0f, STORE_GENERAL);
	if (!st) { toast("Walk to the General Store to buy a truck.", 2.5f); return; }
	if (truckOwned) return;
	if (!decrementMoney(TRUCK_COST)) { toast("Not enough money (" + std::to_string(TRUCK_COST) + " needed).", 2.5f); return; }
	truckOwned = true;
	sf::Vector2f sp = st->getPosition() + sf::Vector2f(0.0f, 30.0f);
	theVehicle = std::make_shared<vehicle>(sp.x, sp.y);
	vehicleLevel = currentLevel;
	toast("Truck bought! Stand next to it and press F. W/S = throttle, A/D = turn.", 5.0f);
}

void GameScene::hireWorker()
{
	if (!nearestStore(thePlayer->getPosition(), 90.0f, STORE_LABOUR)) { toast("Walk to the Labour Office to hire a worker.", 2.5f); return; }
	functionalBuilding* home = nullptr;                      // "check if there is unoccupied worker housing built"
	for (auto& e : get_entities()){
		if (e->getClass() != EC_BUILDING) continue;
		functionalBuilding* b = static_cast<functionalBuilding*>(e.get());
		if (b->type == B_HOUSING && !b->occupied) { home = b; break; }
	}
	if (!home) { toast("No free worker housing. Build Worker Housing first (B).", 3.0f); return; }
	int count = 0;
	for (auto& e : get_entities()) if (e->getClass() == EC_WORKER) count++;
	if (count >= 6) { toast("You can not hire more than 6 workers.", 2.5f); return; }
	if (!decrementMoney(WORKER_COST)) { toast("Not enough money (" + std::to_string(WORKER_COST) + " needed).", 2.5f); return; }
	home->occupied = true;
	sf::Vector2f door = home->getPosition() + sf::Vector2f(0.0f, 4.0f);
	addEntity(std::make_shared<workerAI>(door.x, door.y));  // initial state: inactive (Off)
	toast("Worker hired! Open the Workers menu (M) to give orders.", 4.0f);
}

// =====================================================================================================================
//  building (flowchart: building menu -> select building + enough money -> place on screen)
// =====================================================================================================================
sf::Vector2i GameScene::placementTile(buildingType t)
{
	sf::Vector2f ui = pointerUi();
	float s = 480.0f / static_cast<float>(layout.width);
	sf::Vector2f world(cameraCentre.x - 240.0f + ui.x * s, cameraCentre.y - 135.0f + ui.y * s);
	const buildingInfo& info = getBuildingInfo(t);
	sf::Vector2i tile = this->world().getTileOnScreen(world);
	return sf::Vector2i(tile.x - info.tilesW / 2, tile.y - info.tilesH / 2);
}

bool GameScene::canPlace(buildingType t, int tx, int ty)
{
	const buildingInfo& info = getBuildingInfo(t);
	sf::FloatRect fp(static_cast<float>(tx * tileSize), static_cast<float>(ty * tileSize), static_cast<float>(info.tilesW * tileSize), static_cast<float>(info.tilesH * tileSize));
	tileMap& m = world();
	for (int y = ty; y < ty + info.tilesH; y++){
		for (int x = tx; x < tx + info.tilesW; x++){
			unsigned char id = m.getTileAt(x, y);
			if (id == T_WATER || id == T_HILL) return false;          // can not build on water / hills
			if (m.getFlow(x, y).x != 0.0f || m.getFlow(x, y).y != 0.0f) return false;
		}
	}
	for (auto& e : get_entities()){
		if (e->isDead()) continue;
		int c = e->getClass();
		if (c == EC_ITEM || c == EC_WORKER) continue;
		if (c == EC_SELL || c == EC_GATE){
			sf::FloatRect a = (c == EC_SELL) ? static_cast<sellPoint*>(e.get())->area : static_cast<sceneChangeArea*>(e.get())->area;
			if (a.intersects(fp)) return false;
			continue;
		}
		if (c == EC_OBJECTIVE && static_cast<objectiveBuilding*>(e.get())->dropArea.intersects(fp)) return false;
		if (c == EC_BUILDING){
			if (static_cast<functionalBuilding*>(e.get())->footprint().intersects(fp)) return false;
			continue;
		}
		if (e->isSolid()){ if (e->getHitbox().intersects(fp)) return false; }
		else if (fp.contains(e->getPosition()) || fp.contains(e->getPosition() - sf::Vector2f(0.0f, 1.0f))) return false; // stumps
	}
	bool solidBuilding = !(t == B_CANAL || t == B_CONVEYOR || t == B_CAMPFIRE);
	if (solidBuilding && thePlayer && fp.intersects(thePlayer->getHitbox())) return false;
	return true;
}

void GameScene::tryPlace()
{
	if (placing < 0) return;
	buildingType t = static_cast<buildingType>(placing);
	sf::Vector2i o = placementTile(t);
	const buildingInfo& info = getBuildingInfo(t);
	if (!canPlace(t, o.x, o.y)) { toast("You can not build here.", 1.5f); return; }
	if (!decrementMoney(info.cost)) { toast("Not enough money for a " + std::string(info.name) + " (" + std::to_string(info.cost) + ").", 2.5f); placing = -1; return; }
	std::shared_ptr<functionalBuilding> b;
	if (t == B_CANAL || t == B_CONVEYOR) b = std::make_shared<itemMovementBuilding>(t, o.x, o.y, placeDir);
	else if (t == B_SAWMILL || t == B_CHIPPER || t == B_KILN) b = std::make_shared<productionBuilding>(t, o.x, o.y, placeDir);
	else b = std::make_shared<functionalBuilding>(t, o.x, o.y, placeDir);
	addEntity(b);
	if (money < info.cost) placing = -1;                         // can not afford another one
}

// =====================================================================================================================
//  per-frame logic
// =====================================================================================================================
void GameScene::updatePlayer(float dt)
{
	sf::Vector2f vel = frame.move;
	if (frame.sprint) { vel.x *= 1.7f; vel.y *= 1.7f; }
	thePlayer->setVelocityPublic(vel);
	if (vel.x != 0.0f || vel.y != 0.0f){
		thePlayer->updateDirection(vel);
		thePlayer->moveWithCollision(sf::Vector2f(vel.x * dt, vel.y * dt), *this);
		footstepTimer -= dt;
		if (footstepTimer <= 0.0f) { playSound(SND_FOOTSTEP); footstepTimer = frame.sprint ? 0.28f : 0.4f; }
	}
	thePlayer->update(dt, *this);
}

void GameScene::updateVehicle(float dt)
{
	float throttle = -frame.move.y / maxSpeed;   // W = forward, S = reverse (design: A and D turn the truck)
	float steer = frame.move.x / maxSpeed;
	theVehicle->occupied = true;
	theVehicle->drive(throttle, steer, dt, *this);
	sf::Vector2f vp = theVehicle->getPosition();
	thePlayer->updatePos(vp.x, vp.y);
	thePlayer->setVelocityPublic(sf::Vector2f(0.0f, 0.0f));
	int n = 0;
	for (auto& c : truckCargo) { c->updatePos(vp.x, vp.y); n++; }
}

void GameScene::updateCold(float dt)
{
	sf::Vector2i t = world().getTileOnScreen(thePlayer->getPosition() - sf::Vector2f(0.0f, 2.0f));
	bool cold = world().isCold(t.x, t.y);
	bool warm = isNearHeat(thePlayer->getPosition());
	if (cold && !warm){
		thePlayer->frost = std::min(100.0f, thePlayer->frost + 3.5f * frostFactor() * dt);   // "increase frost buildup"
		if (thePlayer->frost >= 100.0f) thePlayer->health -= 2.5f * frostFactor() * dt;      // "max frost: reduce the player's health"
	}else{
		thePlayer->frost = std::max(0.0f, thePlayer->frost - (warm ? 14.0f : 6.0f) * dt);   // near a heat source: reduce frost
		if (thePlayer->frost <= 0.0f || warm) thePlayer->health = std::min(100.0f, thePlayer->health + 0.6f * dt); // very slight healing
	}
	if (thePlayer->health <= 0.0f){                                                          // you froze: wake up at the level start
		thePlayer->health = 50.0f; thePlayer->frost = 0.0f;
		int lost = money / 10; money -= lost;
		thePlayer->inVehicle = false; if (theVehicle) theVehicle->occupied = false;
		thePlayer->updatePos(levels[currentLevel].spawn.x, levels[currentLevel].spawn.y);
		toast("You froze! You woke up at the camp and lost " + std::to_string(lost) + " coins. Stay near fires in the snow.", 6.0f);
	}
}

void GameScene::updateTrees(float dt)
{
	treeTimer += dt;
	if (treeTimer < 6.0f) return;
	treeTimer = 0.0f;
	if (treeCount(currentLevel) >= treeTargetForLevel(currentLevel)) return;     // flowchart: "Enough trees?" yes -> carry on
	tileMap& m = world();
	std::uniform_int_distribution<int> dx(3, m.width() - 4), dy(3, m.height() - 4);
	for (int tries = 0; tries < 30; tries++){                                    // no -> "Generate new tree"
		int tx = dx(gameRng), ty = dy(gameRng);
		sf::Vector2f p(tx * tileSize + 8.0f, ty * tileSize + 14.0f);
		sf::Vector2f pp = thePlayer->getPosition();
		if (std::fabs(pp.x - p.x) < 240.0f && std::fabs(pp.y - p.y) < 135.0f) continue; // never grow trees in front of the player
		if (isTreeSpot(*this, currentLevel, tx, ty)) { spawnTreeAt(currentLevel, tx, ty); break; }
	}
}

void GameScene::cleanDead()
{
	auto& ents = get_entities();
	ents.erase(std::remove_if(ents.begin(), ents.end(), [](const std::shared_ptr<entity>& e) { return e->isDead(); }), ents.end());
}

void GameScene::update(float dt)
{
	if (!built || !thePlayer || levels.empty()) return;
	if (dt > 0.1f) dt = 0.1f;

	// ---- controller cursor (right stick)
	bool pad = inputControllerConnected();
	if (pad){
		cursorUi.x += (inputStrength("Ui Right") - inputStrength("Ui Left")) * 700.0f * dt * (layout.width / 1280.0f);
		cursorUi.y += (inputStrength("Ui Down") - inputStrength("Ui Up")) * 700.0f * dt * (layout.width / 1280.0f);
		cursorUi.x = std::max(0.0f, std::min(static_cast<float>(layout.width), cursorUi.x));
		cursorUi.y = std::max(0.0f, std::min(static_cast<float>(layout.height), cursorUi.y));
	}
	usingCursor = pad;
	sf::Vector2f ptr = pointerUi();
	bool clicked = frame.mouseClicked || (pad && inputJustPressed("Select"));

	// ---- menus + hot keys
	if (inputJustPressed("Escape")){
		if (placing >= 0) placing = -1;
		else if (panel == PANEL_PAUSE) closePanels();
		else if (panel != PANEL_NONE && panel != PANEL_VICTORY) closePanels();
		else if (panel == PANEL_NONE) openPanel(PANEL_PAUSE);
	}
	if (panel != PANEL_PAUSE && panel != PANEL_VICTORY){
		if (inputJustPressed("Build Menu")) { if (panel == PANEL_BUILD) closePanels(); else openPanel(PANEL_BUILD); }
		if (inputJustPressed("Shop Menu")) { if (panel == PANEL_SHOP) closePanels(); else openPanel(PANEL_SHOP); }
		if (inputJustPressed("Worker Menu")) { if (panel == PANEL_WORKERS) closePanels(); else openPanel(PANEL_WORKERS); }
	}

	// ---- UI (HUD buttons + the open panel). Button clicks are queued in "deferred" and run after the UI loop.
	refreshHud();
	refreshPanels();
	if (panel == PANEL_PAUSE) pausePanel.update(dt, ptr.x, ptr.y, clicked);
	else if (panel == PANEL_VICTORY) victoryPanel.update(dt, ptr.x, ptr.y, clicked);
	else{
		hud.update(dt, ptr.x, ptr.y, clicked);
		if (panel == PANEL_BUILD) buildPanel.update(dt, ptr.x, ptr.y, clicked);
		else if (panel == PANEL_SHOP) shopPanel.update(dt, ptr.x, ptr.y, clicked);
		else if (panel == PANEL_WORKERS) workerPanel.update(dt, ptr.x, ptr.y, clicked);
	}
	if (deferred){
		std::function<void()> action = deferred;
		deferred = nullptr;
		action();
		clicked = false;                    // the click was used by the UI
		frame.mouseClicked = false;
	}
	if (toastTimer > 0.0f) toastTimer -= dt;
	if (gateCooldown > 0.0f) gateCooldown -= dt;
	if (panel == PANEL_PAUSE || panel == PANEL_VICTORY) return;   // the world is paused

	// ---- hotbar (tools 1-3)
	for (int i = 1; i <= 3; i++){
		if (inputJustPressed("Hotbar " + std::to_string(i)) && thePlayer->tools[i - 1]) thePlayer->currentHotbarSlot = i;
	}
	if (inputJustPressed("Hotbar Next") || inputJustPressed("Hotbar Prev")){
		int dir = inputJustPressed("Hotbar Next") ? 1 : -1;
		for (int k = 1; k <= 3; k++){
			int c = ((thePlayer->currentHotbarSlot - 1 + dir * k) % 3 + 3) % 3;
			if (thePlayer->tools[c]) { thePlayer->currentHotbarSlot = c + 1; break; }
		}
	}

	// ---- building placement
	if (placing >= 0){
		if (inputJustPressed("Rotate")) placeDir = (placeDir + 1) % 4;
		if (frame.rightClicked) placing = -1;
		bool onHud = ptr.y < 60.0f || ptr.y > 640.0f;
		if ((clicked && !onHud) || inputJustPressed("Pick Up")) tryPlace();
	}

	// ---- actions
	moverCache.clear();
	for (auto& e : get_entities()){
		if (e->getClass() != EC_BUILDING || e->isDead()) continue;
		functionalBuilding* b = static_cast<functionalBuilding*>(e.get());
		if (b->type == B_CANAL || b->type == B_CONVEYOR) moverCache.push_back(static_cast<itemMovementBuilding*>(b));
	}
	if (placing < 0){
		if (inputHeld("Interact")) handleInteract();
		if (inputJustPressed("Pick Up")) handlePickup();
		if (inputJustPressed("Drop")) handleDrop();
	}
	if (thePlayer->inVehicle) updateVehicle(dt); else updatePlayer(dt);
	if (theVehicle && vehicleLevel == currentLevel && !thePlayer->inVehicle) theVehicle->update(dt, *this);

	// ---- every entity (items float, sell points sell, buildings produce, workers walk ...)
	{
		auto& ents = get_entities();
		for (size_t i = 0; i < ents.size(); i++) ents[i]->update(dt, *this);   // index loop: entities may add items while updating
	}
	cleanDead();
	updateCold(dt);
	updateTrees(dt);

	// ---- level change requested by a gate
	if (pendingLevel >= 0){
		int lvl = pendingLevel; pendingLevel = -1;
		if (placing >= 0) placing = -1;
		currentLevel = lvl;
		thePlayer->updatePos(pendingSpawn.x, pendingSpawn.y);
		if (theVehicle && thePlayer->inVehicle){ theVehicle->updatePos(pendingSpawn.x, pendingSpawn.y); vehicleLevel = lvl; }
		else if (theVehicle && vehicleLevel == lvl) { /* the truck stays where it was parked */ }
		gateCooldown = 2.0f;
		cameraCentre = thePlayer->getPosition();
		toast("Welcome to " + levels[lvl].name + (levels[lvl].map.isCold(pendingSpawn.x / tileSize, pendingSpawn.y / tileSize) ? " - it is cold here, stay near fires!" : "."), 5.0f);
	}

	// ---- camera follows the player (the world moves around the player, who stays in the middle of the display)
	{
		sf::Vector2f target = thePlayer->getPosition() - sf::Vector2f(0.0f, 8.0f);
		sf::Vector2f px = world().pixelSize();
		float hw = 240.0f, hh = 135.0f;
		target.x = std::max(hw, std::min(px.x - hw, target.x));
		target.y = std::max(hh, std::min(px.y - hh, target.y));
		cameraCentre = target;
	}

	// ---- autosave
	autosaveTimer += dt;
	if (autosaveTimer > 90.0f) { autosaveTimer = 0.0f; saveGame(); }
}
