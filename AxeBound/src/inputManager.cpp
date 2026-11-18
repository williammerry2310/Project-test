#include "inputManager.h"
#include "globals.h"
#include <cmath>
#include <map>
#include <set>

// input manager
// gets all inputs and abstracts them to a strength, action type
//
//

std::queue<button> userInputs;
std::vector<sf::Keyboard::Key> controls;
bool controller = false;
int controllerID = 0;
float deadZone = 10.0f;

// ADDED: remembers what was held this frame / last frame so single presses (E, Q, F, B ...) can be told apart from holding
static std::map<std::string, float> heldNow;
static std::set<std::string> heldLast;
static std::set<std::string> pressedNow;

void initInputManager()
{
	controls = { // make a way so that these can be changed in settings, also add controller support?
		sf::Keyboard::W,
		sf::Keyboard::S,
		sf::Keyboard::A,
		sf::Keyboard::D,
		sf::Keyboard::LShift, // Sprint
		sf::Keyboard::LControl, // Sneak/Stealth
		sf::Keyboard::Escape, // escape menu
		sf::Keyboard::Space, // jump or select?
		sf::Keyboard::Up, // ui movements
		sf::Keyboard::Down,
		sf::Keyboard::Left,
		sf::Keyboard::Right,
		sf::Keyboard::E,  // Pick Up / Confirm Placement
		sf::Keyboard::Q,  // Drop
		sf::Keyboard::F,  // Interact / Chop
		sf::Keyboard::B,  // Build menu
		sf::Keyboard::M,  // Worker menu
		sf::Keyboard::N,  // Shop menu
		sf::Keyboard::R   // Rotate building while placing
	};
	if (sf::Joystick::isConnected(0)) controller = true;
}

void keyboardInputs()
{
	if (sf::Keyboard::isKeyPressed(controls[0])) { // up
        userInputs.push({1.0f, "Move Up"}); // keyboard defalts to 1 stregth
    }
    if (sf::Keyboard::isKeyPressed(controls[1])) { // down
        userInputs.push({1.0f, "Move Down"});
    }
	if (sf::Keyboard::isKeyPressed(controls[2])) { // up
        userInputs.push({1.0f, "Move Left"}); // keyboard defalts to 1 stregth
    }
    if (sf::Keyboard::isKeyPressed(controls[3])) { // down
        userInputs.push({1.0f, "Move Right"});
    }
	
	if (sf::Keyboard::isKeyPressed(controls[4])) { // Sprint
        userInputs.push({1.0f, "Sprint"});
    }
	if (sf::Keyboard::isKeyPressed(controls[5])) { // Slow Walk (stealth)
        userInputs.push({1.0f, "Stealth"});
    }
	if (sf::Keyboard::isKeyPressed(controls[6])) { // escape menu
        userInputs.push({1.0f, "Escape"});
    }
	if (sf::Keyboard::isKeyPressed(controls[7])) { // select (jump if platformer style)
        userInputs.push({1.0f, "Space"});
    }
	
	if (sf::Keyboard::isKeyPressed(controls[8])) { // Ui up
        userInputs.push({1.0f, "Ui Up"});
    }
	if (sf::Keyboard::isKeyPressed(controls[9])) { // ui down
        userInputs.push({1.0f, "Ui Down"});
    }
	if (sf::Keyboard::isKeyPressed(controls[10])) { // ui left
        userInputs.push({1.0f, "Ui Left"});
    }
	if (sf::Keyboard::isKeyPressed(controls[11])) { // ui right
        userInputs.push({1.0f, "Ui Right"});
    }
	
	// ADDED: the rest of the design document controls
	if (sf::Keyboard::isKeyPressed(controls[12])) userInputs.push({1.0f, "Pick Up"});
	if (sf::Keyboard::isKeyPressed(controls[13])) userInputs.push({1.0f, "Drop"});
	if (sf::Keyboard::isKeyPressed(controls[14])) userInputs.push({1.0f, "Interact"});
	if (sf::Keyboard::isKeyPressed(controls[15])) userInputs.push({1.0f, "Build Menu"});
	if (sf::Keyboard::isKeyPressed(controls[16])) userInputs.push({1.0f, "Worker Menu"});
	if (sf::Keyboard::isKeyPressed(controls[17])) userInputs.push({1.0f, "Shop Menu"});
	if (sf::Keyboard::isKeyPressed(controls[18])) userInputs.push({1.0f, "Rotate"});
	for (int i = 0; i < 10; i++){ // hotbar slots 0 - 9
		if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(sf::Keyboard::Num0 + i))) userInputs.push({1.0f, "Hotbar " + std::to_string(i)});
	}
}

void joyStickButtons()
{
	if (sf::Joystick::isButtonPressed(controllerID, 0)){ //A button
		userInputs.push({1.0f, "Space"});
		userInputs.push({1.0f, "Pick Up"});   // A = pick up / confirm (design controller table)
	}
	if (sf::Joystick::isButtonPressed(controllerID, 1)) userInputs.push({1.0f, "Drop"});        // B
	if (sf::Joystick::isButtonPressed(controllerID, 3)) userInputs.push({1.0f, "Rotate"});      // Y
	if (sf::Joystick::isButtonPressed(controllerID, 4)) userInputs.push({1.0f, "Hotbar Prev"}); // LB
	if (sf::Joystick::isButtonPressed(controllerID, 5)) userInputs.push({1.0f, "Hotbar Next"}); // RB
	if (sf::Joystick::isButtonPressed(controllerID, 9)) userInputs.push({1.0f, "Select"});      // R3 (press right stick)
	// d-pad: left = escape menu, up = build, right = workers, down = shop (design controller table)
	float povX = sf::Joystick::getAxisPosition(controllerID, sf::Joystick::PovX);
	float povY = sf::Joystick::getAxisPosition(controllerID, sf::Joystick::PovY);
	if (povX < -50.0f) userInputs.push({1.0f, "Escape"});
	if (povX > 50.0f) userInputs.push({1.0f, "Worker Menu"});
	if (povY > 50.0f) userInputs.push({1.0f, "Build Menu"});
	if (povY < -50.0f) userInputs.push({1.0f, "Shop Menu"});
}

