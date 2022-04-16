# Wordle DS
A clone of [Wordle](https://www.nytimes.com/games/wordle/index.html) for the Nintendo DS(i). Unless the NY Times changes the word list, this will have the same word each day as the official Wordle. A sharable text file will be made as `WordleDS.txt` on completion or can be accessed as a QR code from the statistics menu.

## Building
Follow devkitPro's [Getting Started](https://devkitpro.org/wiki/Getting_Started) guide to install libnds, devkitARM, and grit, then simply run `make`.

### Customizing
The word lists are defined in `words.cpp`. Any English word list should work as is. To use a language that needs letters not in English, add the letters to the `letters` array in `defines.hpp` and the `kbdKeys.png`/`letterTiles.png` images, then add the keyboard keys in `kbd.hpp`.

All graphics are in the `gfx` folder and should mostly work fine if the PNGs are edited. A couple have hardcoded palette tweaks at runtime so they may glitch out if the palettes are changed. The more complex images also have their [GIMP](https://www.gimp.org) files in `resources`, the font used is nintendo_NTLG-DB_001. The fonts are in `data` and can be edited using an [NFTR editor](https://pk11.us/nftr-editor/).

Most other things such as the word length, starting day, and any text strings are defined in `defines.hpp`.

You can use a custom song by converting a module (.it, .mod, .s3m, or .xm) to a maxmod soundbank using the following command and putting WordleDS.msl next to WordleDS.json. mmutil can be installed from [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started).

```
mmutil -d [module] -oWordleDS.msl
```

## Credits
- [Pk11](https://github.com/Epicmn11): This DS port
- [Josh Wardle](https://github.com/powerlanguage): The original Wordle
- [lifehackerhansol](https://github.com/lifehackerhansol): animatedbannerpatch.py from [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)
- [Rocket Robz](https://github.com/RocketRobz): Original music composition
- [devkitPro](https://github.com/devkitPro), [WinterMute](https://github.com/WinterMute): devkitARM, libnds, and libfat
- [Kentaro Fukuchi](https://github.com/fukuchi): [libqrencode](https://fukuchi.org/works/qrencode/index.html)
