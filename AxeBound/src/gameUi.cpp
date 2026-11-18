// gameUi.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// The second half of GameScene: the HUD (coins, health, tools, 4 buttons), the Build / Shop / Workers / Pause /
// Victory panels, and all drawing (world layer with the 480x270 camera, then the UI layer).

#include <algorithm>
#include <cmath>
#include <sstream>

#include "scene.h"
#include "gameSystem.h"
#include "globals.h"
#include "inputManager.h"
#include "saveManager.h"
#include "audio.h"

static std::shared_ptr<label> makeLabel(sf::Vector2u s, float x, float y, float w, float h, const std::string& t, float px, bool left, bool box)
{
	auto l = std::make_shared<label>(s, sf::Vector2f(x, y), sf::Vector2f(w, h), t, gameFont, sf::Color::Black);
	if (box) l->setBackground(palette::uiGreenLight, sf::Color::Black);
	l->setTextSize(px);
	l->setAlignLeft(left);
	return l;
}

static std::shared_ptr<uiButton> makeButton(sf::Vector2u s, float x, float y, float w, float h, const std::string& t, std::function<void()> cb)
{
	auto b = std::make_shared<uiButton>(s, sf::Vector2f(x, y), sf::Vector2f(w, h), gameFont, sf::Color::Black, sf::Color::Black, palette::uiGreen, t, cb);
	b->setColours(palette::uiGreen, palette::uiSelected);
	return b;
}

