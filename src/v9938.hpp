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
    void (*detectBlank)(void* arg);
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
    } ctx;
    unsigned char exRam[0x10000];

    V9938()
    {
        memset(palette, 0, sizeof(palette));
        this->reset();
    }

    void initialize(int colorMode, void* arg, void (*detectBlank)(void*), void (*detectBreak)(void*))
    {
        this->colorMode = colorMode;
        this->arg = arg;
        this->detectBlank = detectBlank;
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
    inline bool isEnabledInterrupt() { return ctx.reg[1] & 0b00100000 ? true : false; }
    inline unsigned short getBackdropColor() { return palette[ctx.reg[7] & 0b00001111]; }

    inline void tick()
    {
        this->ctx.countH++;
        if (293 == this->ctx.countH) {
            this->renderScanline(this->ctx.countV - 40);
        } else if (342 == this->ctx.countH) {
            this->ctx.countH -= 342;
            switch (++this->ctx.countV) {
                case 251:
                    this->ctx.stat[0] |= 0x80;
                    if (this->isEnabledInterrupt()) {
                        this->detectBlank(this->arg);
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
        if (0 == sn) this->ctx.stat[0] &= 0b01011111;
        this->ctx.latch = 0;
        return result;
    }

    inline void writePort0(unsigned char value)
    {
        this->ctx.addr &= this->getVramSize() - 1;
        this->ctx.readBuffer = value;
        if (this->isExpansionRAM()) {
            this->exRam[this->ctx.addr & 0xFFFF] = this->ctx.readBuffer;
        } else {
            this->ctx.ram[this->ctx.addr] = this->ctx.readBuffer;
        }
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
    inline void renderScanline(int lineNumber)
    {
        if (0 <= lineNumber && lineNumber < 192) {
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
        this->ctx.readBuffer = this->isExpansionRAM() ? this->exRam[this->ctx.addr & 0xFFFF] : this->ctx.ram[this->ctx.addr];
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
        bool previousInterrupt = this->isEnabledInterrupt();
        this->ctx.reg[rn] = value;
        if (!previousInterrupt && this->isEnabledInterrupt() && this->ctx.stat[0] & 0x80) {
            this->detectBlank(this->arg);
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
        // TODO
        renderSpritesMode2(lineNumber);
    }

    inline void renderScanlineModeG5(int lineNumber)
    {
        // TODO
        renderSpritesMode2(lineNumber);
    }

    inline void renderScanlineModeG6(int lineNumber)
    {
        // TODO
        renderSpritesMode2(lineNumber);
    }

    inline void renderScanlineModeG7(int lineNumber)
    {
        // TODO
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
};

#endif // INCLUDE_V9938_HPP
