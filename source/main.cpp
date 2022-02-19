#include "config.hpp"
#include "gfx.hpp"
#include "howto.hpp"
#include "kbd.hpp"
#include "settings.hpp"
#include "stats.hpp"
#include "tonccpy.h"
#include "words.hpp"

#include "bgBottom.h"

#include <algorithm>
#include <fat.h>
#include <map>
#include <nds.h>
#include <stdio.h>
#include <string>
#include <time.h>

std::vector<TilePalette> check(const std::string &guess, const std::string &answer, Kbd *kbd) {
	std::vector<TilePalette> res;
	res.resize(std::min(guess.size(), answer.size()));

	// Get map of letters for wrong location
	std::map<char, int> letters;
	for(char letter : answer) {
		letters[letter]++;
	}

	// First check for exact matches
	for(uint i = 0; i < res.size(); i++) {
		if(guess[i] == answer[i]) {
			res[i] = TilePalette::green;
			if(kbd)
				kbd->palette(guess[i], TilePalette::green);
			letters[guess[i]]--;
		}
	}
	// Then check for matches in the wrong location
	for(uint i = 0; i < res.size(); i++) {
		if(res[i] == TilePalette::green) {
			continue;
		} else if(letters[guess[i]] > 0) {
			res[i] = TilePalette::yellow;
			if(kbd && kbd->palette(guess[i]) != TilePalette::green)
				kbd->palette(guess[i], TilePalette::yellow);
			letters[guess[i]]--;
		} else {
			res[i] = TilePalette::gray;
			if(kbd && (kbd->palette(guess[i]) != TilePalette::yellow && kbd->palette(guess[i]) != TilePalette::green))
				kbd->palette(guess[i], TilePalette::gray);
		}
	}

	return res;
}

void makeTxt(Config &config, const std::string &answer) {
	FILE *file = fopen("WordleDS.txt", "w");

	if(file) {
		char str[64];
		sprintf(str, "Wordle DS %lld %d/%d%s\n\n", config.lastPlayed() - FIRST_DAY, config.boardState().size(), MAX_GUESSES, config.hardMode() ? "*" : "");
		fwrite(str, 1, strlen(str), file);
		for(const std::string &guess : config.boardState()) {
			toncset(str, 0, 64);

			const char *green = config.altPalette() ? "🟧" : "🟩";
			const char *yellow = config.altPalette() ? "🟦" : "🟨";

			std::vector<TilePalette> colors = check(guess, answer, nullptr);
			for(uint i = 0; i < colors.size(); i++)
				strcat(str, colors[i] == TilePalette::green ? green : (colors[i] == TilePalette::yellow ? yellow : "⬜"));

			strcat(str, "\n");

			fwrite(str, 1, strlen(str), file);
		}

		fclose(file);
	}

}

