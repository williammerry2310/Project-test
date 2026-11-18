// objects.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026

#include <string>
#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>

#include "objects.h"
#include "scene.h"
#include "globals.h"

// =====================================================================================================================
//  tables + small drawing helpers
// =====================================================================================================================
static const buildingInfo buildingTable[B_TYPE_COUNT] = {
	{"Canal",          20, 1, 1},
	{"Conveyor",       60, 1, 1},
	{"Sawmill",       450, 3, 2},
	{"Chipper",       350, 2, 2},
	{"Kiln",          500, 2, 2},
	{"Worker Housing",250, 3, 2},
	{"Campfire",       40, 1, 1}
};
const buildingInfo& getBuildingInfo(buildingType t) { return buildingTable[t]; }

static const char* itemNames[ITEM_TYPE_COUNT] = {"Spruce Log", "Pine Log", "Oak Log", "Frost Log", "Plank", "Woodchips"};
const char* itemName(itemType t) { return itemNames[t]; }
bool itemIsLog(itemType t) { return t <= ITEM_LOG_FROST; }
float itemBasePrice(itemType t)
{
	static const float prices[ITEM_TYPE_COUNT] = {8.0f, 10.0f, 30.0f, 60.0f, 22.0f, 4.0f};
	return prices[t];
}
itemType logForTree(treeType t) { return static_cast<itemType>(t); }
int treeMinAxeTier(treeType t) { static const int tiers[TREE_TYPE_COUNT] = {0, 0, 1, 2}; return tiers[t]; }
float treeMaxHealth(treeType t) { static const float hp[TREE_TYPE_COUNT] = {60.0f, 70.0f, 160.0f, 300.0f}; return hp[t]; }

static sf::Vector2f dirFromIndex(int d)
{
	switch (((d % 4) + 4) % 4){
		case 0: return sf::Vector2f(1.0f, 0.0f);
		case 1: return sf::Vector2f(0.0f, 1.0f);
		case 2: return sf::Vector2f(-1.0f, 0.0f);
		default: return sf::Vector2f(0.0f, -1.0f);
	}
}

static void drawBox(sf::RenderWindow& w, float x, float y, float width, float height, sf::Color fill, sf::Color outline = sf::Color(0, 0, 0, 0), float outlineThickness = 0.0f)
{
	sf::RectangleShape r(sf::Vector2f(width, height));
	r.setPosition(x, y);
	r.setFillColor(fill);
	if (outlineThickness > 0.0f){ r.setOutlineColor(outline); r.setOutlineThickness(outlineThickness); }
	w.draw(r);
}

// the clipped-corner "canopy" shape used in the mock-ups
static void drawCanopy(sf::RenderWindow& w, float cx, float bottom, float width, float height, sf::Color col)
{
	float cut = std::min(width, height) * 0.22f;
	float l = cx - width / 2.0f, r = cx + width / 2.0f, t = bottom - height, b = bottom;
	sf::ConvexShape s(6);
	s.setPoint(0, sf::Vector2f(l + cut, t)); s.setPoint(1, sf::Vector2f(r - cut, t));
	s.setPoint(2, sf::Vector2f(r, t + cut)); s.setPoint(3, sf::Vector2f(r, b));
	s.setPoint(4, sf::Vector2f(l, b));       s.setPoint(5, sf::Vector2f(l, t + cut));
	s.setFillColor(col);
	s.setOutlineColor(sf::Color(0, 0, 0, 90));
	s.setOutlineThickness(0.5f);
	w.draw(s);
}

static sf::Color itemColour(itemType t)
{
	switch (t){
		case ITEM_LOG_SPRUCE: return sf::Color(0x8b, 0x5a, 0x2b);
		case ITEM_LOG_PINE:   return sf::Color(0xb8, 0x77, 0x3a);
		case ITEM_LOG_OAK:    return sf::Color(0x6b, 0x3e, 0x1a);
		case ITEM_LOG_FROST:  return sf::Color(0x9f, 0xc5, 0xe8);
		case ITEM_PLANK:      return sf::Color(0xe6, 0xc2, 0x8a);
		default:              return sf::Color(0xd9, 0xb8, 0x70);
	}
}

static void drawTri(sf::RenderWindow& w, sf::Vector2f c, sf::Vector2f d, sf::Color col)
{
	sf::ConvexShape a(3);
	a.setPoint(0, sf::Vector2f(c.x + d.x * 4.0f, c.y + d.y * 4.0f));
	a.setPoint(1, sf::Vector2f(c.x - d.x * 3.0f - d.y * 3.0f, c.y - d.y * 3.0f + d.x * 3.0f));
	a.setPoint(2, sf::Vector2f(c.x - d.x * 3.0f + d.y * 3.0f, c.y - d.y * 3.0f - d.x * 3.0f));
	a.setFillColor(col);
	w.draw(a);
}

static void drawRoofedBuilding(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color roofCol)
{
	sf::Color line(0x21, 0x1f, 0x34);
	drawBox(window, x - w / 2.0f, y - h * 0.6f, w, h * 0.6f, sf::Color(0xd8, 0xa0, 0x66), line, 0.8f);
	sf::ConvexShape r(4);
	r.setPoint(0, sf::Vector2f(x - w / 2.0f - 3.0f, y - h * 0.6f)); r.setPoint(1, sf::Vector2f(x + w / 2.0f + 3.0f, y - h * 0.6f));
	r.setPoint(2, sf::Vector2f(x + w / 2.0f - 6.0f, y - h)); r.setPoint(3, sf::Vector2f(x - w / 2.0f + 6.0f, y - h));
	r.setFillColor(roofCol); r.setOutlineColor(line); r.setOutlineThickness(0.8f);
	window.draw(r);
	for (int i = -1; i <= 1; i += 2) drawBox(window, x + i * w * 0.27f - 3.0f, y - h * 0.45f, 6.0f, 6.0f, sf::Color(0xcf, 0xe2, 0xf3), line, 0.5f);
	drawBox(window, x - 3.0f, y - 9.0f, 6.0f, 9.0f, sf::Color(0xed, 0xc3, 0x9a), line, 0.5f);
}

// =====================================================================================================================
//  entity
// =====================================================================================================================
entity::entity() : posX(0.0), posY(0.0) {}

