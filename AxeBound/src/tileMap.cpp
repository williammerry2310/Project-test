// tileMap.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026

#include <algorithm>
#include <cmath>

#include "tileMap.h"
#include "globals.h"

// --- tile table
static const tile tileTable[T_COUNT] = {
	{T_GRASS,  0, "Grass",  false, false},
	{T_WATER,  1, "Water",  true,  false},
	{T_SAND,   2, "Sand",   false, false},
	{T_PATH,   3, "Path",   false, false},
	{T_SNOW,   4, "Snow",   false, true},
	{T_HILL,   5, "Hill",   true,  false},
	{T_BRIDGE, 6, "Bridge", false, false}
};

const tile& tileInfo(unsigned char id)
{
	if (id >= T_COUNT) id = T_HILL;
	return tileTable[id];
}

// --- atlas: every tile is a plain colour (design media list), with a few pixels of detail
void buildTileTexture(sf::Texture& target)
{
	sf::Image img;
	img.create(tileSize * T_COUNT, tileSize, sf::Color::Magenta);
	for (int t = 0; t < T_COUNT; t++){
		for (int y = 0; y < tileSize; y++){
			for (int x = 0; x < tileSize; x++){
				sf::Color c = palette::grass;
				switch (t){
					case T_GRASS: c = palette::grass; break;
					case T_WATER:
						c = palette::water;
						if ((x * 7 + y * 3) % 23 == 0) c = sf::Color(0x6f, 0xa0, 0xf0); // ripples
						break;
					case T_SAND: c = palette::sand; break;
					case T_PATH: { // repeating brick pattern
						c = palette::brick;
						int row = y / 4;
						bool mortarRow = (y % 4 == 3);
						bool mortarCol = ((x + (row % 2) * 4) % 8 == 7);
						if (mortarRow || mortarCol) c = sf::Color(0x8f, 0x44, 0x26);
						break; }
					case T_SNOW:
						c = palette::snow;
						if ((x * 5 + y * 11) % 29 == 0) c = sf::Color(0xc4, 0xdb, 0xe8);
						break;
					case T_HILL:
						c = palette::hill;
						if ((x * 3 + y * 5) % 17 == 0) c = sf::Color(0x3a, 0x55, 0x24);
						break;
					case T_BRIDGE:
						c = palette::bridge;
						if (y % 4 == 3) c = sf::Color(0x7a, 0x3f, 0x04);
						break;
				}
				img.setPixel(t * tileSize + x, y, c);
			}
		}
	}
	target.loadFromImage(img);
	target.setSmooth(false);
}

// --- tileMap
tileMap::tileMap() : verts(sf::Quads) {}

void tileMap::create(int w, int h, unsigned char fill)
{
	mapW = w; mapH = h;
	worldData.assign(static_cast<size_t>(w) * h, fill);
	flowX.assign(static_cast<size_t>(w) * h, 0);
	flowY.assign(static_cast<size_t>(w) * h, 0);
}

unsigned char tileMap::getTileAt(int tx, int ty) const
{
	if (!inRange(tx, ty)) return T_HILL;
	return worldData[index(tx, ty)];
}

void tileMap::updateTileAt(int tx, int ty, unsigned char id)
{
	if (!inRange(tx, ty)) return;
	worldData[index(tx, ty)] = id;
}

void tileMap::fillRect(int x, int y, int w, int h, unsigned char id)
{
	for (int yy = y; yy < y + h; yy++)
		for (int xx = x; xx < x + w; xx++)
			updateTileAt(xx, yy, id);
}

void tileMap::setFlow(int x, int y, int w, int h, int dx, int dy)
{
	for (int yy = y; yy < y + h; yy++){
		for (int xx = x; xx < x + w; xx++){
			if (!inRange(xx, yy)) continue;
			flowX[index(xx, yy)] = static_cast<signed char>(dx);
			flowY[index(xx, yy)] = static_cast<signed char>(dy);
		}
	}
}

sf::Vector2f tileMap::getFlow(int tx, int ty) const
{
	if (!inRange(tx, ty)) return sf::Vector2f(0.0f, 0.0f);
	return sf::Vector2f(static_cast<float>(flowX[index(tx, ty)]), static_cast<float>(flowY[index(tx, ty)]));
}

bool tileMap::isSolid(int tx, int ty) const { return tileInfo(getTileAt(tx, ty)).solid; }
bool tileMap::isCold(int tx, int ty) const { return tileInfo(getTileAt(tx, ty)).cold; }
bool tileMap::isWater(int tx, int ty) const { return getTileAt(tx, ty) == T_WATER; }

sf::Vector2i tileMap::getTileOnScreen(sf::Vector2f worldPos) const
{
	return sf::Vector2i(static_cast<int>(std::floor(worldPos.x / tileSize)), static_cast<int>(std::floor(worldPos.y / tileSize)));
}

// only the tiles inside the camera are drawn (one vertex array, one draw call)
void tileMap::draw(sf::RenderWindow& window, const sf::FloatRect& viewRect) const
{
	int x0 = std::max(0, static_cast<int>(std::floor(viewRect.left / tileSize)));
	int y0 = std::max(0, static_cast<int>(std::floor(viewRect.top / tileSize)));
	int x1 = std::min(mapW - 1, static_cast<int>(std::floor((viewRect.left + viewRect.width) / tileSize)));
	int y1 = std::min(mapH - 1, static_cast<int>(std::floor((viewRect.top + viewRect.height) / tileSize)));
	verts.clear();
	for (int y = y0; y <= y1; y++){
		for (int x = x0; x <= x1; x++){
			float tx = static_cast<float>(worldData[index(x, y)] * tileSize);
			float px = static_cast<float>(x * tileSize);
			float py = static_cast<float>(y * tileSize);
			float s = static_cast<float>(tileSize);
			verts.append(sf::Vertex(sf::Vector2f(px, py),         sf::Vector2f(tx, 0.0f)));
			verts.append(sf::Vertex(sf::Vector2f(px + s, py),     sf::Vector2f(tx + s, 0.0f)));
			verts.append(sf::Vertex(sf::Vector2f(px + s, py + s), sf::Vector2f(tx + s, s)));
			verts.append(sf::Vertex(sf::Vector2f(px, py + s),     sf::Vector2f(tx, s)));
		}
	}
	window.draw(verts, &tileTexture);
}
