// audio.h
//
// Games Engineer Group Project - Set09121
//
// Group H
// 10-2026
//
// Music + sound effects from the Media List in the design document.
// The audio files are optional: if a file is missing the game logs a warning and carries on silently.

#pragma once

enum soundId { SND_AXE = 0, SND_FOOTSTEP, SND_COUNT };

void initAudio();
void playMenuMusic();
void playAmbientMusic();
void playSound(soundId id);
void updateAudioVolumes();