void entity::updatePos(double x, double y) {
			posX = x;
			posY = y;
		}

sf::Vector2f entity::getPosition() const { return sf::Vector2f(static_cast<float>(posX), static_cast<float>(posY)); }

sf::FloatRect entity::getHitbox() const
{
	return sf::FloatRect(static_cast<float>(posX) - hitSize.x / 2.0f, static_cast<float>(posY) - hitSize.y, hitSize.x, hitSize.y);
}

// =====================================================================================================================
//  Item
// =====================================================================================================================
Item::Item(itemType t, double x, double y) : type(t)
{
	posX = x; posY = y;
	hitSize = sf::Vector2f(10.0f, 6.0f);
	eclass = EC_ITEM;
}

float Item::value() const
{
	float v = itemBasePrice(type);
	if (damp) v *= 0.85f;   // logs that came down a canal are damp: slightly lower price (design)
	if (dried) v *= 1.25f;  // kiln dried: higher price
	return v;
}

void Item::update(float dt, GameScene& scene)
{
	if (pickedUp) return;
	if (ignoreTimer > 0.0f) ignoreTimer -= dt;
	sf::Vector2f dir; float speed = 0.0f; bool makesDamp = false;
	if (scene.driftAt(getPosition(), dir, speed, makesDamp)){
		posX += dir.x * speed * dt;
		posY += dir.y * speed * dt;
		if (makesDamp && itemIsLog(type)) damp = true;
	}
}

void Item::draw(sf::RenderWindow& window) const
{
	if (pickedUp) return;
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	sf::Color c = itemColour(type);
	if (damp) c = sf::Color(static_cast<sf::Uint8>(c.r * 0.75f), static_cast<sf::Uint8>(c.g * 0.8f), static_cast<sf::Uint8>(std::min(255.0f, c.b * 0.9f + 20.0f)));
	if (itemIsLog(type)){          // "Brown box with outline" (media list)
		drawBox(window, x - 5.0f, y - 6.0f, 10.0f, 6.0f, c, sf::Color(0x2b, 0x17, 0x08), 0.7f);
		drawBox(window, x - 5.0f, y - 4.0f, 1.5f, 2.0f, sf::Color(0xd9, 0xb0, 0x80)); // cut end
	}else if (type == ITEM_PLANK){
		drawBox(window, x - 6.0f, y - 4.0f, 12.0f, 3.0f, dried ? sf::Color(0xf5, 0xdd, 0xaa) : c, sf::Color(0x6b, 0x4a, 0x20), 0.6f);
	}else{
		for (int i = 0; i < 4; i++) drawBox(window, x - 4.0f + (i % 2) * 4.0f, y - 4.0f + (i / 2) * 2.5f, 2.5f, 2.0f, c);
	}
}

std::string Item::serialize() const
{
	std::ostringstream o;
	o << "I " << static_cast<int>(type) << " " << posX << " " << posY << " " << (damp ? 1 : 0) << " " << (dried ? 1 : 0);
	return o.str();
}

// =====================================================================================================================
//  tree / stump / bush
// =====================================================================================================================
tree::tree(treeType t, double x, double y, float healthMult) : type(t)
{
	posX = x; posY = y;
	health = treeMaxHealth(t) * healthMult;
	hitSize = sf::Vector2f(8.0f, 6.0f);
	solid = true;
	eclass = EC_TREE;
}

bool tree::damage(float amount, int axeTier)
{
	if (axeTier < treeMinAxeTier(type)) return false; // not strong enough: stronger trees require better axes (design rule)
	health -= amount;
	if (health <= 0.0f){ fell(); return true; }       // health reaches 0: the tree falls and drops logs
	return false;
}

void tree::fell() { felled = true; dead = true; }

void tree::draw(sf::RenderWindow& window) const
{
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	float cw = 22.0f, ch = 22.0f, trunkH = 14.0f; sf::Color col = palette::canopy;
	switch (type){
		case TREE_SPRUCE: cw = 22.0f; ch = 22.0f; col = palette::canopy; break;             // #3d9072 from the mock-up
		case TREE_PINE:   cw = 17.0f; ch = 27.0f; col = sf::Color(0x2f, 0x7a, 0x5f); trunkH = 16.0f; break;
		case TREE_OAK:    cw = 28.0f; ch = 22.0f; col = sf::Color(0x6a, 0xa8, 0x4f); trunkH = 13.0f; break;
		case TREE_FROST:  cw = 24.0f; ch = 28.0f; col = sf::Color(0x9f, 0xd3, 0xe6); trunkH = 15.0f; break;
		default: break;
	}
	drawBox(window, x - 3.0f, y - trunkH, 6.0f, trunkH, palette::trunk, sf::Color(0, 0, 0, 80), 0.5f);
	drawCanopy(window, x, y - trunkH + 6.0f, cw, ch, col);
}

std::string tree::serialize() const
{
	std::ostringstream o;
	o << "T " << static_cast<int>(type) << " " << posX << " " << posY << " " << health;
	return o.str();
}

stump::stump(double x, double y, float rot) : rotTime(rot)
{
	posX = x; posY = y;
	eclass = EC_STUMP;
}

void stump::update(float dt, GameScene& scene)
{
	rotTime -= dt;
	if (rotTime <= 0.0f) dead = true; // rotted away, the space is free for a new tree
}

void stump::draw(sf::RenderWindow& window) const
{
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	sf::ConvexShape s(4);
	s.setPoint(0, sf::Vector2f(x - 4.0f, y - 6.0f)); s.setPoint(1, sf::Vector2f(x + 4.0f, y - 6.0f));
	s.setPoint(2, sf::Vector2f(x + 6.0f, y));         s.setPoint(3, sf::Vector2f(x - 6.0f, y));
	s.setFillColor(sf::Color(0x7f, 0x40, 0x06));
	s.setOutlineColor(sf::Color(0, 0, 0, 90)); s.setOutlineThickness(0.5f);
	window.draw(s);
}

std::string stump::serialize() const
{
	std::ostringstream o;
	o << "S " << posX << " " << posY << " " << rotTime;
	return o.str();
}

bush::bush(double x, double y)
{
	posX = x; posY = y;
	hitSize = sf::Vector2f(10.0f, 5.0f);
	solid = true;
	eclass = EC_BUSH;
}

