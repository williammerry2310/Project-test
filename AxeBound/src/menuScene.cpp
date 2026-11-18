// menuScene.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// MenuScene = the "UI scene" in the design flowchart ("Main Menu Navigation"). Pages follow the mock-ups:
// Main menu, Saves, New Game, Settings (+ Keybinds), Credits. Colours are the green boxes from the mock-ups.

#include <algorithm>

#include "scene.h"
#include "gameSystem.h"
#include "globals.h"
#include "inputManager.h"
#include "saveManager.h"
#include "audio.h"
#include "render.h"

MenuScene::MenuScene() {}

// ---- small builders so every page uses the same look (green boxes with a thin black outline)
void MenuScene::addBanner(uiGroup& g, const std::string& text)
{
	auto b = std::make_shared<label>(uiSize, sf::Vector2f(0.0f, 0.0f), sf::Vector2f(1280.0f, 84.0f), text, gameFont, sf::Color::Black);
	b->setBackground(palette::uiGreen, sf::Color::Black);
	b->setTextSize(44.0f);
	g.add(b);
}

std::shared_ptr<uiButton> MenuScene::addButton(uiGroup& g, float x, float y, float w, float h, const std::string& text, std::function<void()> cb)
{
	auto b = std::make_shared<uiButton>(uiSize, sf::Vector2f(x, y), sf::Vector2f(w, h), gameFont, sf::Color::Black, sf::Color::Black, palette::uiGreen, text, cb);
	b->setColours(palette::uiGreen, palette::uiSelected);
	g.add(b);
	return b;
}

static void addFooter(uiGroup& g, sf::Vector2u uiSize)
{
	auto f = std::make_shared<label>(uiSize, sf::Vector2f(0.0f, 646.0f), sf::Vector2f(1280.0f, 74.0f), "", gameFont, sf::Color::Black);
	f->setBackground(palette::uiGreenLight, sf::Color::Black);
	g.add(f);
}

void MenuScene::buildMain()
{
	uiGroup& g = pages[PAGE_MAIN];
	addBanner(g, "AxeBound");
	continueButton = addButton(g, 510.0f, 130.0f, 260.0f, 58.0f, "Continue", [this]() {
		int slot = latestSlot();
		if (slot > 0) GameSystem::get().requestLoad(slot);
	});
	addButton(g, 510.0f, 216.0f, 260.0f, 58.0f, "Saves", [this]() { showPage(PAGE_SAVES); });
	addButton(g, 510.0f, 302.0f, 260.0f, 58.0f, "New Game", [this]() { showPage(PAGE_NEWGAME); });
	addButton(g, 510.0f, 388.0f, 260.0f, 58.0f, "Settings", [this]() { showPage(PAGE_SETTINGS); });
	addButton(g, 510.0f, 474.0f, 260.0f, 58.0f, "Credits", [this]() { showPage(PAGE_CREDITS); });
	addButton(g, 510.0f, 560.0f, 260.0f, 58.0f, "Quit Game", []() { quitRequested = true; });
	addFooter(g, uiSize);
}

void MenuScene::buildSaves()
{
	uiGroup& g = pages[PAGE_SAVES];
	addBanner(g, "Saves");
	detailsLabel = std::make_shared<label>(uiSize, sf::Vector2f(10.0f, 96.0f), sf::Vector2f(280.0f, 190.0f), "Details:", gameFont, sf::Color::Black);
	detailsLabel->setBackground(palette::uiGreenLight, sf::Color::Black);
	detailsLabel->setAlignLeft(true);
	detailsLabel->setTextSize(17.0f);
	g.add(detailsLabel);
	for (int i = 0; i < SAVE_SLOTS; i++){
		slotButtons[i] = addButton(g, 305.0f, 96.0f + i * 100.0f, 600.0f, 88.0f, "Empty slot", [this, i]() { selectSlot(i + 1); });
	}
	addButton(g, 8.0f, 596.0f, 220.0f, 42.0f, "Back", [this]() { showPage(PAGE_MAIN); });
	loadButton = addButton(g, 1000.0f, 580.0f, 220.0f, 42.0f, "Load", [this]() {
		if (selectedSlot > 0 && readSaveInfo(selectedSlot).exists) GameSystem::get().requestLoad(selectedSlot);
	});
	addFooter(g, uiSize);
}