void joyStickAxis()
{
	// left stick
	float leftAxisX = sf::Joystick::getAxisPosition(controllerID, sf::Joystick::X);
	float leftAxisY = sf::Joystick::getAxisPosition(controllerID, sf::Joystick::Y);
	if (leftAxisX > deadZone){ // posative
		userInputs.push({std::abs(leftAxisX)/100.0f, "Move Right"});
	}
	if (leftAxisX < -deadZone){ // negative
		userInputs.push({std::abs(leftAxisX)/100.0f, "Move Left"});
	}
	if (leftAxisY > deadZone){ // posative
		userInputs.push({std::abs(leftAxisY)/100.0f, "Move Down"});
	}
	if (leftAxisY < -deadZone){ // negative
		userInputs.push({std::abs(leftAxisY)/100.0f, "Move Up"});
	}
	
	// rightg stick
	float rightAxisX = sf::Joystick::getAxisPosition(controllerID, sf::Joystick::U);
	float rightAxisY = sf::Joystick::getAxisPosition(controllerID, sf::Joystick::V);
	if (rightAxisX > deadZone){ // posative
		userInputs.push({std::abs(rightAxisX)/100.0f, "Ui Right"});
	}
	if (rightAxisX < -deadZone){ // negative
		userInputs.push({std::abs(rightAxisX)/100.0f, "Ui Left"});
	}
	if (rightAxisY > deadZone){ // posative
		userInputs.push({std::abs(rightAxisY)/100.0f, "Ui Down"});
	}
	if (rightAxisY < -deadZone){ // negative
		userInputs.push({std::abs(rightAxisY)/100.0f, "Ui Up"});
	}
	
	// both triggers - Z is left trigger, R is right trigger: -100 is rest, 100 is fully pressed
	// CHANGED: the pressed amount is (axis + 100) / 200 (the old code used abs(axis), which is 0 half way down)
	// LT = sprint, RT = interact / chop / throttle (design controller table)
	float leftAmount = (sf::Joystick::getAxisPosition(controllerID, sf::Joystick::Z) + 100.0f) / 200.0f;
	if (leftAmount > 0.3f){
		userInputs.push({leftAmount, "Left Trigger"});
		userInputs.push({1.0f, "Sprint"});
	}
	float rightAmount = (sf::Joystick::getAxisPosition(controllerID, sf::Joystick::R) + 100.0f) / 200.0f;
	if (rightAmount > 0.3f){
		userInputs.push({rightAmount, "Right Trigger"});
		userInputs.push({1.0f, "Interact"});
	}
}

void joyStickInputs()
{
	joyStickButtons();
	joyStickAxis();
}

void updateUserInputs()
{
	heldLast.clear();
	for (auto &h : heldNow) heldLast.insert(h.first);
	heldNow.clear();
	pressedNow.clear();
	controller = sf::Joystick::isConnected(controllerID); // CHANGED: checked every frame so a pad can be plugged in while playing
	keyboardInputs();
	if (controller) joyStickInputs();
}

button getUserInput()
{
	if (userInputs.empty()) return button{0.0f, "END"};
	button nextInput = userInputs.front();
	userInputs.pop();
	float &st = heldNow[nextInput.action];
	if (nextInput.strength > st) st = nextInput.strength;
	if (!heldLast.count(nextInput.action)) pressedNow.insert(nextInput.action);
	return nextInput;
}

bool inputHeld(const std::string& action) { return heldNow.count(action) > 0; }
bool inputJustPressed(const std::string& action) { return pressedNow.count(action) > 0; }
float inputStrength(const std::string& action) { auto it = heldNow.find(action); return it == heldNow.end() ? 0.0f : it->second; }
bool inputControllerConnected() { return controller; }

std::string keyName(sf::Keyboard::Key key)
{
	int k = static_cast<int>(key);
	if (k >= static_cast<int>(sf::Keyboard::A) && k <= static_cast<int>(sf::Keyboard::Z)) return std::string(1, static_cast<char>('A' + (k - static_cast<int>(sf::Keyboard::A))));
	if (k >= static_cast<int>(sf::Keyboard::Num0) && k <= static_cast<int>(sf::Keyboard::Num9)) return std::string(1, static_cast<char>('0' + (k - static_cast<int>(sf::Keyboard::Num0))));
	switch (key){
		case sf::Keyboard::Space: return "Space";
		case sf::Keyboard::LShift: return "L Shift";
		case sf::Keyboard::RShift: return "R Shift";
		case sf::Keyboard::LControl: return "L Ctrl";
		case sf::Keyboard::RControl: return "R Ctrl";
		case sf::Keyboard::Escape: return "Esc";
		case sf::Keyboard::Tab: return "Tab";
		case sf::Keyboard::Up: return "Up";
		case sf::Keyboard::Down: return "Down";
		case sf::Keyboard::Left: return "Left";
		case sf::Keyboard::Right: return "Right";
		default: return "Key " + std::to_string(k);
	}
}