void bush::draw(sf::RenderWindow& window) const
{
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	sf::CircleShape c(7.0f, 16);
	c.setScale(1.0f, 0.75f);
	c.setPosition(x - 7.0f, y - 10.0f);
	c.setFillColor(palette::bush);
	window.draw(c);
	drawBox(window, x - 7.0f, y - 5.0f, 14.0f, 5.0f, palette::bush);
}

std::string bush::serialize() const { return ""; } // bushes are part of the premade map, not saved

// =====================================================================================================================
//  sellPoint
// =====================================================================================================================
sellPoint::sellPoint(const std::string& sellName, sf::FloatRect sellArea, sf::Color col) : name(sellName), area(sellArea), colour(col)
{
	for (int i = 0; i < ITEM_TYPE_COUNT; i++){ basePrice[i] = 0.0f; priceMult[i] = 1.0f; }
	posX = area.left + area.width / 2.0f;
	posY = area.top + area.height;
	eclass = EC_SELL;
}

void sellPoint::update(float dt, GameScene& scene)
{
	for (int i = 0; i < ITEM_TYPE_COUNT; i++){                // prices slowly recover when nothing is sold
		if (priceMult[i] < 1.0f) priceMult[i] = std::min(1.0f, priceMult[i] + 0.002f * dt);
	}
	for (auto& e : scene.get_entities()){
		if (e->getClass() != EC_ITEM || e->isDead()) continue;
		Item* it = static_cast<Item*>(e.get());
		if (it->pickedUp) continue;
		if (!area.contains(e->getPosition())) continue;
		if (basePrice[it->type] <= 0.0f) continue;       // this buyer does not want it
		float price = basePrice[it->type] * priceMult[it->type] * scene.priceFactor();
		if (it->damp) price *= 0.85f;
		if (it->dried) price *= 1.25f;
		int coins = std::max(1, static_cast<int>(std::lround(price)));
		scene.incrementMoney(coins);
		priceMult[it->type] = std::max(0.25f, priceMult[it->type] * 0.96f); // selling the same thing again and again lowers the price (design: conflict)
		it->destroy();
		scene.toast("+" + std::to_string(coins) + " coins  (" + std::string(itemName(it->type)) + " sold at " + name + ")", 2.0f);
	}
}

void sellPoint::draw(sf::RenderWindow& window) const
{
	sf::RectangleShape r(sf::Vector2f(area.width, area.height));
	r.setPosition(area.left, area.top);
	r.setFillColor(sf::Color(colour.r, colour.g, colour.b, 70));
	r.setOutlineColor(sf::Color(colour.r, colour.g, colour.b, 220));
	r.setOutlineThickness(1.0f);
	window.draw(r);
	float cx = area.left + area.width / 2.0f;
	drawBox(window, cx - 12.0f, area.top + 2.0f, 24.0f, 10.0f, sf::Color(0xd8, 0xa0, 0x66), sf::Color(0x21, 0x1f, 0x34), 0.6f); // a little market stall
	drawBox(window, cx - 14.0f, area.top - 3.0f, 28.0f, 5.0f, colour, sf::Color(0x21, 0x1f, 0x34), 0.6f);
}

// =====================================================================================================================
//  objectiveBuilding
// =====================================================================================================================
objectiveBuilding::objectiveBuilding(int objId, const std::string& objName, sf::Vector2f bottomCentre, sf::Vector2f buildingSize)
	: objectiveId(objId), name(objName), size(buildingSize)
{
	posX = bottomCentre.x; posY = bottomCentre.y;
	hitSize = sf::Vector2f(buildingSize.x, buildingSize.y * 0.5f);
	solid = true;
	eclass = EC_OBJECTIVE;
	dropArea = sf::FloatRect(bottomCentre.x - buildingSize.x / 2.0f - 20.0f, bottomCentre.y + 2.0f, buildingSize.x + 40.0f, 34.0f);
}

bool objectiveBuilding::isComplete() const
{
	for (auto& r : required){
		auto d = delivered.find(r.first);
		int have = (d == delivered.end()) ? 0 : d->second;
		if (have < r.second) return false;
	}
	return true;
}

std::string objectiveBuilding::progressText() const
{
	std::string s = name + (built ? " - BUILT" : " - needs:");
	if (!built){
		for (auto& r : required){
			auto d = delivered.find(r.first);
			int have = (d == delivered.end()) ? 0 : d->second;
			s += "\n  " + std::string(itemName(static_cast<itemType>(r.first))) + "  " + std::to_string(have) + "/" + std::to_string(r.second);
		}
	}
	return s;
}

void objectiveBuilding::update(float dt, GameScene& scene)
{
	if (built) return;
	for (auto& e : scene.get_entities()){
		if (e->getClass() != EC_ITEM || e->isDead()) continue;
		Item* it = static_cast<Item*>(e.get());
		if (it->pickedUp || !dropArea.contains(e->getPosition())) continue;
		int key = static_cast<int>(it->type);
		auto req = required.find(key);
		if (req == required.end()) continue;               // not needed by this building
		if (delivered[key] >= req->second) continue;       // already have enough of that
		delivered[key] += 1;                               // flowchart: remove the resource and increment progress to the objective
		it->destroy();
		scene.toast(std::string(itemName(it->type)) + " delivered to " + name, 2.0f);
	}
	if (isComplete()){
		built = true;
		scene.onObjectiveBuilt(*this, false);
	}
}