// =====================================================================================================================
//  building the UI (done once, in load())
// =====================================================================================================================
void GameScene::buildUi()
{
	GameScene* self = this;
	// ----- HUD (top bar + 4 buttons, bottom tool bar)
	auto top = makeLabel(uiSize, 0, 0, 1280, 56, "", 20, true, true);
	top->setBackground(palette::uiGreen, sf::Color::Black);
	hud.add(top);
	coinsLabel = makeLabel(uiSize, 14, 0, 200, 56, "Coins: 0", 18, true, false);
	coinsLabel->setBackground(palette::uiGreen, palette::uiGreen);
	hud.add(coinsLabel);
	healthLabel = makeLabel(uiSize, 230, 0, 330, 56, "HP 100  Frost 0", 18, true, false);
	healthLabel->setBackground(palette::uiGreen, palette::uiGreen);
	hud.add(healthLabel);
	menuBtn = makeButton(uiSize, 580, 8, 150, 40, "Menu", [self]() { self->deferred = [self]() { self->openPanel(PANEL_PAUSE); }; });
	buildBtn = makeButton(uiSize, 745, 8, 150, 40, "Build", [self]() { self->deferred = [self]() { self->openPanel(self->panel == PANEL_BUILD ? PANEL_NONE : PANEL_BUILD); }; });
	shopBtn = makeButton(uiSize, 910, 8, 150, 40, "Shop", [self]() { self->deferred = [self]() { self->openPanel(self->panel == PANEL_SHOP ? PANEL_NONE : PANEL_SHOP); }; });
	workersBtn = makeButton(uiSize, 1075, 8, 190, 40, "Workers", [self]() { self->deferred = [self]() { self->openPanel(self->panel == PANEL_WORKERS ? PANEL_NONE : PANEL_WORKERS); }; });
	hud.add(menuBtn); hud.add(buildBtn); hud.add(shopBtn); hud.add(workersBtn);
	for (int i = 0; i < 3; i++){
		toolLabel[i] = makeLabel(uiSize, 14.0f + i * 150.0f, 664, 140, 44, "", 15, false, true);
		toolLabel[i]->setBackground(palette::uiGreenLight, sf::Color::Black);
		hud.add(toolLabel[i]);
	}
	objectiveLabel = makeLabel(uiSize, 480, 664, 786, 44, "", 15, true, true);
	hud.add(objectiveLabel);
	toastLabel = makeLabel(uiSize, 240, 76, 800, 44, "", 15, false, true);
	toastLabel->setVisible(false);
	hud.add(toastLabel);
	hintLabel = makeLabel(uiSize, 340, 610, 600, 40, "", 14, false, true);
	hintLabel->setVisible(false);
	hud.add(hintLabel);

	// ----- Build panel (design: list of buildings with cost)
	buildPanel.add(makeLabel(uiSize, 880, 66, 390, 590, "", 14, false, true));
	buildPanel.add(makeLabel(uiSize, 890, 72, 370, 40, "Build (R rotate)", 18, false, false));
	for (int i = 0; i < B_TYPE_COUNT; i++){
		const buildingInfo& info = getBuildingInfo(static_cast<buildingType>(i));
		std::string txt = std::string(info.name) + "  " + std::to_string(info.cost);
		buildItemBtn[i] = makeButton(uiSize, 895, 122.0f + i * 62.0f, 360, 52, txt, [self, i]() {
			self->deferred = [self, i]() { self->placing = i; self->placeDir = 0; self->closePanels(); self->placing = i; };
		});
		buildPanel.add(buildItemBtn[i]);
	}

	// ----- Shop panel
	shopPanel.add(makeLabel(uiSize, 880, 66, 390, 590, "", 14, false, true));
	shopPanel.add(makeLabel(uiSize, 890, 72, 370, 40, "General Store", 18, false, false));
	shopInfo = makeLabel(uiSize, 895, 118, 360, 60, "", 13, true, false);
	shopPanel.add(shopInfo);
	shopIronBtn = makeButton(uiSize, 895, 190, 360, 56, "Iron axe 300", [self]() { self->deferred = [self]() { self->buyTool(1); }; });
	shopSteelBtn = makeButton(uiSize, 895, 258, 360, 56, "Steel axe 900", [self]() { self->deferred = [self]() { self->buyTool(2); }; });
	shopTruckBtn = makeButton(uiSize, 895, 326, 360, 56, "Truck 1200", [self]() { self->deferred = [self]() { self->buyTruck(); }; });
	shopPanel.add(shopIronBtn); shopPanel.add(shopSteelBtn); shopPanel.add(shopTruckBtn);

	// ----- Workers panel (hire + per worker: on/off toggle and deposit location)
	workerPanel.add(makeLabel(uiSize, 640, 66, 630, 590, "", 14, false, true));
	workerPanel.add(makeLabel(uiSize, 650, 72, 400, 40, "Workers", 18, false, false));
	workerInfo = makeLabel(uiSize, 655, 116, 600, 30, "", 13, true, false);
	workerPanel.add(workerInfo);
	hireBtn = makeButton(uiSize, 1040, 70, 220, 40, "Hire 100", [self]() { self->deferred = [self]() { self->hireWorker(); }; });
	workerPanel.add(hireBtn);
	for (int i = 0; i < 6; i++){
		float y = 156.0f + i * 82.0f;
		workerRowLabel[i] = makeLabel(uiSize, 655, y, 130, 60, "", 14, true, false);
		workerToggle[i] = makeButton(uiSize, 790, y, 130, 52, "Off", [self, i]() {
			self->deferred = [self, i]() {
				int n = 0;
				for (auto& e : self->get_entities()){
					if (e->getClass() != EC_WORKER) continue;
					if (n++ == i){ workerAI* w = static_cast<workerAI*>(e.get()); w->setActive(!w->isActive); }
				}
			};
		});
		workerDeposit[i] = makeButton(uiSize, 930, y, 330, 52, "Deposit", [self, i]() {
			self->deferred = [self, i]() {
				int n = 0, count = self->depositCount();
				for (auto& e : self->get_entities()){
					if (e->getClass() != EC_WORKER) continue;
					if (n++ == i && count > 0){ workerAI* w = static_cast<workerAI*>(e.get()); w->depositIndex = (w->depositIndex + 1) % count; }
				}
			};
		});
		workerPanel.add(workerRowLabel[i]); workerPanel.add(workerToggle[i]); workerPanel.add(workerDeposit[i]);
	}

	// ----- Pause panel (also shows the objective + save)
	pausePanel.add(makeLabel(uiSize, 0, 0, 1280, 720, "", 14, false, true));
	pausePanel.add(makeLabel(uiSize, 0, 0, 1280, 84, "Paused", 44, false, true));
	pauseInfo = makeLabel(uiSize, 40, 110, 520, 400, "", 15, true, true);
	pausePanel.add(pauseInfo);
	pausePanel.add(makeButton(uiSize, 700, 150, 360, 60, "Resume", [self]() { self->deferred = [self]() { self->closePanels(); }; }));
	pausePanel.add(makeButton(uiSize, 700, 240, 360, 60, "Save Game", [self]() { self->deferred = [self]() { self->toast(self->saveGame() ? "Game saved." : "Save failed!", 2.5f); self->closePanels(); }; }));
	pausePanel.add(makeButton(uiSize, 700, 330, 360, 60, "Save + Main Menu", [self]() { self->deferred = []() { GameSystem::get().requestMenu(); }; }));
	pausePanel.add(makeButton(uiSize, 700, 420, 360, 60, "Save + Quit", [self]() { self->deferred = [self]() { self->saveGame(); quitRequested = true; }; }));

	// ----- Victory panel
	victoryPanel.add(makeLabel(uiSize, 0, 0, 1280, 720, "", 14, false, true));
	victoryLabel = makeLabel(uiSize, 190, 160, 900, 260, "", 26, false, true);
	victoryPanel.add(victoryLabel);
	victoryPanel.add(makeButton(uiSize, 440, 460, 400, 60, "Keep Playing", [self]() { self->deferred = [self]() { self->closePanels(); }; }));
	victoryPanel.add(makeButton(uiSize, 440, 540, 400, 60, "Save + Main Menu", [self]() { self->deferred = []() { GameSystem::get().requestMenu(); }; }));
	victoryLabel->updateText("CONGRATULATIONS!\n\nThe airport is finished.\nYou escaped the mountains!");

	hud.resize(uiSize);
}

