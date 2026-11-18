// tileMap.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// The tile map from the design document (one per level). Tiles are 16x16, the atlas texture is built in code.

#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

enum TileId : unsigned char { T_GRASS = 0, T_WATER, T_SAND, T_PATH, T_SNOW, T_HILL, T_BRIDGE, T_COUNT };

struct tile {            // "struct tile" in the design class diagram
	unsigned char tileId;
	int textureId;       // index in the atlas (same as tileId here)
	std::string name;
	bool solid;          // the player / trucks can not walk here (water, hills)
	bool cold;           // snow: frost builds up here
};
const tile& tileInfo(unsigned char id);
void buildTileTexture(sf::Texture& target); // makes the atlas from solid colours (design media list: "Solid green colour" ...)

class tileMap {
	private:
		int mapW = 0;
		int mapH = 0;
		std::vector<unsigned char> worldData;
		std::vector<signed char> flowX; // river direction per tile (0,0 = still water)
		std::vector<signed char> flowY;
		mutable sf::VertexArray verts;
		int index(int tx, int ty) const { return ty * mapW + tx; }
		bool inRange(int tx, int ty) const { return tx >= 0 && ty >= 0 && tx < mapW && ty < mapH; }
	public:
		tileMap();
		void create(int w, int h, unsigned char fill);
		int width() const { return mapW; }
		int height() const { return mapH; }
		unsigned char getTileAt(int tx, int ty) const;      // out of range counts as hill
		void updateTileAt(int tx, int ty, unsigned char id);
		void fillRect(int x, int y, int w, int h, unsigned char id);
		void setFlow(int x, int y, int w, int h, int dx, int dy);
		sf::Vector2f getFlow(int tx, int ty) const;
		bool isSolid(int tx, int ty) const;
		bool isCold(int tx, int ty) const;
		bool isWater(int tx, int ty) const;                  // water (things float there)
		sf::Vector2i getTileOnScreen(sf::Vector2f worldPos) const; // world pixels -> tile coordinates
		sf::Vector2f pixelSize() const { return sf::Vector2f(static_cast<float>(mapW * 16), static_cast<float>(mapH * 16)); }
		void draw(sf::RenderWindow& window, const sf::FloatRect& viewRect) const;
	};