void objectiveBuilding::draw(sf::RenderWindow& window) const
{
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	float w = size.x, h = size.y;
	sf::RectangleShape zone(sf::Vector2f(dropArea.width, dropArea.height));
	zone.setPosition(dropArea.left, dropArea.top);
	zone.setFillColor(sf::Color(255, 255, 255, built ? 0 : 40));
	zone.setOutlineColor(sf::Color(255, 255, 255, built ? 0 : 160));
	zone.setOutlineThickness(0.7f);
	window.draw(zone);
	if (objectiveId == 2) drawBox(window, x - w * 0.9f, y + 36.0f, w * 1.8f, 10.0f, sf::Color(0x55, 0x55, 0x55), sf::Color(0x21, 0x1f, 0x34), 0.6f); // runway
	if (objectiveId == 1) for (int i = -4; i <= 4; i++) drawBox(window, x + i * 14.0f - 1.0f, y + 38.0f, 2.0f, 8.0f, sf::Color(0x59, 0x56, 0x52)); // rail sleepers
	if (!built){   // scaffolding
		sf::Color wood(0x7a, 0x3f, 0x04);
		drawBox(window, x - w / 2.0f, y - h * 0.6f, w, h * 0.6f, sf::Color(0xd8, 0xa0, 0x66, 70), wood, 1.0f);
		drawBox(window, x - w / 2.0f, y - h * 0.3f, w, 1.5f, wood);
		drawBox(window, x - w / 4.0f, y - h * 0.6f, 1.5f, h * 0.6f, wood);
		drawBox(window, x + w / 4.0f, y - h * 0.6f, 1.5f, h * 0.6f, wood);
	}else{
		sf::Color roof(0x59, 0x56, 0x52);
		if (objectiveId == 0) roof = sf::Color(0x3d, 0x85, 0xc6);
		if (objectiveId == 1) roof = sf::Color(0xb4, 0x5f, 0x06);
		drawRoofedBuilding(window, x, y, w, h, roof);
		if (objectiveId == 2) drawBox(window, x + w / 2.0f - 8.0f, y - h * 1.4f, 8.0f, h * 0.8f, sf::Color(0x99, 0x99, 0x99), sf::Color(0x21, 0x1f, 0x34), 0.6f); // control tower
	}
}

std::string objectiveBuilding::serialize() const
{
	std::ostringstream o;
	o << "O " << objectiveId << " " << (built ? 1 : 0);
	for (int i = 0; i < ITEM_TYPE_COUNT; i++){
		auto d = delivered.find(i);
		o << " " << (d == delivered.end() ? 0 : d->second);
	}
	return o.str();
}

// =====================================================================================================================
//  sceneChangeArea
// =====================================================================================================================
sceneChangeArea::sceneChangeArea(sf::FloatRect a, int level, sf::Vector2f spawn, const std::string& gateName)
	: area(a), targetLevel(level), targetSpawn(spawn), name(gateName)
{
	posX = a.left + a.width / 2.0f; posY = a.top + a.height;
	eclass = EC_GATE;
}

void sceneChangeArea::update(float dt, GameScene& scene)
{
	if (!active || scene.gateCooldown > 0.0f || !scene.thePlayer) return;
	sf::Vector2f p = scene.thePlayer->getPosition();
	if (scene.thePlayer->inVehicle && scene.theVehicle) p = scene.theVehicle->getPosition();
	if (area.contains(p)) scene.changeLevel(targetLevel, targetSpawn); // "when the player runs into the sceneChangeArea the scene changes"
}

void sceneChangeArea::draw(sf::RenderWindow& window) const
{
	if (!active) return;
	sf::RectangleShape r(sf::Vector2f(area.width, area.height));
	r.setPosition(area.left, area.top);
	r.setFillColor(sf::Color(255, 255, 255, 45));
	r.setOutlineColor(sf::Color(255, 255, 255, 150));
	r.setOutlineThickness(0.8f);
	window.draw(r);
}

// =====================================================================================================================
//  storeBuilding
// =====================================================================================================================
storeBuilding::storeBuilding(storeType t, const std::string& storeName, sf::Vector2f bottomCentre, sf::Vector2f buildingSize)
	: type(t), name(storeName), size(buildingSize)
{
	posX = bottomCentre.x; posY = bottomCentre.y;
	hitSize = sf::Vector2f(buildingSize.x, buildingSize.y * 0.5f);
	solid = true;
	eclass = EC_STORE;
}

void storeBuilding::draw(sf::RenderWindow& window) const
{
	sf::Color roof = (type == STORE_GENERAL) ? sf::Color(0x59, 0x56, 0x52) : sf::Color(0x3d, 0x85, 0xc6);
	drawRoofedBuilding(window, static_cast<float>(posX), static_cast<float>(posY), size.x, size.y, roof);
}

// =====================================================================================================================
//  buildings the player builds
// =====================================================================================================================
functionalBuilding::functionalBuilding(buildingType t, int tx, int ty, int direction) : type(t), dir(((direction % 4) + 4) % 4), tileOrigin(tx, ty)
{
	const buildingInfo& info = getBuildingInfo(t);
	tilesSize = sf::Vector2i(info.tilesW, info.tilesH);
	posX = (tx + info.tilesW / 2.0) * tileSize;
	posY = (ty + info.tilesH) * static_cast<double>(tileSize);
	hitSize = sf::Vector2f(static_cast<float>(info.tilesW * tileSize), static_cast<float>(info.tilesH * tileSize));
	solid = !(t == B_CANAL || t == B_CONVEYOR || t == B_CAMPFIRE); // belts / canals can be walked across
	eclass = EC_BUILDING;
}

sf::FloatRect functionalBuilding::footprint() const
{
	return sf::FloatRect(static_cast<float>(tileOrigin.x * tileSize), static_cast<float>(tileOrigin.y * tileSize),
	                     static_cast<float>(tilesSize.x * tileSize), static_cast<float>(tilesSize.y * tileSize));
}

sf::Vector2f functionalBuilding::dirVector() const { return dirFromIndex(dir); }