void GameScene::openPanel(Panel p)
{
	panel = p;
	if (p != PANEL_NONE) placing = -1;
}

void GameScene::closePanels()
{
	panel = PANEL_NONE;
	placing = (panel == PANEL_NONE) ? placing : -1;
}

// =====================================================================================================================
//  refreshing text / enabled state every frame
// =====================================================================================================================
void GameScene::refreshHud()
{
	coinsLabel->updateText("Coins: " + std::to_string(money));
	std::ostringstream hp; hp << "HP " << static_cast<int>(thePlayer->health) << "  Frost " << static_cast<int>(thePlayer->frost) << "%  " << levels[currentLevel].name;
	healthLabel->updateText(hp.str());
	static const char* names[3] = {"1 Basic axe", "2 Iron axe", "3 Steel axe"};
	for (int i = 0; i < 3; i++){
		toolLabel[i]->updateText(thePlayer->tools[i] ? std::string(thePlayer->currentHotbarSlot == i + 1 ? "> " : "") + names[i] : "locked");
	}
	// objective text
	std::string o;
	int obj = currentObjective();
	for (auto& e : levels[0].entities){
		if (e->getClass() != EC_OBJECTIVE) continue;
		objectiveBuilding* b = static_cast<objectiveBuilding*>(e.get());
		if (b->objectiveId == obj - 1 && !b->built) o = "Goal: " + b->progressText();
	}
	if (obj == 3){
		o = "Goal: build the airport on Summit Ridge";
		for (auto& e : levels[2].entities){
			if (e->getClass() == EC_OBJECTIVE){ objectiveBuilding* b = static_cast<objectiveBuilding*>(e.get()); if (!b->built) o = "Goal: " + b->progressText(); else o = "Airport built!"; }
		}
	}
	objectiveLabel->updateText(o);
	if (toastTimer > 0.0f){ toastLabel->updateText(toastText); toastLabel->setVisible(true); } else toastLabel->setVisible(false);

	// context hint
	std::string hint;
	if (placing >= 0) hint = "Click / E to place, R rotate, right click cancel";
	else if (thePlayer->inVehicle) hint = "W/S throttle  A/D steer  E load  Q unload  F leave";
	else if (nearestTree(thePlayer->getPosition(), 22.0f)) hint = "Hold F to chop";
	else if (nearestItem(thePlayer->getPosition(), 16.0f) && !thePlayer->heldItem) hint = "E: pick up";
	else if (thePlayer->heldItem) hint = "Q: drop (into river or sell area)";
	else if (nearestStore(thePlayer->getPosition(), 70.0f, STORE_GENERAL)) hint = "F: General Store";
	else if (nearestStore(thePlayer->getPosition(), 70.0f, STORE_LABOUR)) hint = "F: Labour Office";
	if (!hint.empty()) { hintLabel->updateText(hint); hintLabel->setVisible(true); } else hintLabel->setVisible(false);
}

