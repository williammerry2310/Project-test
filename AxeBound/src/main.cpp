// main.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// Entry point. CHANGED from the original test version: the test buttons were removed (the real ones live in
// menuScene.cpp / gameUi.cpp) and the window events are now handed to the game through globals.h "frame".

#include <algorithm>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

//local headers
#include "render.h"
#include "init.h"
#include "update.h"
#include "gameSystem.h"
#include "globals.h"
#include "logger.h"
#include "ui.h"

int main(int argc, char** argv)
{
	initWorkingDirectory(argc > 0 ? argv[0] : "");                         // so assets/ and saves/ are found whether run from build/ or the project root
	sf::RenderWindow window(sf::VideoMode(1280, 720), "AxeBound", sf::Style::Default);
	window.setVerticalSyncEnabled(true);
	window.setKeyRepeatEnabled(false);
	updateLayout(window.getSize().x, window.getSize().y);
	sf::Clock clock;
	float dt;
	static float fpsTimer;
	init();                                         // fonts, tile atlas, input, settings, audio, GameSystem::start()

	while (window.isOpen())
	{
		frame.clearEvents();
		dt = clock.restart().asSeconds();
		if (dt > 0.2f) dt = 0.2f;
		fpsTimer += dt;
		if (fpsTimer >= 1.0f){
			window.setTitle("AxeBound - " + std::to_string(static_cast<int>(1.0f / std::max(dt, 0.0001f))) + " fps");
			fpsTimer = 0.0f;
		}

		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Resized){
				updateLayout(event.size.width, event.size.height);
				GameSystem::get().onResize(sf::Vector2u(layout.width, layout.height));
			}
			if (event.type == sf::Event::TextEntered) GameSystem::get().onText(event.text.unicode);
			if (event.type == sf::Event::KeyPressed){
				frame.keyPressedEvent = true;
				frame.lastKey = event.key.code;
			}
			if (event.type == sf::Event::MouseButtonPressed){
				if (event.mouseButton.button == sf::Mouse::Left) frame.mouseClicked = true;
				if (event.mouseButton.button == sf::Mouse::Right) frame.rightClicked = true;
			}
			if (event.type == sf::Event::Closed){
				GameSystem::get().shutdown();
				window.close();
			}
		}
		sf::Vector2i mp = sf::Mouse::getPosition(window);
		frame.mouse = sf::Vector2i(static_cast<int>(mp.x - layout.offsetX), static_cast<int>(mp.y - layout.offsetY));

		window.clear(sf::Color::Black);
		update(dt);
		window.setView(sf::View(sf::FloatRect(-layout.offsetX, -layout.offsetY, static_cast<float>(layout.windowW), static_cast<float>(layout.windowH))));
		render(window);
		window.display();
		gameLog.writeQueue();

		if (quitRequested){
			GameSystem::get().shutdown();
			window.close();
		}
	}
	return 0;
}