void functionalBuilding::draw(sf::RenderWindow& window) const
{
	sf::FloatRect f = footprint();
	sf::Color outline(0x21, 0x1f, 0x34);
	sf::Vector2f d = dirVector();
	float cx = f.left + f.width / 2.0f, cy = f.top + f.height / 2.0f;
	switch (type){
		case B_CANAL:
			drawBox(window, f.left, f.top, f.width, f.height, palette::water, sf::Color(0x2a, 0x56, 0xa8), 0.6f);
			drawTri(window, sf::Vector2f(cx, cy), d, sf::Color(0xb8, 0xd4, 0xff));
			break;
		case B_CONVEYOR:
			drawBox(window, f.left, f.top, f.width, f.height, sf::Color(0x55, 0x55, 0x58), outline, 0.6f);
			drawTri(window, sf::Vector2f(cx, cy), d, sf::Color(0xff, 0xd9, 0x66));
			break;
		case B_SAWMILL: case B_CHIPPER: case B_KILN: {
			sf::Color wall = (type == B_SAWMILL) ? sf::Color(0xd8, 0xa0, 0x66) : (type == B_CHIPPER ? sf::Color(0x99, 0x99, 0x99) : sf::Color(0xc0, 0x60, 0x3a));
			drawBox(window, f.left, f.top + 6.0f, f.width, f.height - 6.0f, wall, outline, 0.8f);
			drawBox(window, f.left - 2.0f, f.top, f.width + 4.0f, 8.0f, sf::Color(0x59, 0x56, 0x52), outline, 0.8f);
			sf::CircleShape blade(5.0f, 12); blade.setPosition(cx - 5.0f, cy - 1.0f);
			blade.setFillColor(type == B_KILN ? sf::Color(0xff, 0x99, 0x00) : sf::Color(0xcc, 0xcc, 0xcc)); blade.setOutlineColor(outline); blade.setOutlineThickness(0.6f);
			window.draw(blade);
			// the yellow square shows the side where the finished items come out
			drawBox(window, cx + d.x * (f.width / 2.0f - 2.0f) - 2.0f, cy + d.y * (f.height / 2.0f - 2.0f) - 2.0f, 4.0f, 4.0f, sf::Color(0xff, 0xd9, 0x66), outline, 0.5f);
			break; }
		case B_HOUSING: {
			drawBox(window, f.left, f.top + 8.0f, f.width, f.height - 8.0f, sf::Color(0xed, 0xc3, 0x9a), outline, 0.8f);
			sf::ConvexShape r(4);
			r.setPoint(0, sf::Vector2f(f.left - 3.0f, f.top + 9.0f)); r.setPoint(1, sf::Vector2f(f.left + f.width + 3.0f, f.top + 9.0f));
			r.setPoint(2, sf::Vector2f(f.left + f.width - 6.0f, f.top)); r.setPoint(3, sf::Vector2f(f.left + 6.0f, f.top));
			r.setFillColor(sf::Color(0xb4, 0x5f, 0x06)); r.setOutlineColor(outline); r.setOutlineThickness(0.8f);
			window.draw(r);
			drawBox(window, cx - 3.0f, f.top + f.height - 10.0f, 6.0f, 10.0f, sf::Color(0x7a, 0x3f, 0x04), outline, 0.5f);
			if (occupied) drawBox(window, f.left + 4.0f, f.top + 12.0f, 5.0f, 5.0f, sf::Color(0xff, 0xff, 0x99), outline, 0.4f);
			break; }
		case B_CAMPFIRE: {
			drawBox(window, cx - 5.0f, cy + 2.0f, 10.0f, 3.0f, sf::Color(0x7a, 0x3f, 0x04), outline, 0.4f);
			sf::CircleShape flame(4.0f, 10); flame.setPosition(cx - 4.0f, cy - 5.0f); flame.setFillColor(sf::Color(0xff, 0x80, 0x00)); window.draw(flame);
			sf::CircleShape core(2.0f, 8); core.setPosition(cx - 2.0f, cy - 3.0f); core.setFillColor(sf::Color(0xff, 0xe0, 0x66)); window.draw(core);
			break; }
		default: break;
	}
}

std::string functionalBuilding::serialize() const
{
	std::ostringstream o;
	o << "B " << static_cast<int>(type) << " " << tileOrigin.x << " " << tileOrigin.y << " " << dir << " " << (occupied ? 1 : 0);
	return o.str();
}

itemMovementBuilding::itemMovementBuilding(buildingType t, int tx, int ty, int direction) : functionalBuilding(t, tx, ty, direction)
{
	itemMoveSpeed = (t == B_CANAL) ? 24.0f : 12.0f; // canals are faster, conveyors slower (and more expensive) - design
}

productionBuilding::productionBuilding(buildingType t, int tx, int ty, int direction) : functionalBuilding(t, tx, ty, direction)
{
	productionSpeed = (t == B_SAWMILL) ? 4.0f : (t == B_CHIPPER ? 2.5f : 6.0f);
}

void productionBuilding::update(float dt, GameScene& scene)
{
	sf::FloatRect f = footprint();
	// 1. take in anything that arrived inside the building
	for (auto& e : scene.get_entities()){
		if (input.size() >= 8) break;
		if (e->getClass() != EC_ITEM || e->isDead()) continue;
		Item* it = static_cast<Item*>(e.get());
		if (it->pickedUp || !f.contains(e->getPosition())) continue;
		bool ok = false;
		if (type == B_SAWMILL || type == B_CHIPPER) ok = itemIsLog(it->type);
		if (type == B_KILN) ok = (itemIsLog(it->type) || it->type == ITEM_PLANK) && !it->dried;
		if (!ok) continue;
		input.push_back({it->type, it->damp, it->dried});
		it->destroy();
	}
	// 2. work on the first item in the queue
	if (input.empty()){ timer = 0.0f; return; }
	timer += dt;
	if (timer >= productionSpeed){
		timer = 0.0f;
		queued q = input.front();
		input.pop_front();
		produce(q, scene);
	}
}

void productionBuilding::produce(const queued& q, GameScene& scene)
{
	sf::FloatRect f = footprint();
	sf::Vector2f d = dirVector();
	sf::Vector2f out(f.left + f.width / 2.0f + d.x * (f.width / 2.0f + 6.0f), f.top + f.height / 2.0f + d.y * (f.height / 2.0f + 6.0f));
	if (type == B_SAWMILL){
		for (int i = 0; i < 2; i++) scene.spawnItem(ITEM_PLANK, out.x + i * 0.5f, out.y + 3.0f, q.damp, q.dried);
	}else if (type == B_CHIPPER){
		for (int i = 0; i < 4; i++) scene.spawnItem(ITEM_CHIP, out.x + (i % 2) * 1.0f, out.y + 3.0f, q.damp, q.dried);
	}else{ // kiln: the same item comes out dry (not damp any more) and worth more
		scene.spawnItem(q.type, out.x, out.y + 3.0f, false, true);
	}
}

// =====================================================================================================================
//  dynamicEntity
// =====================================================================================================================
void dynamicEntity::updateDirection(sf::Vector2f newDirection)
{
	float len = std::sqrt(newDirection.x * newDirection.x + newDirection.y * newDirection.y);
	if (len > 0.001f) direction = sf::Vector2f(newDirection.x / len, newDirection.y / len);
}

