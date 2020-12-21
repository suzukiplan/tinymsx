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
#include "tms9918a.hpp"
#include "sn76489.hpp"
#include "ay8910.hpp"

class TinyMSX {
    private:
        struct MsxBIOS {
            unsigned char main[0x8000];
            unsigned char logo[0x4000];
        } bios;
        int type;
        unsigned char pad[2];
        unsigned char specialKeyX[2];
        unsigned char specialKeyY[2];
        unsigned char* rom;
        size_t romSize;
        short soundBuffer[65536];
        unsigned short soundBufferCursor;
        unsigned char tmpBuffer[1024 * 1024];
    public:
        TMS9918A vdp;
        SN76489 sn76489;
        AY8910 ay8910;
        struct MemoryRegister {
            unsigned char page[4];
            unsigned char slot[4]; // E * * * - B B E E
            unsigned char portAA; // keyboard position
        } mem;
        unsigned char ram[0x4000];
        MsxSlot slots;
        Z80* cpu;
        TinyMSX(int type, const void* rom, size_t romSize, int colorMode);
        ~TinyMSX();
        bool loadBiosFromFile(const char* path);
        bool loadBiosFromMemory(void* bios, size_t size);
        bool loadLogoFromFile(const char* path);
        bool loadLogoFromMemory(void* logo, size_t size);
        void setupSpecialKey1(unsigned char ascii, bool isTenKey = false);
        void setupSpecialKey2(unsigned char ascii, bool isTenKey = false);
        void reset();
        void tick(unsigned char pad1, unsigned char pad2);
        void* getSoundBuffer(size_t* size);
        const void* saveState(size_t* size);
        void loadState(const void* data, size_t size);
        inline int getSlotNumber(int page) { return mem.slot[mem.page[page]] & 0b11; }
        inline int getExtSlotNumber(int page) { return (mem.slot[mem.page[page]] & 0b1100) >> 2; }

    private:
        inline void setupSpecialKeyV(int n, int x, int y) {
            this->specialKeyX[n] = x;
            this->specialKeyY[n] = y;
        }
        void setupSpecialKey(int n, unsigned char ascii, bool isTenKey);
        inline void initBIOS();
        inline bool isSG1000() { return this->type == TINYMSX_TYPE_SG1000; }
        inline bool isMSX1() { return this->type == TINYMSX_TYPE_MSX1; }
        inline unsigned short getInitAddr();
        inline unsigned char readMemory(unsigned short addr);
        inline void setExtraPage(int slot, int extra);
        inline void writeMemory(unsigned short addr, unsigned char value);
        inline unsigned char inPort(unsigned char port);
        inline void outPort(unsigned char port, unsigned char value);
        inline void psgExec(int clocks);
        inline void changeMemoryMap(int page, unsigned char map);
        inline void consumeClock(int clocks);
        inline bool loadSpecificSizeFile(const char* path, void* buffer, size_t size);
        size_t calcAvairableRamSize();
};

#endif