void GameScene::refreshPanels()
{
	if (panel == PANEL_BUILD){
		for (int i = 0; i < B_TYPE_COUNT; i++){
			buildItemBtn[i]->setSelected(placing == i);
			buildItemBtn[i]->setEnabled(money >= getBuildingInfo(static_cast<buildingType>(i)).cost);
		}
	}else if (panel == PANEL_SHOP){
		bool near = nearestStore(thePlayer->getPosition(), 90.0f, STORE_GENERAL) != nullptr;
		shopInfo->updateText(near ? "Pick your tools." : "Walk to the General Store\nto buy.");
		shopIronBtn->setText(thePlayer->tools[1] ? "Iron axe (owned)" : "Iron axe  300");
		shopSteelBtn->setText(thePlayer->tools[2] ? "Steel axe (owned)" : "Steel axe  900");
		shopTruckBtn->setText(truckOwned ? "Truck (owned)" : "Truck  1200");
		shopIronBtn->setEnabled(near && !thePlayer->tools[1]);
		shopSteelBtn->setEnabled(near && !thePlayer->tools[2]);
		shopTruckBtn->setEnabled(near && !truckOwned);
	}else if (panel == PANEL_WORKERS){
		bool near = nearestStore(thePlayer->getPosition(), 90.0f, STORE_LABOUR) != nullptr;
		hireBtn->setEnabled(near);
		int n = 0;
		std::vector<workerAI*> ws;
		for (auto& e : get_entities()) if (e->getClass() == EC_WORKER) ws.push_back(static_cast<workerAI*>(e.get()));
		workerInfo->updateText(near ? "Hire needs free Worker Housing" : "Hire at the Labour Office");
		for (int i = 0; i < 6; i++){
			bool has = i < static_cast<int>(ws.size());
			workerRowLabel[i]->setVisible(has); workerToggle[i]->setVisible(has); workerDeposit[i]->setVisible(has);
			if (!has) continue;
			workerRowLabel[i]->updateText("Worker " + std::to_string(i + 1) + "\n" + ws[i]->stateName());
			workerToggle[i]->setText(ws[i]->isActive ? "On" : "Off");
			workerToggle[i]->setSelected(ws[i]->isActive);
			workerDeposit[i]->setText("-> " + depositName(ws[i]->depositIndex));
			n++;
		}
		(void)n;
	}else if (panel == PANEL_PAUSE){
		std::ostringstream o;
		o << "Player: " << playerName << "\nDifficulty: " << (difficulty == 0 ? "Easy" : difficulty == 1 ? "Normal" : "Hard")
		  << "\nWorld: " << levels[currentLevel].name << "\nCoins: " << money << "\nTrees chopped: " << treesChopped << "\n\nObjective " << currentObjective() << " of 3:\n";
		o << (currentObjective() == 1 ? "Build the Port" : currentObjective() == 2 ? "Build the Train Station" : "Build the Airport");
		pauseInfo->updateText(o.str());
	}
}

// =====================================================================================================================
//  drawing
// =====================================================================================================================
static sf::View uiView()
{
	return sf::View(sf::FloatRect(-layout.offsetX, -layout.offsetY, static_cast<float>(layout.windowW), static_cast<float>(layout.windowH)));
}

void GameScene::drawWorld(sf::RenderWindow& window)
{
	if (!built || !thePlayer || levels.empty()) return;
	float ww = static_cast<float>(layout.windowW), wh = static_cast<float>(layout.windowH);
	worldView.setSize(static_cast<float>(WORLD_WIDTH), static_cast<float>(WORLD_HEIGHT));
	worldView.setCenter(std::floor(cameraCentre.x), std::floor(cameraCentre.y));
	worldView.setViewport(sf::FloatRect(layout.offsetX / ww, layout.offsetY / wh, layout.width / ww, layout.height / wh));
	window.setView(worldView);
	sf::FloatRect vr(cameraCentre.x - WORLD_WIDTH / 2.0f, cameraCentre.y - WORLD_HEIGHT / 2.0f, static_cast<float>(WORLD_WIDTH), static_cast<float>(WORLD_HEIGHT));
	world().draw(window, vr);

	std::vector<const entity*> order;
	for (auto& e : get_entities()) if (!e->isDead()) order.push_back(e.get());
	if (!thePlayer->inVehicle) order.push_back(thePlayer.get());
	if (theVehicle && vehicleLevel == currentLevel) order.push_back(theVehicle.get());
	std::stable_sort(order.begin(), order.end(), [](const entity* a, const entity* b) { return a->getPosition().y < b->getPosition().y; });
	// flat things (items on ground, sell areas, gates) first so they never cover trees
	for (const entity* e : order){ int c = e->getClass(); if (c == EC_SELL || c == EC_GATE || c == EC_STUMP) e->draw(window); }
	for (const entity* e : order){
		int c = e->getClass();
		if (c == EC_SELL || c == EC_GATE || c == EC_STUMP) continue;
		sf::Vector2f p = e->getPosition();
		if (p.x < vr.left - 64 || p.x > vr.left + vr.width + 64 || p.y < vr.top - 32 || p.y > vr.top + vr.height + 96) continue;
		e->draw(window);
	}
	if (placing >= 0) drawPlacementGhost(window);
}

