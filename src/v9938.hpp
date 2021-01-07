/**
 * SUZUKI PLAN - TinyMSX - V9938 Emulator (WIP)
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
#ifndef INCLUDE_V9938_HPP
#define INCLUDE_V9938_HPP

#include <string.h>

class V9938
{
  private:
    int colorMode;
    void* arg;
    void (*detectInterrupt)(void* arg, int ie);
    void (*detectBreak)(void* arg);

  public:
    unsigned short display[256 * 212];
    unsigned short palette[16];

    struct Context {
        int bobo;
        int countH;
        int countV;
        int reserved;
        unsigned char ram[0x20000];
        unsigned char reg[64];
        unsigned char pal[16][2];
        unsigned char tmpAddr[2];
        unsigned char stat[16];
        unsigned int addr;
        unsigned char latch;
        unsigned char readBuffer;
        unsigned char command;
        unsigned short commandX;
        unsigned short commandY;
    } ctx;

    V9938()
    {
        memset(palette, 0, sizeof(palette));
        this->reset();
    }

    void initialize(int colorMode, void* arg, void (*detectInterrupt)(void*, int), void (*detectBreak)(void*))
    {
        this->colorMode = colorMode;
        this->arg = arg;
        this->detectInterrupt = detectInterrupt;
        this->detectBreak = detectBreak;
        this->reset();
    }

    void reset()
    {
        static unsigned int rgb[16] = {0x000000, 0x000000, 0x3EB849, 0x74D07D, 0x5955E0, 0x8076F1, 0xB95E51, 0x65DBEF, 0xDB6559, 0xFF897D, 0xCCC35E, 0xDED087, 0x3AA241, 0xB766B5, 0xCCCCCC, 0xFFFFFF};
        memset(display, 0, sizeof(display));
        memset(&ctx, 0, sizeof(ctx));
        for (int i = 0; i < 16; i++) {
            this->ctx.pal[i][0] = 0;
            this->ctx.pal[i][1] = 0;
            this->ctx.pal[i][0] |= (rgb[i] & 0b111000000000000000000000) >> 17;
            this->ctx.pal[i][1] |= (rgb[i] & 0b000000001110000000000000) >> 13;
            this->ctx.pal[i][0] |= (rgb[i] & 0b000000000000000011100000) >> 5;
            updatePaletteCacheFromRegister(i);
        }
    }

    inline int getVideoMode()
    {
        int mode = 0;
        if (ctx.reg[1] & 0b00010000) mode |= 0b00001;
        if (ctx.reg[1] & 0b00001000) mode |= 0b00010;
        if (ctx.reg[0] & 0b00000010) mode |= 0b00100;
        if (ctx.reg[0] & 0b00000100) mode |= 0b01000;
        if (ctx.reg[0] & 0b00001000) mode |= 0b10000;
        return mode;
    }

    inline bool isExpansionRAM() { return ctx.reg[45] & 0b01000000 ? true : false; }
    inline int getVramSize() { return 0x20000; }
    inline bool isEnabledExternalVideoInput() { return ctx.reg[0] & 0b00000001 ? true : false; }
    inline bool isEnabledScreen() { return ctx.reg[1] & 0b01000000 ? true : false; }
    inline bool isEnabledInterrupt0() { return ctx.reg[1] & 0b00100000 ? true : false; }
    inline bool isEnabledInterrupt1() { return ctx.reg[0] & 0b00010000 ? true : false; }
    inline bool isEnabledInterrupt2() { return ctx.reg[0] & 0b00100000 ? true : false; }
    inline unsigned short getBackdropColor() { return palette[ctx.reg[7] & 0b00001111]; }

    inline void tick()
    {
        this->ctx.countH++;
        if (37 == this->ctx.countH) {
            this->ctx.stat[2] &= 0b11011111; // reset HR flag
            if (this->isEnabledScreen() && this->isEnabledInterrupt1()) {
                this->ctx.stat[1] |= this->ctx.countV == this->ctx.reg[19] ? 0x01 : 0x00;
                this->detectInterrupt(this->arg, 1);
            }
        } else if (293 == this->ctx.countH) {
            this->ctx.stat[2] |= 0b00100000; // set HR flag
            this->renderScanline(this->ctx.countV - 40);
        } else if (342 == this->ctx.countH) {
            this->ctx.countH -= 342;
            switch (++this->ctx.countV) {
                case 251:
                    this->ctx.stat[0] |= 0x80;
                    if (this->isEnabledInterrupt0()) {
                        this->detectInterrupt(this->arg, 0);
                    }
                    break;
                case 262:
                    this->ctx.countV -= 262;
                    this->detectBreak(this->arg);
                    break;
            }
        }
    }

    inline unsigned char readPort0()
    {
        unsigned char result = this->ctx.readBuffer;
        this->readVideoMemory();
        this->ctx.latch = 0;
        return result;
    }

    inline unsigned char readPort1()
    {
        int sn = this->ctx.reg[15] & 0b00001111;
        unsigned char result = this->ctx.stat[sn];
        switch (sn) {
            case 0: this->ctx.stat[sn] &= 0b01011111; break;
            case 1: this->ctx.stat[sn] &= 0b11111110; break;
            case 2: result |= 0b10001100; break;
        }
        this->ctx.latch = 0;
        return result;
    }

    inline void writePort0(unsigned char value)
    {
        this->ctx.addr &= this->getVramSize() - 1;
        this->ctx.readBuffer = value;
        this->ctx.ram[this->ctx.addr] = this->ctx.readBuffer;
        this->ctx.addr++;
        this->ctx.latch = 0;
    }

    inline void writePort1(unsigned char value)
    {
        this->ctx.latch &= 1;
        this->ctx.tmpAddr[this->ctx.latch++] = value;
        if (2 == this->ctx.latch) {
            if ((this->ctx.tmpAddr[1] & 0b11000000) == 0b10000000) {
                // Direct access to VDP registers
                this->updateRegister(this->ctx.tmpAddr[1] & 0b00111111, this->ctx.tmpAddr[0]);
            } else if (this->ctx.tmpAddr[1] & 0b01000000) {
                this->updateAddress();
            } else {
                this->updateAddress();
                this->readVideoMemory();
            }
        } else if (1 == this->ctx.latch) {
            this->ctx.addr &= 0xFF00;
            this->ctx.addr |= this->ctx.tmpAddr[0];
        }
    }

    inline void writePort2(unsigned char value)
    {
        this->ctx.latch &= 1;
        int pn = this->ctx.reg[16] & 0b00001111;
        this->ctx.pal[pn][this->ctx.latch++] = value;
        if (2 == this->ctx.latch) {
            updatePaletteCacheFromRegister(pn);
            this->ctx.reg[16]++;
            this->ctx.reg[16] &= 0b00001111;
        }
    }

    inline void writePort3(unsigned char value)
    {
        // Indirect access to registers through R#17 (Control Register Pointer)
        this->updateRegister(this->ctx.reg[17] & 0b00111111, value);
        if ((this->ctx.reg[17] & 0b11000000) == 0b00000000) {
            this->ctx.reg[17]++;
            this->ctx.reg[17] &= 0b00111111;
        }
    }

  private:
    inline int getLineNumber()
    {
        switch (this->getVideoMode()) {
            case 0b00000: return 192; // G1
            case 0b00100: return 192; // G2
            case 0b01000: return 192; // G3
            case 0b00001: return 192; // TEXT1
            default: return this->ctx.reg[9] & 0x80 ? 212 : 192;
        }
    }

    inline void renderScanline(int lineNumber)
    {
        if (0 <= lineNumber && lineNumber < this->getLineNumber()) {
            this->ctx.stat[2] &= 0b10111111; // reset VR flag
            if (this->isEnabledScreen()) {
                switch (this->getVideoMode()) {
                    case 0b00000: this->renderScanlineModeG1(lineNumber); break;
                    case 0b00100: this->renderScanlineModeG2(lineNumber, false); break;
                    case 0b01000: this->renderScanlineModeG2(lineNumber, false); break;
                    case 0b01100: this->renderScanlineModeG4(lineNumber); break;
                    case 0b10000: this->renderScanlineModeG5(lineNumber); break;
                    case 0b10100: this->renderScanlineModeG6(lineNumber); break;
                    case 0b11100: this->renderScanlineModeG7(lineNumber); break;
                    case 0b00010: this->renderEmptyScanline(lineNumber); break; // MULTI COLOR (unimplemented)
                    case 0b00001: this->renderEmptyScanline(lineNumber); break; // TEXT1 (unimplemented)
                    case 0b01001: this->renderEmptyScanline(lineNumber); break; // TEXT2 (unimplemented)
                    default: this->renderEmptyScanline(lineNumber);
                }
            } else {
                this->renderEmptyScanline(lineNumber);
            }
        } else {
            this->ctx.stat[2] |= 0b01000000; // set VR flag
        }
    }

    inline void updatePaletteCacheFromRegister(int pn)
    {
        unsigned short r = this->ctx.pal[pn][0] & 0b01110000;
        unsigned short b = this->ctx.pal[pn][0] & 0b00000111;
        unsigned short g = this->ctx.pal[pn][1] & 0b00000111;
        switch (this->colorMode) {
            case 0: // RGB555
                r <<= 8;
                g <<= 7;
                b <<= 2;
                break;
            case 1: // RGB565
                r <<= 9;
                g <<= 8;
                b <<= 2;
                break;
            default:
                r = 0;
                g = 0;
                b = 0;
        }
        this->palette[pn] = r | g | b;
    }

    inline void updateAddress()
    {
        this->ctx.addr = this->ctx.tmpAddr[1];
        this->ctx.addr <<= 8;
        this->ctx.addr |= this->ctx.tmpAddr[0];
        unsigned int ha = this->ctx.reg[14] & 0b00000111;
        ha <<= 14;
        this->ctx.addr += ha;
    }

    inline void readVideoMemory()
    {
        this->ctx.addr &= this->getVramSize() - 1;
        this->ctx.readBuffer = this->ctx.ram[this->ctx.addr & 0x1FFFF];
        this->ctx.addr++;
    }

    inline void updateRegister(int rn, unsigned char value)
    {
#ifdef DEBUG
        int previousMode = 0;
        if (ctx.reg[1] & 0b00010000) previousMode |= 1; // Mode 1
        if (ctx.reg[0] & 0b00000010) previousMode |= 2; // Mode 2
        if (ctx.reg[1] & 0b00001000) previousMode |= 4; // Mode 3
        int vramSize = getVramSize();
        bool screen = this->isEnabledScreen();
        bool externalVideoInput = this->isEnabledExternalVideoInput();
#endif
        bool previousInterrupt = this->isEnabledInterrupt0();
        this->ctx.reg[rn] = value;
        if (!previousInterrupt && this->isEnabledInterrupt0() && this->ctx.stat[0] & 0x80) {
            this->detectInterrupt(this->arg, 0);
        }
        if (44 == rn && this->ctx.command) {
            switch (this->ctx.commandX) {
                case 0b1111: this->executeCommandHMMC(); break;
            }
        } else if (46 == rn) {
            this->executeCommand((value & 0xF0) >> 4, value & 0x0F);
        }
#ifdef DEBUG
        int currentMode = 0;
        if (ctx.reg[1] & 0b00010000) currentMode |= 1; // Mode 1
        if (ctx.reg[0] & 0b00000010) currentMode |= 2; // Mode 2
        if (ctx.reg[1] & 0b00001000) currentMode |= 4; // Mode 3
        if (previousMode != currentMode) {
            printf("Change display mode: %d -> %d\n", previousMode, currentMode);
        }
        if (vramSize != getVramSize()) {
            printf("Change VDP RAM size: %d -> %d\n", vramSize, getVramSize());
        }
        if (screen != this->isEnabledScreen()) {
            printf("Change VDP screen enabled: %s\n", this->isEnabledScreen() ? "ENABLED" : "DISABLED");
        }
        if (externalVideoInput != this->isEnabledExternalVideoInput()) {
            printf("Change VDP external video input enabled: %s\n", this->isEnabledExternalVideoInput() ? "ENABLED" : "DISABLED");
        }
#endif
    }

    inline void renderScanlineModeG1(int lineNumber)
    {
        int pn = (this->ctx.reg[2] & 0b00001111) << 10;
        int ct = this->ctx.reg[3] << 6;
        int pg = (this->ctx.reg[4] & 0b00000111) << 11;
        int bd = this->ctx.reg[7] & 0b00001111;
        int pixelLine = lineNumber % 8;
        unsigned char* nam = &this->ctx.ram[pn + lineNumber / 8 * 32];
        int cur = lineNumber * 256;
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx.ram[pg + nam[i] * 8 + pixelLine];
            unsigned char c = this->ctx.ram[ct + nam[i] / 8];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[1] = cc[1] ? cc[1] : bd;
            cc[0] = c & 0x0F;
            cc[0] = cc[0] ? cc[0] : bd;
            this->display[cur++] = this->palette[cc[(ptn & 0b10000000) >> 7]];
            this->display[cur++] = this->palette[cc[(ptn & 0b01000000) >> 6]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00100000) >> 5]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00010000) >> 4]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00001000) >> 3]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00000100) >> 2]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00000010) >> 1]];
            this->display[cur++] = this->palette[cc[ptn & 0b00000001]];
        }
        renderSpritesMode1(lineNumber);
    }

    inline void renderScanlineModeG2(int lineNumber, bool isSpriteMode2)
    {
        int pn = (this->ctx.reg[2] & 0b00001111) << 10;
        int ct = (this->ctx.reg[3] & 0b10000000) << 6;
        int cmask = this->ctx.reg[3] & 0b01111111;
        cmask <<= 3;
        cmask |= 0x07;
        int pg = (this->ctx.reg[4] & 0b00000100) << 11;
        int pmask = this->ctx.reg[4] & 0b00000011;
        pmask <<= 8;
        pmask |= 0xFF;
        int bd = this->ctx.reg[7] & 0b00001111;
        int pixelLine = lineNumber % 8;
        unsigned char* nam = &this->ctx.ram[pn + lineNumber / 8 * 32];
        int cur = lineNumber * 256;
        int ci = (lineNumber / 64) * 256;
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx.ram[pg + ((nam[i] + ci) & pmask) * 8 + pixelLine];
            unsigned char c = this->ctx.ram[ct + ((nam[i] + ci) & cmask) * 8 + pixelLine];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[1] = cc[1] ? cc[1] : bd;
            cc[0] = c & 0x0F;
            cc[0] = cc[0] ? cc[0] : bd;
            this->display[cur++] = this->palette[cc[(ptn & 0b10000000) >> 7]];
            this->display[cur++] = this->palette[cc[(ptn & 0b01000000) >> 6]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00100000) >> 5]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00010000) >> 4]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00001000) >> 3]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00000100) >> 2]];
            this->display[cur++] = this->palette[cc[(ptn & 0b00000010) >> 1]];
            this->display[cur++] = this->palette[cc[ptn & 0b00000001]];
        }
        if (isSpriteMode2) {
            renderSpritesMode2(lineNumber);
        } else {
            renderSpritesMode1(lineNumber);
        }
    }

    inline void renderScanlineModeG4(int lineNumber)
    {
        int pn = (this->ctx.reg[2] & 0b01111111) << 10;
        int curD = lineNumber * 256;
        int curP = lineNumber * 128;
        for (int i = 0; i < 128; i++) {
            this->display[curD++] = this->palette[(this->ctx.ram[curP] & 0xF0) >> 4];
            this->display[curD++] = this->palette[this->ctx.ram[curP++] & 0x0F];
        }
        renderSpritesMode2(lineNumber);
    }

    inline void renderScanlineModeG5(int lineNumber)
    {
        renderEmptyScanline(lineNumber);
        renderSpritesMode2(lineNumber);
    }

    inline void renderScanlineModeG6(int lineNumber)
    {
        renderEmptyScanline(lineNumber);
        renderSpritesMode2(lineNumber);
    }

    inline unsigned short convertColor_8bit_to_16bit(unsigned char c)
    {
        switch (this->colorMode) {
            case 0: {
                unsigned short result = (c & 0b00011100) << 10;
                result |= (c & 0b11100000) >> 3;
                result |= (c & 0b00000011) << 3;
                return result;
            }
            case 1: {
                unsigned short result = (c & 0b00011100) << 11;
                result |= (c & 0b11100000) >> 2;
                result |= (c & 0b00000011) << 3;
                return result;
            }
            default: return 0;
        }
    }

    inline void renderScanlineModeG7(int lineNumber)
    {
        int pn = (this->ctx.reg[2] & 0b01111111) << 10;
        int curD = lineNumber * 256;
        int curP = lineNumber * 256;
        for (int i = 0; i < 256; i++) {
            this->display[curD++] = convertColor_8bit_to_16bit(this->ctx.ram[curP++]);
        }
        renderSpritesMode2(lineNumber);
    }

    inline void renderEmptyScanline(int lineNumber)
    {
        int bd = this->getBackdropColor();
        int cur = lineNumber * 256;
        for (int i = 0; i < 256; i++) {
            this->display[cur++] = palette[bd];
        }
    }

    inline void renderSpritesMode1(int lineNumber)
    {
        static const unsigned char bit[8] = {
            0b10000000,
            0b01000000,
            0b00100000,
            0b00010000,
            0b00001000,
            0b00000100,
            0b00000010,
            0b00000001};
        bool si = this->ctx.reg[1] & 0b00000010 ? true : false;
        bool mag = this->ctx.reg[1] & 0b00000001 ? true : false;
        int sa = (this->ctx.reg[5] & 0b01111111) << 7;
        int sg = (this->ctx.reg[6] & 0b00000111) << 11;
        int sn = 0;
        unsigned char dlog[256];
        unsigned char wlog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        for (int i = 0; i < 32; i++) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx.ram[cur++];
            if (208 == y) break;
            unsigned char x = this->ctx.ram[cur++];
            unsigned char ptn = this->ctx.ram[cur++];
            unsigned char col = this->ctx.ram[cur++];
            if (col & 0x80) x -= 32;
            col &= 0b00001111;
            y++;
            if (mag) {
                if (si) {
                    // 16x16 x 2
                    if (y <= lineNumber && lineNumber < y + 32) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        int dcur = lineNumber * 256;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                        cur += 16;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                } else {
                    // 8x8 x 2
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        int dcur = lineNumber * 256;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                }
            } else {
                if (si) {
                    // 16x16 x 1
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        int dcur = lineNumber * 256;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                        cur += 16;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                } else {
                    // 8x8 x 1
                    if (y <= lineNumber && lineNumber < y + 8) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        int dcur = lineNumber * 256;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    inline void renderSpritesMode2(int lineNumber)
    {
        // TODO
    }

    inline void executeCommand(int cm, int lo)
    {
        if (cm) {
            if (this->ctx.stat[2] & 0b00000001) return; // already executing
            this->ctx.command = cm;
            this->ctx.commandX = 0;
            this->ctx.commandY = 0;
            this->ctx.stat[2] |= 0b00000001;
            switch (cm) {
                case 0b1111: this->executeCommandHMMC(); break;
                case 0b1110: this->executeCommandYMMM(); break;
                case 0b1101: this->executeCommandHMMM(); break;
                case 0b1100: this->executeCommandHMMV(); break;
                case 0b1011: this->executeCommandLMMC(lo); break;
                case 0b1010: this->executeCommandLMCM(lo); break;
                case 0b1001: this->executeCommandLMMM(lo); break;
                case 0b1000: this->executeCommandLMMV(lo); break;
                case 0b0111: this->executeCommandLINE(lo); break;
                case 0b0110: this->executeCommandSRCH(); break;
                case 0b0101: this->executeCommandPSET(lo); break;
                case 0b0100: this->executeCommandPOINT(); break;
            }
        } else {
            this->ctx.stat[2] &= 0b11111110;
        }
    }

    inline unsigned char logicalOperation(int lo, unsigned char dc, unsigned char sc)
    {
        if ((lo & 0b1000) && sc == 0) return dc;
        switch (lo & 0b0111) {
            case 0b000: return sc;
            case 0b001: return dc & sc;
            case 0b010: return dc | sc;
            case 0b011: return dc ^ sc;
            case 0b100: return ~sc;
            default: return dc;
        }
    }

    inline unsigned short getInt16FromRegister(int rn)
    {
        unsigned short result = this->ctx.reg[rn + 1];
        result <<= 8;
        result |= this->ctx.reg[rn];
        return result;
    }

    inline unsigned short getSourceX() { return this->getInt16FromRegister(32) & 0x01FF; }
    inline unsigned short getSourceY() { return this->getInt16FromRegister(34) & 0x03FF; }
    inline unsigned short getDestinationX() { return this->getInt16FromRegister(36) & 0x01FF; }
    inline unsigned short getDestinationY() { return this->getInt16FromRegister(38) & 0x03FF; }
    inline unsigned short getNumberOfDotsX() { return this->getInt16FromRegister(40) & 0x01FF; }
    inline unsigned short getNumberOfDotsY() { return this->getInt16FromRegister(42) & 0x03FF; }

    inline void getEdge(int* ex, int* ey, int* dotsPerByte)
    {
        switch (this->getVideoMode()) {
            case 0b01100: // G4
                *ex = 256;
                *ey = 1024;
                *dotsPerByte = 2;
                break;
            case 0b10000: // G5
                *ex = 512;
                *ey = 1024;
                *dotsPerByte = 4;
                break;
            case 0b10100: // G6
                *ex = 256;
                *ey = 512;
                *dotsPerByte = 2;
                break;
            case 0b11100: // G7
                *ex = 256;
                *ey = 512;
                *dotsPerByte = 1;
                break;
            default:
                *ex = 0;
                *ey = 0;
                *dotsPerByte = 1;
        }
    }

    inline int getDestinationAddr(int dx, int dy, int ox, int oy)
    {
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        dx += ox;
        dy += oy;
        while (ex <= dx) dx -= ex;
        while (ex < 0) dx += ex;
        while (ey <= dy) dy -= ey;
        while (dy < 0) dy += ey;
        dx /= dpb;
        switch (this->getVideoMode()) {
            case 0b01100: return dy * 128 + dx;
            case 0b10000: return dy * 128 + dx;
            case 0b10100: return dy * 128 + dx;
            case 0b11100: return dy * 256 + dx;
            default: return 0;
        }
    }

    inline int getCommandAddX()
    {
        switch (this->getVideoMode()) {
            case 0b01100: return 2; // G4
            case 0b10000: return 4; // G5
            case 0b10100: return 2; // G6
            case 0b11100: return 1; // G7
            default: return 0;
        }
    }

    inline void executeCommandHMMC()
    {
        int ax = this->getCommandAddX();
        switch (ax) {
            case 2:
                this->ctx.reg[36] &= 0b11111110;
                this->ctx.reg[40] &= 0b11111110;
                break;
            case 4:
                this->ctx.reg[36] &= 0b11111100;
                this->ctx.reg[40] &= 0b11111100;
                break;
        }
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        int ox = this->ctx.reg[45] & 0b000000100 ? -this->ctx.commandX : this->ctx.commandX;
        int oy = this->ctx.reg[45] & 0b000001000 ? -this->ctx.commandY : this->ctx.commandY;
        // NOTE: in fact, the transfer is not completed immediatly, but it is completed immediatly.
        this->ctx.ram[this->getDestinationAddr(dx, dy, ox, oy)] = this->ctx.reg[44];
        this->ctx.commandX += ax;
        if (this->getNumberOfDotsX() == this->ctx.commandX) {
            this->ctx.commandX = 0;
            this->ctx.commandY++;
            if (this->getNumberOfDotsY() == this->ctx.commandY) {
                this->ctx.command = 0;
                this->ctx.stat[2] &= 0b11111110;
            }
        }
    }

    inline void executeCommandYMMM()
    {
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        switch (this->getCommandAddX()) {
            case 2: this->ctx.reg[36] &= 0b11111110; break;
            case 4: this->ctx.reg[36] &= 0b11111100; break;
        }
        int sy = this->getSourceY();
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        int ny = this->getNumberOfDotsY();
        int baseX = this->ctx.reg[45] & 0b000000100 ? 0 : dx;
        int size = baseX ? ex - baseX : baseX;
        size /= dpb;
        // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
        while (ny--) {
            memmove(&this->ctx.ram[this->getDestinationAddr(baseX, dy, 0, 0)], &this->ctx.ram[this->getDestinationAddr(baseX, sy, 0, 0)], size);
            if (this->ctx.reg[45] & 0b000001000) {
                dy--;
                sy--;
                if (dy < 0) dy += ey;
                if (sy < 0) sy += ey;
            } else {
                dy++;
                sy++;
                if (ey <= dy) dy -= ey;
                if (ey <= sy) sy -= ey;
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
    }

    inline void executeCommandHMMM()
    {
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        switch (dpb) {
            case 2:
                this->ctx.reg[32] &= 0b11111110;
                this->ctx.reg[36] &= 0b11111110;
                this->ctx.reg[40] &= 0b11111110;
                break;
            case 4:
                this->ctx.reg[32] &= 0b11111100;
                this->ctx.reg[36] &= 0b11111100;
                this->ctx.reg[40] &= 0b11111100;
                break;
        }
        int sx = this->getSourceX();
        int sy = this->getSourceY();
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        int nx = this->getNumberOfDotsX();
        int ny = this->getNumberOfDotsY();
        int baseX = this->ctx.reg[45] & 0b000000100 ? sx - nx : sx;
        while (baseX < 0) baseX += ex;
        int size = nx / dpb;
        if (ex - dx < size) size = ex - dx;
        // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
        while (ny--) {
            memmove(&this->ctx.ram[this->getDestinationAddr(dx, dy, 0, 0)], &this->ctx.ram[this->getDestinationAddr(baseX, sy, 0, 0)], size);
            if (this->ctx.reg[45] & 0b000001000) {
                dy--;
                sy--;
                if (dy < 0) dy += ey;
                if (sy < 0) sy += ey;
            } else {
                dy++;
                sy++;
                if (ey <= dy) dy -= ey;
                if (ey <= sy) sy -= ey;
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
    }

    inline void executeCommandHMMV() {}
    inline void executeCommandLMMC(int lo) {}
    inline void executeCommandLMCM(int lo) {}
    inline void executeCommandLMMM(int lo) {}
    inline void executeCommandLMMV(int lo) {}
    inline void executeCommandLINE(int lo) {}
    inline void executeCommandSRCH() {}
    inline void executeCommandPSET(int lo) {}
    inline void executeCommandPOINT() {}
};

#endif // INCLUDE_V9938_HPP
