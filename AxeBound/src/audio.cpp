// audio.cpp
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// Files (all optional, see the Media List in the design document):
//   assets/music/menu.ogg      (Ui Background)       assets/music/ambient.ogg (Background / Ambient)
//   assets/sounds/axe.wav      assets/sounds/footstep.wav

#include <SFML/Audio.hpp>
#include <string>
#include <memory>

#include "audio.h"
#include "saveManager.h"
#include "globals.h"

static sf::Music menuMusic;
static sf::Music ambientMusic;
static bool haveMenu = false, haveAmbient = false;
static sf::SoundBuffer buffers[SND_COUNT];
static sf::Sound sounds[SND_COUNT];
static bool haveSound[SND_COUNT] = {false, false};

static bool openMusic(sf::Music& m, const std::string& path)
{
	if (!m.openFromFile(path)) { gameLog.warn("Music not found (game continues silently): " + path); return false; }
	m.setLoop(true);
	return true;
}

void initAudio()
{
	haveMenu = openMusic(menuMusic, "assets/music/menu.ogg");
	haveAmbient = openMusic(ambientMusic, "assets/music/ambient.ogg");
	const char* files[SND_COUNT] = {"assets/sounds/axe.wav", "assets/sounds/footstep.wav"};
	for (int i = 0; i < SND_COUNT; i++){
		if (buffers[i].loadFromFile(files[i])) { sounds[i].setBuffer(buffers[i]); haveSound[i] = true; }
		else gameLog.warn(std::string("Sound not found (game continues silently): ") + files[i]);
	}
	updateAudioVolumes();
}

void updateAudioVolumes()
{
	if (haveMenu) menuMusic.setVolume(static_cast<float>(musicVolume));
	if (haveAmbient) ambientMusic.setVolume(static_cast<float>(musicVolume));
	for (int i = 0; i < SND_COUNT; i++) if (haveSound[i]) sounds[i].setVolume(static_cast<float>(soundVolume));
}

void playMenuMusic()
{
	if (haveAmbient) ambientMusic.stop();
	if (haveMenu && menuMusic.getStatus() != sf::SoundSource::Playing) menuMusic.play();
}

void playAmbientMusic()
{
	if (haveMenu) menuMusic.stop();
	if (haveAmbient && ambientMusic.getStatus() != sf::SoundSource::Playing) ambientMusic.play();
}

void playSound(soundId id)
{
	if (id < 0 || id >= SND_COUNT || !haveSound[id]) return;
	sounds[id].play();
}