void GameScene::drawPlacementGhost(sf::RenderWindow& window)
{
	buildingType t = static_cast<buildingType>(placing);
	sf::Vector2i o = placementTile(t);
	const buildingInfo& info = getBuildingInfo(t);
	sf::RectangleShape r(sf::Vector2f(static_cast<float>(info.tilesW * tileSize), static_cast<float>(info.tilesH * tileSize)));
	r.setPosition(static_cast<float>(o.x * tileSize), static_cast<float>(o.y * tileSize));
	bool ok = canPlace(t, o.x, o.y);
	r.setFillColor(ok ? sf::Color(0x56, 0xbd, 0x3b, 140) : sf::Color(220, 40, 40, 140));
	r.setOutlineColor(sf::Color::Black); r.setOutlineThickness(-1.0f);
	window.draw(r);
	static const float ang[4] = {0, 90, 180, 270};
	sf::ConvexShape arrow(3);
	arrow.setPoint(0, sf::Vector2f(4, 0)); arrow.setPoint(1, sf::Vector2f(-3, -3)); arrow.setPoint(2, sf::Vector2f(-3, 3));
	arrow.setFillColor(sf::Color::Black);
	arrow.setPosition(r.getPosition() + r.getSize() / 2.0f);
	arrow.setRotation(ang[placeDir & 3]);
	window.draw(arrow);
}

void GameScene::drawOverheadLabels(sf::RenderWindow& window)
{
	float s = static_cast<float>(layout.width) / static_cast<float>(WORLD_WIDTH);
	sf::Text t("", gameFont, static_cast<unsigned>(std::max(8.0f, 11.0f * layout.width / 1280.0f)));
	for (auto& e : get_entities()){
		if (e->isDead()) continue;
		std::string txt = e->label();
		if (txt.empty()) continue;
		sf::Vector2f p = e->getPosition();
		float ux = (p.x - (cameraCentre.x - WORLD_WIDTH / 2.0f)) * s;
		float uy = (p.y - (cameraCentre.y - WORLD_HEIGHT / 2.0f)) * s;
		if (ux < -100 || ux > layout.width + 100 || uy < 0 || uy > layout.height + 100) continue;
		if (e->getClass() == EC_WORKER) continue;
		float lift = 38.0f * s;
		if (e->getClass() == EC_SELL || e->getClass() == EC_GATE) lift = 10.0f * s;
		t.setString(txt);
		t.setFillColor(sf::Color::Black);
		sf::FloatRect b = t.getLocalBounds();
		t.setOrigin(b.left + b.width / 2.0f, b.top + b.height);
		t.setPosition(ux, uy - lift);
		sf::RectangleShape bg(sf::Vector2f(b.width + 8.0f, b.height + 6.0f));
		bg.setFillColor(sf::Color(0x9f, 0xce, 0x63, 220));
		bg.setPosition(ux - (b.width + 8.0f) / 2.0f, uy - lift - b.height - 3.0f);
		window.draw(bg);
		window.draw(t);
	}
}

void GameScene::render(sf::RenderWindow& window)
{
	window.setView(uiView());
	if (!built || !thePlayer || levels.empty()) return;
	// the UI view is in window pixels shifted by the letterbox offset, so the elements (scaled to the 16:9 area) line up
	drawOverheadLabels(window);
	if (panel == PANEL_PAUSE) { pausePanel.draw(window); }
	else if (panel == PANEL_VICTORY) { victoryPanel.draw(window); }
	else{
		hud.draw(window);
		if (panel == PANEL_BUILD) buildPanel.draw(window);
		else if (panel == PANEL_SHOP) shopPanel.draw(window);
		else if (panel == PANEL_WORKERS) workerPanel.draw(window);
	}
	if (usingCursor){
		sf::CircleShape c(6.0f * layout.width / 1280.0f);
		c.setFillColor(sf::Color::White); c.setOutlineColor(sf::Color::Black); c.setOutlineThickness(2.0f);
		c.setPosition(cursorUi.x - c.getRadius(), cursorUi.y - c.getRadius());
		window.draw(c);
	}
}
