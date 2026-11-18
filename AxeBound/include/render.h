#ifndef RENDER_H
#define RENDER_H

#include <SFML/Graphics.hpp>

// CHANGED: the duplicate init() / update() declarations and "extern square" were removed (init.h / update.h own those)

extern char renderMode; // 'T' = game scene (tilemap + HUD), 'M' = menu scene (set by GameSystem when the scene changes)

void render(sf::RenderWindow &window);

#endif