void MenuScene::buildNewGame()
{
	uiGroup& g = pages[PAGE_NEWGAME];
	addBanner(g, "New Game");
	const char* names[3] = {"Easy", "Normal", "Hard"};
	for (int i = 0; i < 3; i++){
		diffButtons[i] = addButton(g, 40.0f, 250.0f + i * 52.0f, 210.0f, 42.0f, names[i], [this, i]() {
			difficulty = i;
			for (int k = 0; k < 3; k++) diffButtons[k]->setSelected(k == difficulty);
		});
	}
	auto panel = std::make_shared<label>(uiSize, sf::Vector2f(293.0f, 248.0f), sf::Vector2f(590.0f, 146.0f), "", gameFont, sf::Color::Black);
	panel->setBackground(palette::uiGreenLight, sf::Color::Black);
	g.add(panel);
	auto nameLabel = std::make_shared<label>(uiSize, sf::Vector2f(310.0f, 304.0f), sf::Vector2f(80.0f, 24.0f), "Name:", gameFont, sf::Color::Black);
	g.add(nameLabel);
	nameBox = std::make_shared<inputTextBox>(uiSize, sf::Vector2f(420.0f, 296.0f), sf::Vector2f(440.0f, 52.0f), gameFont, sf::Color::Black, sf::Color::Black, palette::uiGrey, "Type here...", 14);
	g.add(nameBox);
	addButton(g, 930.0f, 310.0f, 220.0f, 46.0f, "Start Game", [this]() {
		std::string n = nameBox->getInput();
		if (n.empty()) n = "Lumberjack";
		GameSystem::get().requestNewGame(n, difficulty);
	});
	addButton(g, 8.0f, 596.0f, 220.0f, 42.0f, "Back", [this]() { showPage(PAGE_MAIN); });
	addFooter(g, uiSize);
}

void MenuScene::buildSettings()
{
	uiGroup& g = pages[PAGE_SETTINGS];
	addBanner(g, "Settings");
	musicButton = addButton(g, 450.0f, 150.0f, 380.0f, 62.0f, "Music", [this]() {
		musicVolume = (musicVolume >= 100) ? 0 : musicVolume + 10;
		updateAudioVolumes(); saveSettings(); refreshSettings();
	});
	soundButton = addButton(g, 450.0f, 240.0f, 380.0f, 62.0f, "Sound", [this]() {
		soundVolume = (soundVolume >= 100) ? 0 : soundVolume + 10;
		updateAudioVolumes(); saveSettings(); refreshSettings();
	});
	addButton(g, 450.0f, 330.0f, 380.0f, 62.0f, "Keybinds", [this]() { keyPage = 0; rebinding = -1; refreshKeybinds(); showPage(PAGE_KEYBINDS); });
	addButton(g, 8.0f, 596.0f, 220.0f, 42.0f, "Back", [this]() { showPage(PAGE_MAIN); });
	addFooter(g, uiSize);
}

// keybinds: 3 pages x 4 rows. controls[] index list is in inputManager.cpp
static const int keyRows[3][4] = {{0, 2, 1, 3}, {4, 12, 13, 14}, {15, 16, 17, 18}};
static const char* keyRowNames[3][4] = {{"Forward", "Left", "Back", "Right"}, {"Sprint", "Pick Up", "Drop", "Interact"}, {"Build Menu", "Worker Menu", "Shop Menu", "Rotate"}};

void MenuScene::buildKeybinds()
{
	uiGroup& g = pages[PAGE_KEYBINDS];
	addBanner(g, "Keybinds");
	for (int r = 0; r < 4; r++){
		float y = 131.0f + r * 88.0f;
		auto nameBtn = std::make_shared<label>(uiSize, sf::Vector2f(370.0f, y), sf::Vector2f(254.0f, 58.0f), "", gameFont, sf::Color::Black);
		nameBtn->setBackground(palette::uiGreen, sf::Color::Black);
		nameBtn->setTextSize(22.0f);
		g.add(nameBtn);
		keyNameLabels[r] = nameBtn;
		keyValueButtons[r] = addButton(g, 670.0f, y, 112.0f, 58.0f, "", []() {});
		keyNewButtons[r] = addButton(g, 810.0f, y, 126.0f, 58.0f, "New Key", [this, r]() {
			rebinding = keyRows[keyPage][r];
			refreshKeybinds();
		});
	}
	prevPageButton = addButton(g, 370.0f, 490.0f, 180.0f, 58.0f, "Prev Page", [this]() { if (keyPage > 0) { keyPage--; rebinding = -1; refreshKeybinds(); } });
	pageLabel = std::make_shared<label>(uiSize, sf::Vector2f(560.0f, 490.0f), sf::Vector2f(180.0f, 58.0f), "Page 1", gameFont, sf::Color::Black);
	pageLabel->setBackground(palette::uiGreen, sf::Color::Black);
	pageLabel->setTextSize(22.0f);
	g.add(pageLabel);
	nextPageButton = addButton(g, 750.0f, 490.0f, 186.0f, 58.0f, "Next Page", [this]() { if (keyPage < 2) { keyPage++; rebinding = -1; refreshKeybinds(); } });
	addButton(g, 8.0f, 596.0f, 220.0f, 42.0f, "Back", [this]() { rebinding = -1; showPage(PAGE_SETTINGS); });
	addFooter(g, uiSize);
}

