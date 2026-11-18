#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "SFML/Graphics.hpp"
#include "iostream"
#include "string"
#include "queue"
#include "vector"

struct button
{
	float strength; // for controller support
	std::string action;
};

// CHANGED: public so the keybind screen can change them. Index list:
//  0 W  1 S  2 A  3 D  4 Sprint  5 Stealth  6 Escape  7 Space  8-11 UI arrows (the original 12)
//  12 E pick up  13 Q drop  14 F interact  15 B build  16 M workers  17 N shop  18 R rotate (ADDED)
extern std::vector<sf::Keyboard::Key> controls;

void initInputManager(); // run once at init
void updateUserInputs(); // run everyframe to update new user inputs
button getUserInput(); // run once per frame after updateUserInputs to get a button returned for proccessing

// ADDED: helpers that work on what getUserInput() has returned this frame (call them AFTER the queue has been drained)
bool inputHeld(const std::string& action);
bool inputJustPressed(const std::string& action); // true only on the first frame the action is down
float inputStrength(const std::string& action);
bool inputControllerConnected();
std::string keyName(sf::Keyboard::Key key);

#endif