void dynamicEntity::updateVelocity(sf::Vector2f newVelocity) { velocity = newVelocity; }

void dynamicEntity::moveWithCollision(sf::Vector2f delta, GameScene& scene)
{
	if (delta.x != 0.0f){
		sf::FloatRect t = getHitbox(); t.left += delta.x;
		if (!scene.isBlocked(t, this)) posX += delta.x;
	}
	if (delta.y != 0.0f){
		sf::FloatRect t = getHitbox(); t.top += delta.y;
		if (!scene.isBlocked(t, this)) posY += delta.y;
	}
}

// =====================================================================================================================
//  player
// =====================================================================================================================
player::player(std::string playerName, double startX, double startY) : name(playerName)
{
	posX = startX;
	posY = startY;
	hitSize = sf::Vector2f(8.0f, 6.0f);
	eclass = EC_PLAYER;
}

void player::pickupItem(std::shared_ptr<Item> item)
{
	heldItem = item;
	if (item) item->pickedUp = true;
}

std::shared_ptr<Item> player::dropItem()
{
	std::shared_ptr<Item> out = heldItem;
	heldItem.reset();
	if (out) out->pickedUp = false;
	return out;
}

void player::update(float dt, GameScene& scene)
{
	if (chopCooldown > 0.0f) chopCooldown -= dt;
	if (swingTimer > 0.0f) swingTimer -= dt;
	float speedNow = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
	if (speedNow > 1.0f) walkTimer += dt * (6.0f + speedNow / 12.0f);
}

void player::draw(sf::RenderWindow& window) const
{
	if (inVehicle) return;
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	float face = (direction.x < -0.1f) ? -1.0f : 1.0f;
	bool moving = (std::fabs(velocity.x) + std::fabs(velocity.y)) > 1.0f;
	float step = moving ? std::sin(walkTimer) * 1.8f : 0.0f;
	sf::Color skin = palette::playerGrey;
	sf::Color line(0x33, 0x33, 0x33);
	// legs, body, head (the grey figure from the mock-up)
	drawBox(window, x - 3.5f, y - 5.0f - std::max(0.0f, step) * 0.3f, 3.0f, 5.0f, skin, line, 0.4f);
	drawBox(window, x + 0.5f, y - 5.0f - std::max(0.0f, -step) * 0.3f, 3.0f, 5.0f, skin, line, 0.4f);
	drawBox(window, x - 4.0f, y - 13.0f, 8.0f, 8.5f, skin, line, 0.4f);
	sf::CircleShape head(3.6f, 14); head.setPosition(x - 3.6f, y - 20.0f); head.setFillColor(skin); head.setOutlineColor(line); head.setOutlineThickness(0.4f);
	window.draw(head);
	// axe (handle + head) swings when chopping
	float angle = 35.0f;
	if (swingTimer > 0.0f) angle = 35.0f - (1.0f - swingTimer / 0.3f) * 110.0f;
	sf::RectangleShape handle(sf::Vector2f(2.0f, 10.0f));
	handle.setFillColor(palette::bridge);
	handle.setOrigin(1.0f, 10.0f);
	handle.setPosition(x + face * 4.0f, y - 8.0f);
	handle.setRotation(angle * face);
	window.draw(handle);
	sf::RectangleShape axeHead(sf::Vector2f(5.0f, 3.0f));
	axeHead.setFillColor(sf::Color(0xcc, 0xcc, 0xcc));
	axeHead.setOrigin(face > 0 ? 0.0f : 5.0f, 12.0f);
	axeHead.setPosition(x + face * 4.0f, y - 8.0f);
	axeHead.setRotation(angle * face);
	window.draw(axeHead);
	if (heldItem){ // one log carried in the arms
		drawBox(window, x - 5.0f, y - 12.0f, 10.0f, 5.0f, itemColour(heldItem->type), sf::Color(0x2b, 0x17, 0x08), 0.6f);
	}
}

// =====================================================================================================================
//  vehicle (the truck)
// =====================================================================================================================
vehicle::vehicle(double x, double y)
{
	posX = x; posY = y;
	hitSize = sf::Vector2f(16.0f, 10.0f);
	solid = false;
	eclass = EC_VEHICLE;
}

void vehicle::drive(float throttle, float steer, float dt, GameScene& scene)
{
	const float maxFwd = 95.0f, maxRev = -35.0f;
	if (throttle != 0.0f) speedNow += throttle * 90.0f * dt;
	else if (speedNow > 0.0f) speedNow = std::max(0.0f, speedNow - 70.0f * dt);
	else if (speedNow < 0.0f) speedNow = std::min(0.0f, speedNow + 70.0f * dt);
	speedNow = std::max(maxRev, std::min(maxFwd, speedNow));
	// A and D TURN the truck instead of strafing (design) - you can only turn while moving
	if (std::fabs(speedNow) > 3.0f){
		float sign = speedNow > 0.0f ? 1.0f : -1.0f;
		heading += steer * 120.0f * sign * std::min(1.0f, std::fabs(speedNow) / 45.0f) * dt;
	}
	while (heading < 0.0f) heading += 360.0f;
	while (heading >= 360.0f) heading -= 360.0f;
	float rad = heading * 3.14159265f / 180.0f;
	direction = sf::Vector2f(std::cos(rad), std::sin(rad));
	sf::Vector2f delta(direction.x * speedNow * dt, direction.y * speedNow * dt);
	double bx = posX, by = posY;
	moveWithCollision(delta, scene);
	float moved = static_cast<float>(std::fabs(posX - bx) + std::fabs(posY - by));
	if (moved < (std::fabs(delta.x) + std::fabs(delta.y)) * 0.3f) speedNow *= 0.3f; // hit something
	velocity = sf::Vector2f(static_cast<float>(posX - bx) / std::max(dt, 0.0001f), static_cast<float>(posY - by) / std::max(dt, 0.0001f));
}

void vehicle::update(float dt, GameScene& scene)
{
	if (!occupied) speedNow = 0.0f; // nobody driving: it stops
}

