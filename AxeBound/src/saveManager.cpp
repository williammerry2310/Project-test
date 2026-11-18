// saveManager.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026

#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <algorithm>

#include "saveManager.h"
#include "inputManager.h"
#include "globals.h"
#include "audio.h"

int musicVolume = 50;
int soundVolume = 70;

std::string slotPath(int slot) { return "saves/slot" + std::to_string(slot) + ".sav"; }

// The first lines of a slot file are "key value" pairs, the game scene writes the entities after the line "ENTITIES".
saveInfo readSaveInfo(int slot)
{
	saveInfo info;
	std::ifstream f(slotPath(slot));
	if (!f.is_open()) return info;
	std::string line;
	while (std::getline(f, line)){
		if (line == "ENTITIES") break;
		std::istringstream in(line);
		std::string key; in >> key;
		if (key == "name") { std::getline(in >> std::ws, info.name); }
		else if (key == "date") in >> info.date;
		else if (key == "epoch") in >> info.epoch;
		else if (key == "difficulty") in >> info.difficulty;
		else if (key == "level") { in >> info.world; info.world += 1; }
		else if (key == "money") in >> info.money;
		else if (key == "trees") in >> info.trees;
		else if (key == "objective") in >> info.objective;
	}
	info.exists = true;
	if (info.name.empty()) info.name = "Save " + std::to_string(slot);
	return info;
}

int latestSlot()
{
	int best = 0; long long bestEpoch = -1;
	for (int i = 1; i <= SAVE_SLOTS; i++){
		saveInfo s = readSaveInfo(i);
		if (s.exists && s.epoch > bestEpoch) { best = i; bestEpoch = s.epoch; }
	}
	return best;
}

int firstFreeOrOldestSlot()
{
	int oldest = 1; long long oldestEpoch = -1;
	for (int i = 1; i <= SAVE_SLOTS; i++){
		saveInfo s = readSaveInfo(i);
		if (!s.exists) return i;
		if (oldestEpoch < 0 || s.epoch < oldestEpoch) { oldest = i; oldestEpoch = s.epoch; }
	}
	return oldest;
}

void loadSettings()
{
	std::ifstream f("saves/settings.cfg");
	if (!f.is_open()) return;
	std::string key;
	while (f >> key){
		if (key == "music") f >> musicVolume;
		else if (key == "sound") f >> soundVolume;
		else if (key == "key"){
			size_t idx; int code;
			f >> idx >> code;
			if (idx < controls.size() && code >= 0 && code < static_cast<int>(sf::Keyboard::KeyCount)) controls[idx] = static_cast<sf::Keyboard::Key>(code);
		}
	}
	musicVolume = std::max(0, std::min(100, musicVolume));
	soundVolume = std::max(0, std::min(100, soundVolume));
}

void saveSettings()
{
	std::error_code ec;
	std::filesystem::create_directories("saves", ec);
	std::ofstream f("saves/settings.cfg", std::ios::trunc);
	if (!f.is_open()) { gameLog.error("saveSettings: can not write saves/settings.cfg"); return; }
	f << "music " << musicVolume << "\n" << "sound " << soundVolume << "\n";
	for (size_t i = 0; i < controls.size(); i++) f << "key " << i << " " << static_cast<int>(controls[i]) << "\n";
}