void MenuScene::buildCredits()
{
	uiGroup& g = pages[PAGE_CREDITS];
	addBanner(g, "Credits");
	auto text = std::make_shared<label>(uiSize, sf::Vector2f(240.0f, 120.0f), sf::Vector2f(800.0f, 440.0f),
		"AxeBound\n\nGames Engineering Group Project - Set09121\nGroup H\n\nBen Arthurs\nAndrew Donohoe\nWilliam Merry\n\nFont: Dogica (dafont.com)\nBuilt with SFML", gameFont, sf::Color::Black);
	text->setBackground(palette::uiGreenLight, sf::Color::Black);
	text->setAlignLeft(true);
	text->setTextSize(20.0f);
	g.add(text);
	addButton(g, 8.0f, 596.0f, 220.0f, 42.0f, "Back", [this]() { showPage(PAGE_MAIN); });
	addFooter(g, uiSize);
}

// ---- page logic
void MenuScene::load()
{
	if (!built){
		buildMain(); buildSaves(); buildNewGame(); buildSettings(); buildKeybinds(); buildCredits();
		built = true;
		diffButtons[1]->setSelected(true);
	}
	rebinding = -1;
	showPage(PAGE_MAIN);
	playMenuMusic();
}

void MenuScene::showPage(Page p)
{
	current = p;
	for (int i = 0; i < PAGE_COUNT; i++) pages[i].setVisible(i == p);
	if (p == PAGE_MAIN) continueButton->setEnabled(latestSlot() > 0);
	if (p == PAGE_SAVES) { selectedSlot = 0; refreshSaves(); }
	if (p == PAGE_SETTINGS) refreshSettings();
	if (p == PAGE_KEYBINDS) refreshKeybinds();
}

void MenuScene::refreshSaves()
{
	for (int i = 0; i < SAVE_SLOTS; i++){
		saveInfo s = readSaveInfo(i + 1);
		slotButtons[i]->setText(s.exists ? (s.name + "    " + s.date) : "Empty slot");
		slotButtons[i]->setSelected(selectedSlot == i + 1);
	}
	saveInfo s = (selectedSlot > 0) ? readSaveInfo(selectedSlot) : saveInfo();
	if (s.exists){
		detailsLabel->updateText("Details:\n- World " + std::to_string(s.world) + "\n- Money " + std::to_string(s.money) + "\n- Trees chopped " + std::to_string(s.trees) + "\n- Current Objective " + std::to_string(s.objective));
	}else{
		detailsLabel->updateText("Details:\n\nSelect a save");
	}
	loadButton->setEnabled(s.exists);
}

void MenuScene::selectSlot(int slot) { selectedSlot = slot; refreshSaves(); }

void MenuScene::refreshSettings()
{
	musicButton->setText("Music: " + std::to_string(musicVolume) + "%");
	soundButton->setText("Sound: " + std::to_string(soundVolume) + "%");
}

void MenuScene::refreshKeybinds()
{
	for (int r = 0; r < 4; r++){
		int idx = keyRows[keyPage][r];
		keyNameLabels[r]->updateText(keyRowNames[keyPage][r]);
		keyValueButtons[r]->setText(rebinding == idx ? "..." : keyName(controls[idx]));
	}
	pageLabel->updateText("Page " + std::to_string(keyPage + 1));
	prevPageButton->setEnabled(keyPage > 0);
	nextPageButton->setEnabled(keyPage < 2);
}

void MenuScene::update(float dt)
{
	if (!built) return;
	// rebinding: the next key pressed becomes the new key (Esc cancels)
	if (rebinding >= 0 && frame.keyPressedEvent){
		if (frame.lastKey != sf::Keyboard::Escape && frame.lastKey != sf::Keyboard::Unknown){
			controls[rebinding] = frame.lastKey;
			saveSettings();
		}
		rebinding = -1;
		refreshKeybinds();
		return;
	}
	Page p = current; // a click can change the page: only the page that was visible at the start of the frame handles it
	pages[p].update(dt, static_cast<float>(frame.mouse.x), static_cast<float>(frame.mouse.y), frame.mouseClicked);
}

void MenuScene::render(sf::RenderWindow& window)
{
	if (!built) return;
	pages[current].draw(window);
}

void MenuScene::onResize(sf::Vector2u newSize)
{
	uiSize = newSize;
	for (int i = 0; i < PAGE_COUNT; i++) pages[i].resize(newSize);
}

void MenuScene::onText(sf::Uint32 unicode)
{
	if (built && current == PAGE_NEWGAME) nameBox->inputKey(unicode);
}
