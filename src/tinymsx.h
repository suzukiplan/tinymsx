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

class TinyMSX {
    private:
        struct MsxBIOS {
            unsigned char main[0x8000];
            unsigned char logo[0x4000];
        } bios;
        int type;
        unsigned char pad[2];
        unsigned char* rom;
        size_t romSize;
    public:
        unsigned short display[256 * 192];
        unsigned short palette[16];
        struct VideoDisplayProcessor {
            unsigned char ram[0x4000];
            unsigned char reg[8];
            unsigned char tmpAddr[2];
            unsigned short addr;
            unsigned char stat;
            unsigned char latch;
            unsigned char readBuffer;
        } vdp;
        struct ProgrammableSoundGenerator {
            int b;
            int i;
            unsigned int r[8];
            unsigned int c[4];
            unsigned int e[4];
            unsigned int np;
            unsigned int ns;
            unsigned int nx;
        } psg;
        unsigned char psgLevels[16];
        unsigned int psgCycle;
        struct MemoryRegister {
            unsigned char page[4];
            unsigned char slot[4]; // E * * * - B B E E
        } mem;
        struct InternalRegister {
            int frameClock;
            int lineClock;
            int lineNumber;
        } ir;
        unsigned char ram[0x4000];
        Z80* cpu;
        TinyMSX(int type, const void* rom, size_t romSize, int colorMode);
        ~TinyMSX();
        bool loadBiosFromFile(const char* path);
        bool loadBiosFromMemory(void* bios, size_t size);
        bool loadLogoFromFile(const char* path);
        bool loadLogoFromMemory(void* logo, size_t size);
        void reset();
        void tick(unsigned char pad1, unsigned char pad2);
        void* quickSave(size_t* size);
        void quickLoad(void* data, size_t size);
        inline int getVideoMode() { return (vdp.reg[0] & 0b000000010) | (vdp.reg[1] & 0b00010000) >> 4 | (vdp.reg[1] & 0b000001000) >> 1; }
        inline int getSlotNumber(int page) { return mem.slot[mem.page[page]] & 0b11; }
        inline int getExtSlotNumber(int page) { return (mem.slot[mem.page[page]] & 0b1100) >> 2; }

    private:
        inline void initBIOS();
        inline bool isSG1000() { return this->type == TINY_MSX_TYPE_SG1000; }
        inline bool isMSX1() { return this->type == TINY_MSX_TYPE_MSX1; }
        inline unsigned short getInitAddr();
        inline unsigned char readMemory(unsigned short addr);
        inline void writeMemory(unsigned short addr, unsigned char value);
        inline unsigned char inPort(unsigned char port);
        inline void outPort(unsigned char port, unsigned char value);
        inline unsigned char vdpReadData();
        inline unsigned char vdpReadStatus();
        inline void vdpWriteData(unsigned char value);
        inline void vdpWriteAddress(unsigned char value);
        inline void updateVdpAddress();
        inline void readVideoMemory();
        inline void updateVdpRegister();
        inline void psgWrite(unsigned char value);
        inline unsigned char psgRead();
        inline void psgCalc(short* left, short* right);
        inline void changeMemoryMap(int page, unsigned char map);
        inline void consumeClock(int clocks);
        inline void checkUpdateScanline();
        inline void drawScanline(int lineNumber);
        inline void drawScanlineMode0(int lineNumber);
        inline void drawScanlineMode2(int lineNumber);
        inline void drawScanlineMode3(int lineNumber);
        inline void drawEmptyScanline(int lineNumber);
        inline void drawSprites(int lineNumber);
        inline bool loadSpecificSizeFile(const char* path, void* buffer, size_t size);
};

#endif