void vehicle::draw(sf::RenderWindow& window) const
{
	float snapped = std::round(heading / 22.5f) * 22.5f; // 16 directions
	float rad = snapped * 3.14159265f / 180.0f;
	sf::Vector2f d(std::cos(rad), std::sin(rad));
	float x = static_cast<float>(posX), y = static_cast<float>(posY) - 5.0f;
	sf::RectangleShape body(sf::Vector2f(22.0f, 11.0f));
	body.setOrigin(11.0f, 5.5f); body.setPosition(x, y); body.setRotation(snapped);
	body.setFillColor(sf::Color(0xcc, 0x41, 0x25)); body.setOutlineColor(sf::Color(0x21, 0x1f, 0x34)); body.setOutlineThickness(0.8f);
	window.draw(body);
	sf::RectangleShape cab(sf::Vector2f(8.0f, 9.0f));
	cab.setOrigin(4.0f, 4.5f); cab.setPosition(x + d.x * 6.0f, y + d.y * 6.0f); cab.setRotation(snapped);
	cab.setFillColor(sf::Color(0xcf, 0xe2, 0xf3)); cab.setOutlineColor(sf::Color(0x21, 0x1f, 0x34)); cab.setOutlineThickness(0.6f);
	window.draw(cab);
}

std::string vehicle::serialize() const
{
	std::ostringstream o;
	o << "V " << posX << " " << posY << " " << heading;
	return o.str();
}

// =====================================================================================================================
//  workerAI  (State pattern)
// =====================================================================================================================
static int nextWorkerId = 1000;

workerAI::workerAI(double x, double y)
{
	posX = x; posY = y;
	hitSize = sf::Vector2f(8.0f, 6.0f);
	eclass = EC_WORKER;
	id = nextWorkerId++;
	_state.reset(new OffState());
}

void workerAI::setState(std::unique_ptr<State> newState) { _pending = std::move(newState); }
void workerAI::setActive(bool active) { isActive = active; }

void workerAI::update(float dt, GameScene& scene)
{
	if (_state) _state->handle(*this, dt, scene);
	if (_pending){ _state = std::move(_pending); } // state switches happen after handle() returns, never while it is running
}

bool workerAI::followPath(float dt, GameScene& scene)
{
	if (pathIndex >= path.size()) return true;
	sf::Vector2f target(path[pathIndex].x * tileSize + tileSize / 2.0f, path[pathIndex].y * tileSize + tileSize / 2.0f + 3.0f);
	sf::Vector2f diff(target.x - static_cast<float>(posX), target.y - static_cast<float>(posY));
	float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
	float step = 48.0f * dt;
	if (dist <= step){
		posX = target.x; posY = target.y;
		pathIndex++;
		velocity = sf::Vector2f(0.0f, 0.0f);
	}else{
		posX += diff.x / dist * step; posY += diff.y / dist * step;
		updateDirection(diff);
		velocity = sf::Vector2f(diff.x / dist * 48.0f, diff.y / dist * 48.0f);
		walkTimer += dt * 10.0f;
	}
	return pathIndex >= path.size();
}

void workerAI::draw(sf::RenderWindow& window) const
{
	float x = static_cast<float>(posX), y = static_cast<float>(posY);
	float face = (direction.x < -0.1f) ? -1.0f : 1.0f;
	float step = std::sin(walkTimer) * 1.5f;
	sf::Color skin(0xe0, 0x9f, 0x3e); // "modified player sprite": the NPC is tinted orange
	sf::Color line(0x33, 0x33, 0x33);
	drawBox(window, x - 3.5f, y - 5.0f - std::max(0.0f, step) * 0.3f, 3.0f, 5.0f, skin, line, 0.4f);
	drawBox(window, x + 0.5f, y - 5.0f - std::max(0.0f, -step) * 0.3f, 3.0f, 5.0f, skin, line, 0.4f);
	drawBox(window, x - 4.0f, y - 13.0f, 8.0f, 8.5f, sf::Color(0xcc, 0x41, 0x25), line, 0.4f);
	sf::CircleShape head(3.6f, 14); head.setPosition(x - 3.6f, y - 20.0f); head.setFillColor(skin); head.setOutlineColor(line); head.setOutlineThickness(0.4f);
	window.draw(head);
	drawBox(window, x - 4.5f, y - 22.0f, 9.0f, 2.5f, sf::Color(0xff, 0xd9, 0x66), line, 0.3f); // hard hat
	if (holdingItem) drawBox(window, x + face * 2.0f - 5.0f, y - 12.0f, 10.0f, 5.0f, itemColour(heldType), sf::Color(0x2b, 0x17, 0x08), 0.6f);
}

std::string workerAI::serialize() const
{
	std::ostringstream o;
	o << "W " << posX << " " << posY << " " << depositIndex << " " << (isActive ? 1 : 0);
	return o.str();
}

// ---- states
static void releaseTarget(workerAI& w)
{
	if (w.target && w.target->reservedBy == w.getId()) w.target->reservedBy = 0;
	w.target.reset();
	w.path.clear();
	w.pathIndex = 0;
}

void OffState::handle(workerAI& worker, float dt, GameScene& scene)
{
	if (worker.isActive) worker.setState(std::unique_ptr<State>(new OnSearchingState())); // active == true -> searching
}

void OnSearchingState::handle(workerAI& worker, float dt, GameScene& scene)
{
	if (!worker.isActive){ worker.setState(std::unique_ptr<State>(new OffState())); return; }
	searchTime += dt;
	if (searchTime > 300.0f){ // searching for more than 5 minutes -> back to off
		worker.isActive = false;
		scene.toast("A worker found nothing to collect for 5 minutes and stopped.", 4.0f);
		worker.setState(std::unique_ptr<State>(new OffState()));
		return;
	}
	pollTimer += dt;
	if (pollTimer < 0.5f) return; // only look twice a second
	pollTimer = 0.0f;
	std::shared_ptr<Item> item = scene.findFreeLog(worker);               // checkForFreeItems
	if (!item) return;
	sf::Vector2i from = scene.world().getTileOnScreen(worker.getPosition() - sf::Vector2f(0.0f, 2.0f));
	sf::Vector2i to = scene.world().getTileOnScreen(item->getPosition() - sf::Vector2f(0.0f, 2.0f));
	std::vector<sf::Vector2i> p = findPathAStar(scene, from, to);
	if (p.empty()){ item->ignoreTimer = 20.0f; return; } // can not reach it, try again later
	item->reservedBy = worker.getId();
	worker.target = item;
	worker.path = p;
	worker.pathIndex = 0;
	worker.setState(std::unique_ptr<State>(new OnCollectionState()));
}

