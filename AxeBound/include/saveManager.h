// saveManager.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// Small text-file save system: saves/slot1.sav .. slot5.sav and saves/settings.cfg

#pragma once
#include <string>

constexpr int SAVE_SLOTS = 5;

struct saveInfo {
	bool exists = false;
	std::string name;
	std::string date;      // DD/MM/YY
	long long epoch = 0;   // used to find the newest save for "Continue"
	int difficulty = 1;
	int world = 1;
	int money = 0;
	int trees = 0;
	int objective = 1;     // 1 = port, 2 = train station, 3 = airport
};

std::string slotPath(int slot);                 // slot is 1..5
saveInfo readSaveInfo(int slot);
int latestSlot();                               // 0 if there are no saves
int firstFreeOrOldestSlot();

// user settings (keybinds + volumes)
extern int musicVolume;
extern int soundVolume;
void loadSettings();
void saveSettings();
