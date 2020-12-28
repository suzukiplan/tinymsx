/**
 * SUZUKI PLAN - TinyMSX
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Yoji Suzuki.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#ifndef INCLUDE_TINYMSX_H
#define INCLUDE_TINYMSX_H
#include <stdio.h>
#include "z80.hpp"
#include "tinymsx_def.h"
#include "msxslot.hpp"
#include "msxslot_gm2.hpp"
#include "tms9918a.hpp"
#include "sn76489.hpp"
#include "ay8910.hpp"

class TinyMSX {
    private:
        struct MsxBIOS {
            unsigned char main[0x8000];
        } bios;
        int type;
        unsigned char pad[2];
        unsigned char specialKeyX[2];
        unsigned char specialKeyY[2];
        unsigned char* rom;
        size_t romSize;
        size_t ramSize;
        short soundBuffer[65536];
        unsigned short soundBufferCursor;
        unsigned char tmpBuffer[1024 * 1024];
    public:
        TMS9918A vdp;
        SN76489 sn76489;
        AY8910 ay8910;
        unsigned char io[0x100];
        unsigned char ram[0x10000];
        MsxSlot slot;
        MsxSlotGM2 slotGM2;
        Z80* cpu;
        TinyMSX(int type, const void* rom, size_t romSize, size_t ramSize, int colorMode);
        ~TinyMSX();
        bool loadBiosFromFile(const char* path);
        bool loadBiosFromMemory(void* bios, size_t size);
        void setupSpecialKey1(unsigned char ascii, bool isTenKey = false);
        void setupSpecialKey2(unsigned char ascii, bool isTenKey = false);
        void reset();
        void tick(unsigned char pad1, unsigned char pad2);
        void* getSoundBuffer(size_t* size);
        const void* saveState(size_t* size);
        void loadState(const void* data, size_t size);

    private:
        inline void setupSpecialKeyV(int n, int x, int y) {
            this->specialKeyX[n] = x;
            this->specialKeyY[n] = y;
        }
        void setupSpecialKey(int n, unsigned char ascii, bool isTenKey);
        inline bool isSG1000() { return this->type == TINYMSX_TYPE_SG1000; }
        inline bool isMSX1() { return this->type == TINYMSX_TYPE_MSX1; }
        inline bool isMSX1_GameMaster2() { return this->type == TINYMSX_TYPE_MSX1_GameMaster2; }
        inline unsigned char readMemory(unsigned short addr);
        inline void writeMemory(unsigned short addr, unsigned char value);
        inline unsigned char inPort(unsigned char port);
        inline void outPort(unsigned char port, unsigned char value);
        inline void consumeClock(int clocks);
        inline bool loadSpecificSizeFile(const char* path, void* buffer, size_t size);
        size_t calcAvairableRamSize();
};

#endif