void OnCollectionState::handle(workerAI& worker, float dt, GameScene& scene)
{
	if (!worker.isActive){
		releaseTarget(worker);
		if (worker.holdingItem){ scene.spawnItem(worker.heldType, worker.getPosition().x, worker.getPosition().y, worker.heldDamp, worker.heldDried); worker.holdingItem = false; }
		worker.setState(std::unique_ptr<State>(new OffState()));
		return;
	}
	if (!worker.holdingItem){
		// the log got picked up by the player (or removed) before we got there -> search again
		if (!worker.target || worker.target->isDead() || worker.target->pickedUp){
			releaseTarget(worker);
			worker.setState(std::unique_ptr<State>(new OnSearchingState()));
			return;
		}
		bool arrived = worker.followPath(dt, scene);
		sf::Vector2f d = worker.target->getPosition() - worker.getPosition();
		if (arrived || std::sqrt(d.x * d.x + d.y * d.y) < 7.0f){
			worker.heldType = worker.target->type; worker.heldDamp = worker.target->damp; worker.heldDried = worker.target->dried;
			worker.target->destroy();                   // removed from the pool of entities
			worker.holdingItem = true;
			releaseTarget(worker);
			sf::Vector2f dep = scene.depositPosition(worker.depositIndex);
			sf::Vector2i from = scene.world().getTileOnScreen(worker.getPosition() - sf::Vector2f(0.0f, 2.0f));
			sf::Vector2i to = scene.world().getTileOnScreen(dep);
			worker.path = findPathAStar(scene, from, to);   // pathfindingAStar
			worker.pathIndex = 0;
			if (worker.path.empty()){ // can not reach the deposit: put it down and look again
				scene.spawnItem(worker.heldType, worker.getPosition().x, worker.getPosition().y, worker.heldDamp, worker.heldDried);
				worker.holdingItem = false;
				worker.setState(std::unique_ptr<State>(new OnSearchingState()));
			}
		}
	}else{
		if (worker.followPath(dt, scene)){
			sf::Vector2f dep = scene.depositPosition(worker.depositIndex);
			float jitter = static_cast<float>((scene.get_entities().size() * 7) % 9) - 4.0f;
			scene.spawnItem(worker.heldType, dep.x + jitter, dep.y, worker.heldDamp, worker.heldDried);
			worker.holdingItem = false;                   // delivered -> back to searching
			worker.setState(std::unique_ptr<State>(new OnSearchingState()));
		}
	}
}

// =====================================================================================================================
//  A* path finding on the tile grid (used by the workers)
// =====================================================================================================================
std::vector<sf::Vector2i> findPathAStar(GameScene& scene, sf::Vector2i from, sf::Vector2i to)
{
	tileMap& map = scene.world();
	int w = map.width(), h = map.height();
	std::vector<sf::Vector2i> result;
	if (from.x < 0 || from.y < 0 || from.x >= w || from.y >= h || to.x < 0 || to.y < 0 || to.x >= w || to.y >= h) return result;
	// blocked grid: solid tiles + the tiles under solid entities (trees, buildings ...)
	std::vector<char> blocked(static_cast<size_t>(w) * h, 0);
	for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (map.isSolid(x, y)) blocked[y * w + x] = 1;
	for (auto& e : scene.get_entities()){
		if (!e->isSolid() || e->isDead()) continue;
		sf::FloatRect b = e->getHitbox();
		int x0 = static_cast<int>(std::floor(b.left / tileSize)), x1 = static_cast<int>(std::floor((b.left + b.width - 0.01f) / tileSize));
		int y0 = static_cast<int>(std::floor(b.top / tileSize)), y1 = static_cast<int>(std::floor((b.top + b.height - 0.01f) / tileSize));
		for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (x >= 0 && y >= 0 && x < w && y < h) blocked[y * w + x] = 1;
	}
	blocked[from.y * w + from.x] = 0;
	blocked[to.y * w + to.x] = 0; // the goal tile is where the item / deposit point is, always allowed
	auto heuristic = [&](int x, int y){ int dx = std::abs(x - to.x), dy = std::abs(y - to.y); return static_cast<float>(std::max(dx, dy)) + 0.41f * static_cast<float>(std::min(dx, dy)); };
	std::vector<float> cost(static_cast<size_t>(w) * h, 1e9f);
	std::vector<int> parent(static_cast<size_t>(w) * h, -1);
	typedef std::pair<float, int> node;
	std::priority_queue<node, std::vector<node>, std::greater<node>> open;
	int startIdx = from.y * w + from.x, goalIdx = to.y * w + to.x;
	cost[startIdx] = 0.0f;
	open.push(node(heuristic(from.x, from.y), startIdx));
	const int dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
	const int dy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
	while (!open.empty()){
		node n = open.top(); open.pop();
		int cur = n.second;
		if (cur == goalIdx) break;
		int cx = cur % w, cy = cur / w;
		if (n.first - heuristic(cx, cy) > cost[cur] + 0.001f) continue; // stale entry
		for (int k = 0; k < 8; k++){
			int nx = cx + dx[k], ny = cy + dy[k];
			if (nx < 0 || ny < 0 || nx >= w || ny >= h || blocked[ny * w + nx]) continue;
			if (k >= 4 && (blocked[cy * w + nx] || blocked[ny * w + cx])) continue; // no cutting corners
			float step = (k >= 4) ? 1.41f : 1.0f;
			int ni = ny * w + nx;
			if (cost[cur] + step < cost[ni]){
				cost[ni] = cost[cur] + step;
				parent[ni] = cur;
				open.push(node(cost[ni] + heuristic(nx, ny), ni));
			}
		}
	}
	if (parent[goalIdx] == -1 && goalIdx != startIdx) return result;
	for (int c = goalIdx; c != startIdx && c != -1; c = parent[c]) result.push_back(sf::Vector2i(c % w, c / w));
	std::reverse(result.begin(), result.end());
	return result;
}
