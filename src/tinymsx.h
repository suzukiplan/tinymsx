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
#include <stdio.h>
#include "z80.hpp"

#define TINY_MSX_COLOR_MODE_RGB555 0
#define TINY_MSX_COLOR_MODE_RGB565 1

class TinyMSX {
    private:
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
        struct InternalRegister {
            int frameClock;
            int lineClock;
            int lineNumber;
        } ir;
        unsigned char ram[0x2000];
        Z80* cpu;
        TinyMSX(void* rom, size_t romSize, int colorMode);
        ~TinyMSX();
        void reset();
        void* quickSave(size_t* size);
        void quickLoad(void* data, size_t size);

    private:
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
        inline int getVideoMode() { return ((vdp.reg[0] & 0b00001110) >> 1) + (vdp.reg[1] & 0b00011000); }
        inline void consumeClock(int clocks);
        inline void checkUpdateScanline();
        inline void drawScanline(int lineNumber);
        inline void drawScanlineMode0(int lineNumber);
        inline void drawScanlineMode2(int lineNumber);
        inline void drawScanlineMode3(int lineNumber);
        inline void drawEmptyScanline(int lineNumber);
        inline void drawSprites(int lineNumber);
};
