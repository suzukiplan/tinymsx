# TinyMSX

TinyMSX is a multi-emulator core library for C/C++ that supports SG-1000, OthelloMultiVision and MSX1.

|                   SG-1000                   |             Othello MultiVision              |                  MSX1                  |
| :-----------------------------------------: | :------------------------------------------: | :------------------------------------: |
| ![SG-1000](doc/image/screenshot_sg1000.png) | ![OTHELLO](doc/image/screenshot_othello.png) | ![MSX1](doc/image/screenshot_msx1.png) |

## TODO

- [x] Emulate SG-1000
- [x] Emulate Othello Multivision (note: some input port are ignored)
- [x] Emulate MSX1 cartridge games
- [ ] Emulate MSX1 mega-rom cartridge games

## How to use

### Basic usage

```c++
#include "tinymsx.h"
```

```c++
    // Create an instance
    TinyMSX msx(TINYMSX_TYPE_MSX1, rom, romSize, TINYMSX_COLOR_MODE_RGB555);

    // Reset
    msx.reset();

    // Execute 1 frame
    msx.tick(0, 0);

    // Get display buffer (256 x 192 x 2 bytes)
    unsigned short* display = msx.vdp.display;

    // Get and clear the buffered audio data (44.1Hz/16bit/2ch) by tick execution.
    size_t soundSize;
    void* soundBuffer = msx.getSoundBuffer(&soundSize);

    // State save (quick save)
    size_t stateSize;
    const void* stateData = msx.saveState(&stateSize);

    // State load (quick load)
    msx.loadState(stateData, stateSize);
```

### Example

- [for macOS (Cocoa)](test/osx)