int main(void) {
	if(!fatInitDefault()) {
		consoleDemoInit();
		printf("FAT init failed.");
		while(1)
			swiWaitForVBlank();
	}

	Config config("WordleDS.json");

	initGraphics(config.altPalette());

	// Show howto if first game
	if(config.gamesPlayed() < 1)
		howtoMenu();

	// Get random word based on date
	time_t today = time(NULL) / 24 / 60 / 60;
	std::string answer = choices[(today - FIRST_DAY) % choices.size()];
	config.lastPlayed(today);

	Kbd kbd;

	u16 pressed, held;
	s8 key = NOKEY;
	touchPosition touch;
	std::string guess = "";
	int currentGuess = 0;
	int popupTimeout = -1;
	bool won = false, statsSaved = false;
	std::string knownLetters, knownPositions; // for hard mode
	for(int i = 0; i < WORD_LEN; i++)
		knownPositions += ' ';

	if(config.boardState().size() > 0) {
		std::vector<TilePalette> palettes;
		for(const std::string &guess : config.boardState()) {
			Sprite *sprite = &letterSprites[currentGuess * WORD_LEN];
			for(char letter : guess) {
				(sprite++)->palette(TilePalette::whiteDark).gfx(letterGfx[letter - 'a' + 1]);
			}
			std::vector<TilePalette> newPalettes = check(guess, answer, &kbd);
			palettes.reserve(palettes.size() + newPalettes.size());
			palettes.insert(palettes.end(), newPalettes.begin(), newPalettes.end());
			won = std::find_if_not(newPalettes.begin(), newPalettes.end(), [](TilePalette a) { return a == TilePalette::green; }) == newPalettes.end();
			Sprite::update(false);
			currentGuess++;
			if(currentGuess >= MAX_GUESSES)
				break;
		}
		flipSprites(letterSprites.data(), currentGuess * WORD_LEN, palettes);
	}

	if(currentGuess == MAX_GUESSES && !won)
		currentGuess++;

	if(!won && currentGuess <= MAX_GUESSES)
		kbd.show();
	else
		statsSaved = true; // an already completed game was loaded, don't re-save

	while(1) {
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
			touchRead(&touch);
			if(held & KEY_TOUCH)
				key = kbd.get();
			else
				key = Kbd::NOKEY;

			if(popupTimeout == 0)
				tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap, SCREEN_SIZE_TILES); // Normal
			if(popupTimeout >= 0)
				popupTimeout--;
		} while(!pressed && key == Kbd::NOKEY);

		// Process keyboard
		switch(key) {
			case Kbd::NOKEY:
				break;
			case Kbd::ENTER:
				// Ensure guess is a choice or valid guess
				if(std::find(choices.begin(), choices.end(), guess) != choices.end()
				|| std::binary_search(guesses.begin(), guesses.end(), guess)) {
					// check if meets hard mode requirements
					if(config.hardMode()) {
						bool valid = true;
						for(char letter : knownLetters) {
							if(std::count(knownLetters.begin(), knownLetters.end(), letter) != std::count(guess.begin(), guess.end(), letter)) {
								valid = false;
								break;
							}
						}
						for(uint i = 0; i < knownPositions.size(); i++) {
							if(knownPositions[i] != ' ' && guess[i] != knownPositions[i]) {
								valid = false;
								break;
							}
						}

						if(!valid) {
							tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap + (32 * 24) * 3, SCREEN_SIZE_TILES); // Invalid hard mode guess message
							popupTimeout = 120;
							break;
						}
					}

					// Find status of the letters
					std::vector<TilePalette> newPalettes = check(guess, answer, &kbd);
					won = std::find_if_not(newPalettes.begin(), newPalettes.end(), [](TilePalette a) { return a == TilePalette::green; }) == newPalettes.end();
					flipSprites(&letterSprites[currentGuess * WORD_LEN], WORD_LEN, newPalettes);
					Sprite::update(false);

					// Save info needed for hard mode
					if(config.hardMode()) {
						knownLetters = "";
						for(uint i = 0; i < guess.size(); i++) {
							if(newPalettes[i] == TilePalette::yellow)
								knownLetters += guess[i];
							else if(newPalettes[i] == TilePalette::green)
								knownPositions[i] = guess[i];
						}
					}

					config.boardState(guess);

					guess = "";
					currentGuess++;
				} else {
					if(guess.length() < WORD_LEN)
						tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap + (32 * 24), SCREEN_SIZE_TILES); // Too short message
					else
						tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap + (32 * 24) * 2, SCREEN_SIZE_TILES); // Not a word message
					popupTimeout = 120;
				}
				break;
			case Kbd::BACKSPACE:
				if(guess.length() > 0) {
					guess.pop_back();
					letterSprites[currentGuess * WORD_LEN + guess.length()].palette(TilePalette::white).gfx(letterGfx[0]).update();
				}
				break;
			default: // Letter
				if(guess.length() < WORD_LEN) {
					Sprite sprite = letterSprites[currentGuess * WORD_LEN + guess.length()];
					sprite.palette(TilePalette::whiteDark).gfx(letterGfx[key - 'a' + 1]).affineIndex(0, false);
					for(int i = 0; i < 6; i++) {
						swiWaitForVBlank();
						sprite.rotateScale(0, 1.1f - .1f / (6 - i), 1.1f - .1f / (6 - i)).update();
					}
					sprite.affineIndex(-1, false).update();
					guess += key;
				}
				break;
		}

		if(pressed & KEY_START) {
			config.save();
			return 0;
		}

		if(pressed & KEY_TOUCH) {
			if(touch.py < 24) { // One of the icons at the top
				bool showKeyboard = kbd.visible();
				if(showKeyboard)
					kbd.hide();

				if(touch.px < 24)
					howtoMenu();
				else if(touch.px > 116 && touch.px < 140)
					statsMenu(config, won);
				else if(touch.px > 232)
					settingsMenu(config);

				if(showKeyboard)
					kbd.show();
			}
		}

		if(!statsSaved && (won || currentGuess >= MAX_GUESSES)) {
			statsSaved = true;

			if(currentGuess == MAX_GUESSES && !won)
				currentGuess++;

			kbd.hide();

			// Update stats
			config.guessCounts(currentGuess);
			config.gamesPlayed(config.gamesPlayed() + 1);
			config.streak(config.streak() + 1);
			config.save();

			// Generate sharable txt
			makeTxt(config, answer);

			if(won) {
				tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap + (32 * 24) * 4, SCREEN_SIZE_TILES); // Win message
				for(int i = 0; i < 180; i++)
					swiWaitForVBlank();
			} else {
				tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap + (32 * 24) * 5, SCREEN_SIZE_TILES); // Lose message

				std::vector<Sprite> answerSprites;
				for(uint i = 0; i < answer.length(); i++) {
					answerSprites.emplace_back(false, SpriteSize_32x32, SpriteColorFormat_16Color);
					answerSprites.back()
						.move((((256 - (WORD_LEN * 24 + (WORD_LEN - 1) * 2)) / 2) - 4) + (i % WORD_LEN) * 26, 96)
						.palette(TilePalette::white)
						.gfx(letterGfxSub[answer[i] - 'a' + 1])
						.visible(false);
				}

				flipSprites(answerSprites.data(), answerSprites.size(), {}, FlipOptions::show);

				for(int i = 0; i < 180; i++)
					swiWaitForVBlank();

				flipSprites(answerSprites.data(), answerSprites.size(), {}, FlipOptions::hide);
			}

			// Show stats
			statsMenu(config, won);
		}
	}
}
